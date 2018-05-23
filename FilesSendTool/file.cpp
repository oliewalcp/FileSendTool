#include "file.h"
#include "ui_file.h"
#include "mainwindow.h"
#include <QFileDialog>

QHash<int, File*> File::myFiles;//我上传的文件（编号 —— 文件名）
QString File::localIPAddress;//本地IP地址
QHash<QString, QHash<int, File*>*> File::othersFiles;
QHash<int, File*> File::mySelectedFiles;//我选择的文件

File::File(ConnectTools *contool, QString ip, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::File),
    SenderIP(ip),
    con(contool)
{
    ui->setupUi(this);
    if(ip == localIPAddress)
    {
        ui->saveTo->setText("删除");
        ui->fileName->setEnabled(false);
    }
}

File::~File()
{
    delete ui;
}
QString File::getCheckFileName()
{
    if(ui->fileName->checkState() == Qt::Checked)
        return ui->fileName->text();
    else return "";
}
void File::setFileName(QString filename)
{
    ui->fileName->setText(filename);
}

QString File::getFileName()
{
    return ui->fileName->text();
}

void File::setFile(QFileInfo &file)
{
    setFileName(file.absoluteFilePath());
}
void File::on_fileName_stateChanged(int arg1)
{
    switch(arg1)
    {
    case Qt::Checked: mySelectedFiles.insert(id, this); break;
    case Qt::Unchecked: mySelectedFiles.remove(id); break;
    }
}
//保存到 按钮事件
void File::on_saveTo_clicked()
{
    if(ui->saveTo->text() == "保存到")
    {
        QString dir = QFileDialog::getExistingDirectory(
                    this,
                    tr("保存到"),
                    "/home",
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(dir.isNull() || dir.isEmpty()) return;
        con->sendMessage("0002;" + QString::number(id), SenderIP);
        MainWindow::receiving = true;
        emit on_saveToClicked_event(this);
    }
    else if(ui->saveTo->text() == "删除")
    {
        con->sendMessage("0003;" + QFileInfo(getFileName()).fileName());
        emit on_deleting_event(this);
    }
}
