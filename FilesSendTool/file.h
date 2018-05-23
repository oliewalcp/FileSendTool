#ifndef FILE_H
#define FILE_H

#include <QWidget>
#include <unordered_map>
#include <QFileInfo>
#include <QVBoxLayout>
#include "connecttools.h"

namespace Ui {
class File;
}

using __uint64 = unsigned long long;

class File : public QWidget
{
    Q_OBJECT
public:
    explicit File(ConnectTools *contool, QString ip, QWidget *parent = 0);
    ~File();

    void setFile(QFileInfo &file);
    void setFileName(QString filename);
    void setSenderIP(QString ip){SenderIP = ip;}
    void setId(int id){ this->id = id;}//设置id
    void setFileSize(__uint64 size){fileSize = size;}
    __uint64 getFileSize(){return fileSize;}
    int getId(){return id;}
    QString getCheckFileName();//获取勾选状态下的文件名
    QString getFileName();//获取文件名
    QString getSenderIP(){return SenderIP;}

    static QHash<int, File*> myFiles;//我上传的文件（id —— File对象）
    static QHash<QString, QHash<int, File*>*> othersFiles;//别人上传的文件（IP地址，File容器）
    static QHash<int, File*> mySelectedFiles;//我选择的文件（File对象，当set用）
    static QString localIPAddress;//本地IP地址
signals:
    void on_deleting_event(QWidget *child);
    void on_determineDir_event(QString dir);
    void on_saveToClicked_event(File*);
private slots:
    void on_fileName_stateChanged(int arg1);

    void on_saveTo_clicked();

private:
    Ui::File *ui;
    ConnectTools *con;
    int id;
    __uint64 fileSize;
    QString SenderIP;//发送者IP地址
};

#endif // FILE_H
