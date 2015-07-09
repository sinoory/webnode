/*
 *程序的上传文件界面
 *用于选择需要上传的文件，只有在登陆离线空间之后才能显示此界面
 */
#include "download_mainwindow.h"
#include "cloud_upload.h"
#include "ui_cloud_upload.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QTextCodec>
#include <QSettings>
#include <QDir>

extern "C"{
    #include "../include/apx_hftsc_api.h"
}
extern QString fileId;
extern QString cloud_filename;
extern QString cloud_path;
extern int task_id;

cloud_upload::cloud_upload(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cloud_upload)
{
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    ui->setupUi(this);
}

cloud_upload::~cloud_upload()
{
    delete ui;
}

void cloud_upload::on_pushButton_3_clicked()
{
    QString files = QFileDialog::getOpenFileName(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

    if(files.isEmpty())
        return;

    ui->lineEdit->setText(files);
}

void cloud_upload::on_pushButton_clicked()
{
    done(1);
    QString file = ui->lineEdit->text();

    QFile files(file);
    if(!files.exists()){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("文件不存在"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);
        return;
    }
    QStringList list_file = file.split("/");
    cloud_filename = list_file.last();
    cloud_path = file.mid(0,file.length()-cloud_filename.length()-1);
    struct apx_trans_opt addnew_opt = {0};
    QByteArray filename_array = cloud_filename.toLocal8Bit();
    char *filename_c = filename_array.data();
    QByteArray path_array = cloud_path.toLocal8Bit();
    char *path_c = path_array.data();
    strncpy(addnew_opt.fname,filename_c,sizeof(addnew_opt.fname)-1);
    strncpy(addnew_opt.fpath,path_c,sizeof(addnew_opt.fpath)-1);
    addnew_opt.type = APX_TASK_TYPE_SERVER_UP;

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
    QString url = "http://" + s_ip + ":" + QString::number(port) + "/upload";
    QByteArray url_array = url.toLocal8Bit();
    char *url_c = url_array.data();
    strncpy(addnew_opt.uri,url_c,sizeof(addnew_opt.uri)-1);
    int check_result = apx_task_uri_check(&addnew_opt);
    if(check_result<0){
        done(0);
        return;
    }
    if(check_result>0){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("当前文件已上传！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);
        return;
    }
    QString user_path = QDir::homePath();
    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
    unsigned int upload = settings.value("settings/upload",1024).toInt();
    unsigned int download = settings.value("settings/download",1024).toInt();
    addnew_opt.up_splimit = upload;
    addnew_opt.down_splimit = download;
    addnew_opt.concurr = one_max_thread;
    task_id  = apx_task_create(&addnew_opt);
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
    fileId = QString(QLatin1String(addnew_opt.fileId));
    apx_task_start(task_id);

    return;
}

void cloud_upload::on_pushButton_2_clicked()
{
    done(0);
    return;
}
