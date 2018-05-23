#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QIODevice>

std::atomic<bool> MainWindow::receiving;//标记是否在接收文件中
std::atomic<bool> MainWindow::sending;//标记是否正在发送文件中
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    forbidRefresh = false;
    receiving = false;
    sending = false;
    baseUnit = 1;
    connectTools = new ConnectTools();
    File::localIPAddress = connectTools->getLocalIPAddress();
    ui->SendFileName->setText(tr("你的IP地址：") + File::localIPAddress);
    connect(connectTools, SIGNAL(on_getFileList_event(QString, QList<QWidget *>*)), this, SLOT(on_getOtherFile_event(QString, QList<QWidget *>*)));
    connect(connectTools, SIGNAL(on_getDeleteFile_event(QWidget*)), this, SLOT(on_childDelete_event(QWidget*)));
    connect(connectTools, SIGNAL(on_getFileContent_event(QByteArray*, QString, int)), this, SLOT(on_getFileContent_event(QByteArray*, QString, int)));
    connect(connectTools, SIGNAL(on_sendSizeChange_event(__uint64)), this, SLOT(on_SizeChange_event(__uint64)));
    connect(connectTools, SIGNAL(on_receiveSizeChange_event(__uint64)), this, SLOT(on_SizeChange_event(__uint64)));
    connectTools->sendMessage("0000");
}
MainWindow::~MainWindow()
{
    delete connectTools;
    delete ui;
}
//选择发送的文件
void MainWindow::on_SelectSendFile_clicked()
{
    if(File::myFiles.size() > 254)
    {
        QMessageBox::warning(this, tr("请节制"), tr("发送文件过多，请取消发送不必要的文件再选择发送文件"), QMessageBox::Close);
        return;
    }
    //选择文件
    QList<QString> filename = QFileDialog::getOpenFileNames(
        this,
        QMainWindow::tr("选择文件"),
        QMainWindow::tr("/home"));
    if(filename.isEmpty()) return;
    int i = 0;
    for(QString name : filename)
    {
        while(File::myFiles.contains(i)) i++;
        QFileInfo file(name);
        File *f = new File(connectTools, File::localIPAddress);
        f->setId(i);
        f->setFile(file);
        File::myFiles.insert(i++, f);
        ui->FileList->insertWidget(0, f);
    }
}
//刷新列表
void MainWindow::refreshList()
{
    File::mySelectedFiles.clear();
    for(QHash<int, File*>* temp : File::othersFiles.values())
    {
        for(File* file : temp->values())
        {
            file->hide();
            ui->FileList->removeWidget(file);
            delete file;
        }
    }
    File::othersFiles.clear();
    for(File *f : File::myFiles.values())
    {
        ui->FileList->addWidget(f);
        connect(f, SIGNAL(on_deleting_event(QWidget*)), this, SLOT(on_childDelete_event(QWidget*)));
    }
    connectTools->sendMessage("0000");
}
//接收按钮
void MainWindow::on_Receive_clicked()
{
    if(sending && QMessageBox::warning(this, tr("遇到点问题"), tr("正在发送文件，无法操作！"), QMessageBox::Close) == QMessageBox::Close)
        return;
    if(receiving && QMessageBox::warning(this, tr("遇到点问题"), tr("正在接收文件，无法操作！"), QMessageBox::Close) == QMessageBox::Close)
        return;
    if(File::mySelectedFiles.size() == 0)
    {
        QMessageBox::information(
                this,
                QMainWindow::tr("请确认"),
                QMainWindow::tr("没有选择接收的文件"),
                QMessageBox::Close);
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(
                this,
                tr("选择保存的目录"),
                "/",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir.isNull() || dir.isEmpty()) return;
    saveDir = dir;
    allSize = 0;
    __uint64 t = 1;
    ui->unit->setText(tr("Bytes"));
    for(File *file : File::mySelectedFiles.values())
    {
        allSize += (double)(file->getFileSize()) / (double)t;
        if(allSize >= 1024)
        {
            allSize /= 1024.0;
            t *= 1024;
        }
        connectTools->sendMessage("0002;" + file->getId(), file->getSenderIP());
    }
    switch(t)
    {
    case 1:ui->unit->setText(tr("B"));break;
    case 1024:ui->unit->setText(tr("KB"));break;
    case 1048576:ui->unit->setText(tr("MB"));break;
    case 1073741824:ui->unit->setText(tr("GB"));break;
    }
    baseUnit = t;
    ui->filesize->setText(QString::number(allSize, 'g', 2));
    receiving = true;
}
//刷新按钮
void MainWindow::on_pushButton_clicked()
{
    if(forbidRefresh) return;
    addressRefresh();
    forbidRefresh = true;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_timer_event()));
    timer->setInterval(5000);//5秒防刷
    timer->start();
    refreshList();
}
void MainWindow::on_childDelete_event(QWidget *child)
{
    child->hide();
    ui->FileList->removeWidget(child);
    delete child;
}
void MainWindow::on_getOtherFile_event(QString ip, QList<QWidget *> *newList)
{
    if(newList == nullptr)
    {
        QHash<int, File*>* List = File::othersFiles.value(ip);
        for(File* f : List->values())
        {
            ui->FileList->addWidget(f);
            connect(f, SIGNAL(on_determineDir_event(QString)), this, SLOT(on_getSaveDir_event(QString)));
            connect(f, SIGNAL(on_saveToClicked_event(File*)), this, SLOT(on_saveTo_event(File*)));
        }
    }
    else
    {
        for(QWidget *temp : *newList)
        {
            File *f = (File*) temp;
            ui->FileList->addWidget(f);
            connect(f, SIGNAL(on_determineDir_event(QString)), this, SLOT(on_getSaveDir_event(QString)));
            connect(f, SIGNAL(on_saveToClicked_event(File*)), this, SLOT(on_saveTo_event(File*)));
        }
    }
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(sending)
    {
        if(QMessageBox::warning(this, tr("请确认"), tr("当前正在发送文件，是否退出？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            event->accept();
        else
        {
            event->ignore();
            return;
        }
    }
    if(receiving)
    {
        if(QMessageBox::warning(this, tr("请确认"), tr("当前正在接收文件，是否退出？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            event->accept();
        else
        {
            event->ignore();
            return;
        }
    }
    if(QMessageBox::warning(this, tr("请确认"), tr("是否退出？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        event->accept();
    else event->ignore();
    File::myFiles.clear();
    File::othersFiles.clear();
    File::mySelectedFiles.clear();
}
void MainWindow::on_getFileContent_event(QByteArray *fileContent, QString ip, int id)
{
    QString fileName = File::othersFiles.value(ip)->value(id)->getFileName();//获取文件名
    QFile file(saveDir + "/" + fileName);
    if(file.exists())
    {
        int button = QMessageBox::warning(this, tr("请确认"), tr("文件已存在，是否替换？\n按“是”替换，按“否”不替换并跳过该文件"), QMessageBox::Yes | QMessageBox::No);
        if(button == QMessageBox::Yes) file.remove();
        else return;
    }
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_10);
    out.writeRawData(fileContent->data(), fileContent->size());
    file.close();
    File::mySelectedFiles.remove(id);
    delete fileContent;
    if(File::mySelectedFiles.size() == 0)
    {
        receiving = false;
        ui->progressBar->setValue(100);
        QMessageBox::information(this, tr("恭喜"), tr("接收完毕！"), QMessageBox::Ok);
    }
}
void MainWindow::on_SizeChange_event(__uint64 increaseSize)
{
    __uint64 rate = increaseSize / baseUnit / (__uint64)allSize;
    ui->progressBar->setValue(ui->progressBar->value() + rate);
}
void MainWindow::on_timer_event()
{
    delete timer;
    forbidRefresh = false;
}
void MainWindow::addressRefresh()
{
    File::localIPAddress = connectTools->getLocalIPAddress();
    ui->SendFileName->setText(tr("你的IP地址：") + File::localIPAddress);
    for(File *f : File::myFiles.values())
        f->setSenderIP(File::localIPAddress);
}
void MainWindow::on_saveTo_event(File *file)
{
    baseUnit = 1;
    allSize = 1;
    double fileSize = file->getFileSize();
    if(fileSize >= 1073741824)
    {
        fileSize /= 1073741824.0;
        baseUnit *= 1073741824;
        ui->unit->setText(tr("GB"));
    }
    else if(fileSize >= 1048576)
    {
        fileSize /= 1048576.0;
        baseUnit *= 1048576;
        ui->unit->setText(tr("MB"));
    }
    else if(fileSize >= 1024)
    {
        fileSize /= 1024.0;
        baseUnit *= 1024;
        ui->unit->setText(tr("KB"));
    }
    allSize = fileSize;
    ui->filesize->setText(QString::number(allSize));
}
