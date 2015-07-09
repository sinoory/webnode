/*
 *程序的新建界面
 *用于处理程序新建任务的一些操作
 *同时还负责对输入的url进行探测以及跟浏览器插件的交互
 */

#include "addnew.h"
#include "ui_addnew.h"
#include "download_mainwindow.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDir>
#include <QTextCodec>
#include "bt_list.h"
#include <unistd.h>

extern "C"{
    #include "../include/apx_hftsc_api.h"
    #include "../client/apx_proto_ctl.h"
    //#include "../include/apx_list.h"
}
extern bool cancel_new;
extern QString file_path;
extern QString download_url;
extern QLocalServer m_localServer;
extern int task_id;
extern QString file_name;
extern bool is_login;
static QString user_path;
extern bool param_start;
extern int check_uri;
QString download_cookie="";

addnew::addnew(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::addnew)		//初始化新建界面，读取设置中的默认路径，将默认路径显示在界面上，并隐藏大部分控件
{
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    user_path = QDir::homePath();
    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    QString path = settings.value("settings/path",user_path + "/下载").toString();
    ui->setupUi(this);
    ui->lineEdit_path->setText(QString(path));
    ui->label_url->hide();
    ui->lineEdit_url->hide();
    ui->lineEdit_bt_url->hide();
    ui->label_filename->hide();
    ui->lineEdit_filename->hide();
    ui->label_bt_url->hide();
    ui->pushButton_scan_bt->hide();
    ui->label_username->hide();
    ui->label_password->hide();
    ui->lineEdit_username->hide();
    ui->lineEdit_password->hide();
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
    ui->lineEdit_url->installEventFilter(this);
    int i = QCoreApplication::argc();
    QStringList a = QApplication::arguments();

    if(i>=2&&download_url==""){
        ui->radioButton_http->setChecked(true);
        ui->label_url->show();
        ui->lineEdit_url->show();
        ui->lineEdit_url->setText(a[1]);

        if(i>=3){
            ui->lineEdit_filename->setText(a[2]);
            if(i==4)
                download_cookie = a[3];
        }
        ui->label_filename->show();
        ui->lineEdit_filename->show();
    }

}

addnew::~addnew()
{
    delete ui;
}

