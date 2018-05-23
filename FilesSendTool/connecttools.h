#ifndef CONNECTTOOLS_H
#define CONNECTTOOLS_H

#include <QUdpSocket>
#include <QTcpSocket>

#define UDP_PORTNUMBER 33333 //UDP端口号
#define TCP_PORTNUMBER 33334 //TCP端口号

using __uint64 = unsigned long long;

class ConnectTools : public QObject
{
    Q_OBJECT
public:
    ConnectTools();//构造函数
    ~ConnectTools();//析构函数
    void sendMessage(QString MesData, QString IPAddress = "");//发送消息
    void sendMessage(QByteArray &MesData, QString IPAddress = "");//发送消息
    void sendFileTo();//发送文件到
    QString getLocalIPAddress();//获取本地IP地址
    bool getNetworkStatus();//获取网络状态
signals:
    void on_getFileList_event(QString, QList<QWidget *>*);//收到别人的文件列表
    void on_getDeleteFile_event(QWidget*);//收到别人取消发送文件
    void on_getFileContent_event(QByteArray *, QString, int);//收到别人发送的文件内容
    void on_sendSizeChange_event(__uint64);//发送一个文件完成
    void on_receiveSizeChange_event(__uint64);//接收一个文件完成
private slots:
    void on_ReceiveFile_event();//接收文件事件
    void on_ReceiveMsg_event();//接收消息事件
private:
    QUdpSocket *msgReceiver;//消息接收器
    QTcpSocket *fileRecevier;//文件接收器
};
#endif // CONNECTTOOLS_H
