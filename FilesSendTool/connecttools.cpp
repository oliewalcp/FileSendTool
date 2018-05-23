#include <QHostInfo>
#include <thread>
#include "connecttools.h"
#include "file.h"
#include "mainwindow.h"
/* UDP协议通信规则 ;分隔类型与文件，文件间隔用:，文件属性用@分割
 * 0000 —— 获取文件列表
 * 0001 —— 获取别人发送的文件列表，参数：文件列表（其中，包含文件id、文件大小）
 * 0002 —— 准备接收文件，参数：文件id（用@分开各id）
 * 0003 —— 取消发送文件，参数：文件id
 * 0004 —— 发送文件内容，参数：文件id（第一位）
 *
 */
ConnectTools::ConnectTools()
{
    getLocalIPAddress();
    msgReceiver = new QUdpSocket();
    msgReceiver->bind(UDP_PORTNUMBER, QUdpSocket::ShareAddress);
    connect(msgReceiver, SIGNAL(readyRead()), this, SLOT(on_ReceiveMsg_event()));
}
ConnectTools::~ConnectTools()
{
    delete msgReceiver;
}
bool ConnectTools::getNetworkStatus()
{
    if(File::localIPAddress == "") return false;
    return true;
}
QString ConnectTools::getLocalIPAddress()
{
    QString Local_IP = "";
    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    //如果存在多条ip地址ipv4和ipv6：
    for(QHostAddress address : info.addresses())
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
            return address.toString();//只取ipv4协议的地址
    return Local_IP;
}

void ConnectTools::sendMessage(QString MesData, QString IPAddress)
{
    QUdpSocket *send = new QUdpSocket();
    QByteArray datagram = MesData.toUtf8();
    send->writeDatagram(datagram.data(), datagram.size(), IPAddress == "" ? QHostAddress::Broadcast : QHostAddress(IPAddress), UDP_PORTNUMBER);
    delete send;
}
void ConnectTools::sendMessage(QByteArray &MesData, QString IPAddress)
{
    QUdpSocket *send = new QUdpSocket();
    send->writeDatagram(MesData.data(), MesData.size(), IPAddress == "" ? QHostAddress::Broadcast : QHostAddress(IPAddress), UDP_PORTNUMBER);
    delete send;
}
void ConnectTools::on_ReceiveFile_event()
{

}
void ConnectTools::on_ReceiveMsg_event()
{
    QHostAddress sender;
    QByteArray datagram;
    datagram.resize(msgReceiver->pendingDatagramSize());
    msgReceiver->readDatagram(datagram.data(), datagram.size(), &sender);
    //获取发送者的IP地址
    QList<QString> str = sender.toString().split(":");
    QString remoteIP = str.at(str.size() - 1);
    if(remoteIP == File::localIPAddress) return;//忽略自己发的消息
    QString msg(datagram);
    QList<QString> result = msg.split(";");
    int style = result.at(0).toInt();
    switch(style)
    {
    case 0://别人要获取文件列表
    {
        QString fileList = "0001;";
        for(int k : File::myFiles.keys())
        {
            File *temp = File::myFiles.value(k);
            QFileInfo info = QFileInfo(temp->getFileName());
            if(fileList != "0001;") fileList += ":";
            fileList += info.fileName() + tr("@") + QString::number(k) + tr("@") + QString::number(info.size());
        }
        sendMessage(fileList, remoteIP);
    }break;
    case 1://别人回信，收到别人的文件列表
    {
        if(result.at(1) == "") return;
        QHash<int, File*> *fileContainer = nullptr;
        QList<QWidget *>* newList = nullptr;
        if(!File::othersFiles.contains(remoteIP))
        {
            fileContainer = new QHash<int, File*>();
            newList = new QList<QWidget*>();
        }
        else fileContainer = File::othersFiles.value(remoteIP);
        QList<QString> fileList = result.at(1).split(":");
        //处理文件
        for(QString file : fileList)
        {
            result = file.split("@");
            File* temp = new File(this, remoteIP);
            temp->setFileName(result.at(0));
            temp->setId(result.at(1).toInt());
            temp->setFileSize(result.at(2).toInt());
            fileContainer->insert(temp->getId(), temp);
            if(newList != nullptr) newList->append(temp);
        }
        File::othersFiles.insert(remoteIP, fileContainer);
        emit on_getFileList_event(remoteIP, newList);
    }break;
    case 2://收到别人的接收文件请求
    {
        MainWindow::sending = true;
        QList<QString> idList = result.at(1).split("@");
        for(QString id : idList)
        {
            char ID = id.toInt();
            if(!File::myFiles.contains((int)ID)) break;
            File *myfile = File::myFiles.value((int)ID);
            QString fileName = myfile->getFileName();//获取文件的绝对路径
            //打开文件
            QFile file(fileName);
            file.open(QIODevice::ReadOnly);
            QDataStream in(&file);
            in.setVersion(QDataStream::Qt_5_10);
            QByteArray fileContent;
            __uint64 fileSize = file.size();
            fileContent.resize(fileSize);
            in.readRawData(fileContent.data(), fileSize);//读取文件字节内容
            QByteArray result;
            result.append("0004;");
            result.append(ID);
            result.append(fileContent);
            sendMessage(result, remoteIP);
            emit on_sendSizeChange_event(fileSize);
        }
        MainWindow::sending = false;
    }break;
    case 3://收到别人的取消发送文件消息
    {
        int ID = result.at(1).toInt();
        QHash<int, File*> *fileContainer = File::othersFiles.value(remoteIP);
        QWidget *temp = fileContainer->value(ID);
        fileContainer->remove(ID);
        emit on_getDeleteFile_event(temp);
    }break;
    case 4://收到别人发送的文件内容
    {
        datagram.remove(0, 5);
        int id = datagram.at(0);
        datagram.remove(0, 1);
        emit on_getFileContent_event(&datagram, remoteIP, id);
    }break;
    }
}