void addnew::on_pushButton_ok_clicked()		//当填写完下载信息之后点击确定，执行一下操作
{
    cancel_new = false;
    file_path = ui->lineEdit_path->text();
    done(1);
    QByteArray download_array1 = file_path.toLocal8Bit();
    char *download_path = download_array1.data();
    if(access(download_path,4)<0){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("无权在该目录下创建文件！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        done(0);
        return;
    }
    QString filename = ui->lineEdit_filename->text();
    QByteArray download_array = filename.toLocal8Bit();
    char *download_name = download_array.data();
    unsigned char task_type;
    if(ui->chk_offline->isChecked())
        task_type = APX_TASK_TYPE_SERVER_DOWN;
    else
        task_type = APX_TASK_TYPE_DOWN;
    bool is_reset = false;
    if(ui->radioButton_http->isChecked()){		//当下载任务是http下载时，执行的操作
            QByteArray url_array=ui->lineEdit_url->text().toLocal8Bit();
            char *addnew_url = url_array.data();
            file_name = ui->lineEdit_filename->text().toLocal8Bit();
            QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
            unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
            unsigned int upload = settings.value("settings/upload",1024).toInt();
            unsigned int download = settings.value("settings/download",1024).toInt();
            struct apx_trans_opt addnew_opt = {0};
            strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri)-1);

            if(download_cookie!=""){

                if(download_cookie.left(7)!="Cookie:")
                    download_cookie = "Cookie: " + download_cookie;

                QByteArray cookie_array = download_cookie.toLocal8Bit();
                char *cookie_char = cookie_array.data();
                addnew_opt.cookie = new char[5000];
                strncpy(addnew_opt.cookie,cookie_char,5000);
                download_cookie="";
            }

            else
                addnew_opt.cookie = NULL;

            addnew_opt.type = task_type;
            qDebug()<<"task type is : "<<addnew_opt.type;
            check_uri = apx_task_uri_check(&addnew_opt);

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
                if(strcmp(download_name,"")==0 && strcmp(addnew_opt.fname,"")==0){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("文件名不能为空！"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    done(0);
                    return;
                }

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

            QString path_and_name = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text();
            QString path_and_name_tmp = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text() + ".tmp";
            QString path_and_name_info = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text() + ".info";
            if(QFile::exists(path_and_name) || QFile::exists(path_and_name_tmp) || QFile::exists(path_and_name_info)){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("文件名已存在！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                if(ui->lineEdit_filename->text()!=""){
                    msgbox.exec();
                    done(0);
                    return;
                }
            }

            /*if(addnew_opt.fsize<=0&&is_reset==false){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取文件大小失败！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }*/

            addnew_opt.concurr = one_max_thread;

            if(strcmp(download_name,"")==0){
                download_name = addnew_opt.fname;
                file_name = addnew_opt.fname;
            }

            strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname)-1);
            strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath)-1);

            addnew_opt.up_splimit = upload;
            addnew_opt.down_splimit = download;
            if(ui->chk_offline->isChecked()){
                int server_state;
                task_id = apx_cloud_task_create(&addnew_opt,&server_state);
                if(server_state!=200){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("连接服务器失败！"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    done(0);

                    return;
                }
                else if(task_id<0){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("创建任务失败！"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    done(0);

                    return;
                }
                else{
                    int task_state;
                    int http_code;
                    if(apx_cloud_task_start(task_id,&http_code,&task_state)<0){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("启动任务失败！"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();
                        done(0);

                    }
                    return;
                }

            }
            else
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

            QSettings settings1(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
            settings1.beginGroup(QString::number(task_id));
            settings1.setValue("url",addnew_url);
            settings1.setValue("task_type",task_type);
            settings1.setValue("concurr",one_max_thread);
            settings1.setValue("download_path",file_path);
            settings1.setValue("download_name",file_name);
            settings1.endGroup();
            char fpath[100]={0};
            char fname[10]={0};

            if(apx_task_file_name_get(task_id,fpath,sizeof(fpath),fname,sizeof(fname))<0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取文件名失败!"));
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
            if(is_reset){
                done(2);
                is_reset = false;
            }

            if(ui->chk_offline->isChecked())
                done(3);
            download_cookie = "";
            int bp_result = apx_task_bpcontinue_get(task_id);
            qDebug()<<"bp_continue is : "<<bp_result;

    }
    if(ui->radioButton_bt->isChecked()){			//当下载任务是bt下载时执行的操作
        QByteArray url_array=ui->lineEdit_bt_url->text().toLocal8Bit();
        char *addnew_url = url_array.data();
        if(ui->chk_offline->isChecked())
            task_type = APX_TASK_TYPE_SERVER_DOWN;
        else
            task_type = APX_TASK_TYPE_DOWN;
        file_name = ui->lineEdit_filename->text().toLocal8Bit();
        QSettings settings(user_path + FILE_PATH + "/config.ini",QSettings::IniFormat);
        unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
        unsigned int upload = settings.value("settings/upload",1024).toInt();
        unsigned int download = settings.value("settings/download",1024).toInt();
        struct apx_trans_opt addnew_opt = {0};
        addnew_opt.concurr = one_max_thread;
        addnew_opt.type = task_type;
        strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri)-1);
        strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname)-1);
        strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath)-1);
        addnew_opt.up_splimit = upload;
        addnew_opt.down_splimit = download;
        //addnew_opt.bp_continue = '1';
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
        bt_list btlist1;
        int i = btlist1.exec();		//弹出bt文件列表界面，显示该bt种子中包含的所有要下载的文件

        if(i==0){
            done(0);

            return;
        }

        QByteArray download_array1 = file_name.toLocal8Bit();
        char *bt_download_name = download_array1.data();
        strncpy(addnew_opt.fname,bt_download_name,sizeof(addnew_opt.fname)-1);
        QSettings settings1(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
        settings1.beginGroup(QString::number(task_id));
        settings1.setValue("url",addnew_url);
        settings1.setValue("task_type",task_type);
        settings1.setValue("concurr",one_max_thread);
        settings1.setValue("download_path",download_path);
        settings1.endGroup();

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
        if(ui->chk_offline->isChecked())
            done(3);

    }

    if(ui->radioButton_ftp->isChecked()){			//当下载任务是ftp下载时，执行的操作
            QByteArray url_array=ui->lineEdit_url->text().toLocal8Bit();
            char *addnew_url = url_array.data();
            if(ui->chk_offline->isChecked())
                task_type = APX_TASK_TYPE_SERVER_DOWN;
            else
                task_type = APX_TASK_TYPE_DOWN;
            file_name = ui->lineEdit_filename->text().toLocal8Bit();
            QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
            unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
            unsigned int upload = settings.value("settings/upload",1024).toInt();
            unsigned int download = settings.value("settings/download",1024).toInt();
            struct apx_trans_opt addnew_opt = {0};
            strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri)-1);
            QString username = ui->lineEdit_username->text();
            QString password = ui->lineEdit_password->text();

            if (username != "" && password!=""){
                QByteArray username_arr = username.toLatin1();
                char *file_username = username_arr.data();
                QString password = ui->lineEdit_password->text();
                QByteArray password_arr = password.toLatin1();
                char *file_password = password_arr.data();
                strncpy(addnew_opt.ftp_user,file_username,sizeof(addnew_opt.ftp_user)-1);
                strncpy(addnew_opt.ftp_passwd,file_password,sizeof(addnew_opt.ftp_passwd)-1);
            }
            addnew_opt.type = task_type;
            check_uri = apx_task_uri_check(&addnew_opt);

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

            QString path_and_name = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text();
            QString path_and_name_tmp = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text() + ".tmp";
            QString path_and_name_info = ui->lineEdit_path->text() + "/" + ui->lineEdit_filename->text() + ".info";
            if(QFile::exists(path_and_name) || QFile::exists(path_and_name_tmp) || QFile::exists(path_and_name_info)){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("文件名已存在！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                if(ui->lineEdit_filename->text()!=""){
                    msgbox.exec();
                    done(0);
                    return;
                }
            }

            if(addnew_opt.fsize<=0&&is_reset==false){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取文件大小失败！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }

            addnew_opt.concurr = one_max_thread;

            if(strcmp(download_name,"")==0){
                download_name = addnew_opt.fname;
                file_name = addnew_opt.fname;
            }

            strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname)-1);
            strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath)-1);
            addnew_opt.up_splimit = upload;
            addnew_opt.down_splimit = download;
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

            QSettings settings1(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
            settings1.beginGroup(QString::number(task_id));
            settings1.setValue("url",addnew_url);
            settings1.setValue("task_type",task_type);
            settings1.setValue("concurr",one_max_thread);
            settings1.setValue("download_path",download_path);
            settings1.setValue("download_name",download_name);
            settings1.endGroup();
            char fpath[100]={0};
            char fname[10]={0};

            if(apx_task_file_name_get(task_id,fpath,sizeof(fpath),fname,sizeof(fname))<0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取文件名失败!"));
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

            if(is_reset){
                done(2);
                is_reset = false;
            }

            if(ui->chk_offline->isChecked())
                done(3);

    }

    this->hide();
}

