/*
 *程序的设置界面
 *用于配置程序的各种全局配置
 */
#include "download_mainwindow.h"
#include "settings.h"
#include "ui_settings.h"
#include <QSettings>
#include <QString>
#include <QFileDialog>
#include <QDesktopServices>
#include "uilog.h"
#include <QMessageBox>
#include <QByteArray>
#include <QValidator>

static QString user_path;
extern int uid;
settings::settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settings)		//初始化主界面，从settings.ini中读取相应的配置，在设置界面中显示出来
{
    user_path = QDir::homePath();
    ui->setupUi(this);
    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    QString path = settings.value("settings/path",user_path + "/下载").toString();
    int max_connection = settings.value("settings/max_connection",5).toInt();
    bool start_open = settings.value("settings/start_open",false).toBool();
    int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
    QString upload = settings.value("settings/upload","1024").toString();
    QString download = settings.value("settings/download","1024").toString();
    QString username = settings.value("settings/username","").toString();
    QString password = settings.value("settings/password","").toString();
    QString ip = settings.value("settings/cloud_ip","").toString();
    QString port = settings.value("settings/cloud_port","").toString();
    bool login = settings.value("settings/auto_login",false).toBool();
    ui->edit_max_upload->setValidator(new QIntValidator(0,1048576,this));
    ui->edit_max_download->setValidator(new QIntValidator(0,1048576,this));
    ui->checkbox_auto_start->setChecked(start_open);
    ui->spinBox_max_thread->setValue(one_max_thread);
    ui->edit_max_upload->setText(upload);
    ui->edit_max_download->setText(download);
    ui->spinBox_max_task->setValue(max_connection);
    ui->edit_default_path->setText(path);
    ui->edit_username->setText(username);
    ui->edit_passward->setText(password);
    ui->edit_cloud_ip->setText(ip);
    ui->edit_cloud_port->setText(port);
    ui->checkbox_login->setChecked(login);
    ui->edit_passward->setEchoMode(QLineEdit::Password);
    //QValidator *validator = new QIntValidator(0,65535,this);
    ui->edit_cloud_port->setValidator(new QIntValidator(0,65535,this));
}

settings::~settings()
{
    delete ui;
}


void settings::on_pushButton_scan_clicked()		//点击“浏览”，选择默认下载路径
{
    QString files = QFileDialog::getExistingDirectory(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

    if(files.isEmpty())
        return;

    ui->edit_default_path->setText(files);
}

void settings::on_pushButton_save_clicked()		//点击“保存”按钮，将配置保存到settings.ini文件中
{
    QString max_connection = ui->spinBox_max_task->text();
    int max_number = ui->spinBox_max_task->text().toInt();
    QString path = ui->edit_default_path->text();
    QString one_max_thread = ui->spinBox_max_thread->text();
    bool start_open = ui->checkbox_auto_start->isChecked();
    QString upload = ui->edit_max_upload->text();
    QString download = ui->edit_max_download->text();
    QString username = ui->edit_username->text();
    QString password = ui->edit_passward->text();
    QString ip = ui->edit_cloud_ip->text();
    u16 port = ui->edit_cloud_port->text().toUShort();
    bool auto_login = ui->checkbox_login->isChecked();
    int server_result = -1;

    if(ip!=""){
        u32 ip_u32 = ipToU32(ip);

        if(ip_u32!=0){

            server_result = apx_conf_serv_set(NULL,ip_u32,port);

            if(server_result<0){
                QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("设置服务器失败!"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                ui->edit_cloud_ip->setText("");
                ui->edit_cloud_port->setText("");
                return;
            }

        }

        else{
            QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("ip格式不正确，请输入正确的ip！"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
            return;
        }

    }

    int writeback_result = apx_conf_writeback();

    if(writeback_result<0){
        UIlog log;
        log.uilog_write("Write config of server failed!Error id is " + QString::number(writeback_result) + "\n");
    }

    if(apx_user_limit_set(uid,0,0,max_number)<0){
        QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("设置最大任务数失败！"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        QSettings settings(user_path + FILE_PATH +"/settings.ini",QSettings::IniFormat);
        ui->spinBox_max_task->setValue(settings.value("settings/max_connection",5).toInt());
        max_connection = settings.value("settings/max_connection",5).toInt();
    }

    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    settings.beginGroup("settings");
    settings.setValue("max_connection",max_connection);
    settings.setValue("path",path);
    settings.setValue("one_max_thread",one_max_thread);
    settings.setValue("start_open",start_open);
    settings.setValue("upload",upload);
    settings.setValue("download",download);
    settings.setValue("username",username);
    settings.setValue("password",password);
    settings.setValue("auto_login",auto_login);
    if(server_result>=0){
        settings.setValue("cloud_ip",ip);
        settings.setValue("cloud_port",port);
    }
    settings.endGroup();
    this->close();
}


void settings::on_pushButton_cancel_clicked()		//点击“取消”按钮
{
    this->close();
}

u32 settings::ipToU32(QString ip){

    QStringList ip_list = ip.split(".");

    if(ip_list.size()<4)
        return 0;

    else{
        QByteArray ip_array = ip.toLatin1();
        char *ip_char = ip_array.data();
        u32 inet_ip = inet_addr(ip_char);
        return inet_ip;
    }

}
