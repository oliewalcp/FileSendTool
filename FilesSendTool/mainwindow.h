#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextCodec>
#include <QCloseEvent>
#include <atomic>
#include <QTimer>
#include "connecttools.h"
#include "file.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
protected:
     void closeEvent(QCloseEvent *event);
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
     static std::atomic<bool> receiving;//标记是否在接收文件中
     static std::atomic<bool> sending;//标记是否正在发送文件中

private slots:
    void on_SelectSendFile_clicked();//选择文件按钮
    void on_Receive_clicked();//接收按钮
    void on_pushButton_clicked();//刷新按钮
    void on_childDelete_event(QWidget *child);//子组件被删除
    void on_getOtherFile_event(QString ip, QList<QWidget *>* newList);//收到别人的文件列表
    void on_getFileContent_event(QByteArray *fileContent, QString ip, int id);//接收到文件内容
    void on_SizeChange_event(__uint64 increaseSize);//新增的大小
    void on_timer_event();//用于刷新限制
    void on_getSaveDir_event(QString dir){saveDir = dir;}
    void on_saveTo_event(File *file);

private:
    Ui::MainWindow *ui;
    double allSize;
    __uint64 baseUnit;//基本单位
    ConnectTools *connectTools;//联网工具
    QString saveDir;//保存的目录
    QTimer *timer;
    std::atomic<bool> forbidRefresh;//标记是否可以刷新

    void refreshList();//刷新列表
    void addressRefresh();//IP地址重定向

};

#endif // MAINWINDOW_H