void addnew::on_pushButton_cancel_clicked()		//当点击“取消”时，执行的操作
{
    cancel_new = true;
    this->close();
}



void addnew::on_radioButton_http_clicked()		//当选择“http下载”单选框时执行的操作
{
    ui->label_url->show();
    ui->lineEdit_url->show();
    ui->lineEdit_bt_url->hide();
    ui->pushButton_scan_bt->hide();
    ui->label_bt_url->hide();
    ui->label_filename->show();
    ui->lineEdit_filename->show();
    ui->label_username->hide();
    ui->label_password->hide();
    ui->lineEdit_username->hide();
    ui->lineEdit_password->hide();
}

void addnew::on_radioButton_bt_clicked()		//当选择“bt下载”单选框时执行的操作
{
    ui->label_url->hide();
    ui->lineEdit_url->hide();
    ui->lineEdit_bt_url->show();
    ui->pushButton_scan_bt->show();
    ui->label_bt_url->show();
    ui->label_filename->hide();
    ui->lineEdit_filename->hide();
    ui->label_username->hide();
    ui->label_password->hide();
    ui->lineEdit_username->hide();
    ui->lineEdit_password->hide();
}

void addnew::on_radioButton_ftp_clicked()		//当选择“ftp下载”单选框时执行的操作
{
    ui->label_url->show();
    ui->lineEdit_url->show();
    ui->lineEdit_bt_url->hide();
    ui->pushButton_scan_bt->hide();
    ui->label_bt_url->hide();
    ui->label_filename->show();
    ui->lineEdit_filename->show();
    ui->label_username->show();
    ui->label_password->show();
    ui->lineEdit_username->show();
    ui->lineEdit_password->show();
}

