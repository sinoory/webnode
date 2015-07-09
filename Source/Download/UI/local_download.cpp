#include "download_mainwindow.h"
#include "local_download.h"
#include "ui_local_download.h"
#include <QString>
#include <QByteArray>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QSettings>
#include <QDebug>
#include <QDir>

extern "C"{
    #include "../include/apx_hftsc_api.h"
}
extern QString f_name;
extern QString file_name;
extern QString file_path;
extern QString fileId;
extern int task_id;
extern int check_uri;
local_download::local_download(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::local_download)
{
    ui->setupUi(this);

}

local_download::~local_download()
{
    delete ui;
}

void local_download::on_pushButton_overflow_clicked()
{
    QString files = QFileDialog::getExistingDirectory(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

    if(files.isEmpty())
        return;

    ui->lineEdit_path->setText(files);
}

void local_download::on_pushButton_cancel_clicked()
{
    done(0);
    return;
}

void local_download::on_pushButton_ok_clicked()
{
    bool is_reset = false;
    file_path = ui->lineEdit_path->text();
    file_name = ui->lineEdit_name->text();
    QByteArray file_path_array = file_path.toLocal8Bit();
    QByteArray file_name_array = file_name.toLocal8Bit();
    char *file_path_c = file_path_array.data();
    char *file_name_c = file_name_array.data();
    struct apx_trans_opt local_opt = {0};
    u32 ip;
    u16 port;

    if(apx_conf_serv_get(NULL,0,&ip,&port)!=0){
        QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("连接服务器失败！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);
        return;
    }
    struct in_addr ip_st;
    ip_st.s_addr = ip;
    QString s_ip = QString(QLatin1String(inet_ntoa(ip_st)));
    QString url = "http://" + s_ip + ":" + QString::number(port) + "/download/" + fileId;
    qDebug()<<"url is: " << url;
    QByteArray url_array = url.toLocal8Bit();
    char *url_c = url_array.data();
    strncpy(local_opt.uri,url_c,sizeof(local_opt.uri)-1);
    strncpy(local_opt.fpath,file_path_c,sizeof(local_opt.fpath)-1);
    local_opt.type = APX_TASK_TYPE_SERVER_DOWN;
    QString user_path = QDir::homePath();
    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
    unsigned int upload = settings.value("settings/upload",1024).toInt();
    unsigned int download = settings.value("settings/download",1024).toInt();
    local_opt.down_splimit = download;
    local_opt.up_splimit = upload;
    local_opt.concurr = one_max_thread;
    check_uri = apx_task_uri_check(&local_opt);

    if(check_uri<0){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("不能识别下载链接，请确定您的下载链接正确"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);
        return;
    }

    else if(check_uri>0){

        QMessageBox msgbox(QMessageBox::NoIcon,tr("提示"),tr("任务已存在，是否重新下载？"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/info.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        QPushButton *button_ok = msgbox.addButton(tr("是"),QMessageBox::AcceptRole);
        QPushButton *button_cancel = msgbox.addButton(tr("否"),QMessageBox::RejectRole);
        int msg_result = msgbox.exec();
        if(msgbox.clickedButton() == button_ok){
                apx_task_destroy(check_uri);
                is_reset = true;
        }

        if(msgbox.clickedButton() == button_cancel || msg_result == 1){
            done(0);
            return;
        }
    }
    strncpy(local_opt.fname,file_name_c,sizeof(local_opt.fname)-1);
    task_id = apx_task_create(&local_opt);

    if(task_id==-1){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("uid不存在"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);

        return;
    }

    if(task_id==-2){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("未知的协议"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);

        return;
    }

    if(task_id==-3){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取下载文件大小失败"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);

        return;
    }

    if(task_id==-4){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("申请内存失败"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);

        return;
    }

    if(apx_task_start(task_id)<0){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("启动任务失败！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);

        return;
    }
    done(1);
    if(is_reset){
        done(2);
        is_reset=false;
    }

    return;

    this->hide();

}

int local_download::exec(){
    QString user_path = QDir::homePath();
    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    QString path = settings.value("settings/path",user_path + "/下载").toString();
    ui->lineEdit_path->setText(path);
    ui->lineEdit_name->setText(f_name);
    return QDialog::exec();
}