void addnew::on_pushButton_scan_bt_clicked()		//当点击“浏览”按钮时执行的操作
{
    QString files = QFileDialog::getOpenFileName(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
    if(files.isEmpty())
        return;
    ui->lineEdit_bt_url->setText(files);

}

void addnew::on_pushButton_scan_clicked()		//当点击“浏览”按钮时执行的操作
{
     QString files = QFileDialog::getExistingDirectory(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

     if(files.isEmpty())
         return;

     ui->lineEdit_path->setText(files);
}

void addnew::change_url(){		//当浏览器插件传过来的url参数不为空的时候，将url链接在界面上显示

    if(download_url!=""){
        ui->lineEdit_url->setText("");
        ui->lineEdit_filename->setText("");
        QStringList arg = download_url.split("|");
        ui->radioButton_http->setChecked(true);
        ui->label_url->show();
        ui->lineEdit_url->show();
        ui->lineEdit_url->setText(arg[0]);

        if(arg.count()>=2){
            ui->lineEdit_filename->setText(arg[1]);
            if(arg.count()==3){
                download_cookie = arg[2];
            }
        }
        download_url = "";
        ui->label_filename->show();
        ui->lineEdit_filename->show();
    }

}


bool addnew::eventFilter(QObject *watched, QEvent *event){

        /*if(event->type()==QEvent::FocusOut){
            struct apx_trans_opt addnew_opt = {0};
            QByteArray url_array=ui->lineEdit_url->text().toLocal8Bit();
            char *addnew_url = url_array.data();
            strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri)-1);

            if(ui->radioButton_ftp->isChecked()){
                QString username = ui->lineEdit_username->text();
                QString password = ui->lineEdit_password->text();

                if (username != "" && password!=""){
                    QByteArray username_arr = username.toLatin1();
                    char *file_username = username_arr.data();
                    QString password = ui->lineEdit_password->text();
                    QByteArray password_arr = password.toLatin1();
                    char *file_password = password_arr.data();
                    strncpy(addnew_opt.ftp_user,file_username,sizeof(addnew_opt.ftp_user)-1);
                    strncpy(addnew_opt.ftp_passwd,file_password,sizeof(addnew_opt.ftp_passwd)-1);
                }

            }

            int check_uri = apx_task_uri_check(&addnew_opt);

            if(check_uri==0){
                ui->lineEdit_filename->setText(QString(addnew_opt.fname));

                if(strcmp(addnew_opt.fname,"")==0){
                    QString e_url = ui->lineEdit_url->text();
                    QStringList list_url = e_url.split("/");
                    ui->lineEdit_filename->setText(list_url.last());
                }

            }
        }*/

    return QWidget::eventFilter(watched,event);
}



int addnew::exec(){		//当离线空间未登录的时候，隐藏界面上的“离线下载”和“云端加速”

    if(is_login){
        ui->chk_offline->setEnabled(true);
        ui->chk_cloud_accelerate->setEnabled(true);
    }

    else{
        //ui->chk_offline->setEnabled(false);
        ui->chk_cloud_accelerate->setEnabled(false);
    }
    ui->radioButton_http->click();

    return QDialog::exec();
}

void addnew::closeEvent(QCloseEvent *event){
    cancel_new = true;
}
