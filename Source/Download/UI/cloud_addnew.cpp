#include "cloud_addnew.h"

#include "download_mainwindow.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include "bt_list.h"
extern bool cancel_new;
extern QString file_path;
extern QString download_url;
extern QLocalServer m_localServer;
extern int task_id;
extern QString file_name;
extern "C"{
    #include "../lib/apx_hftsc_api.h"
}
cloud_addnew::cloud_addnew(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cloud_addnew)
{
    QSettings settings("/etc/download/settings.ini",QSettings::IniFormat);
    QString path = settings.value("settings/path","/etc/file/download").toString();
    ui->setupUi(this);
    ui->lineEdit_3->setText(QString(path));
    ui->label->hide();
    ui->lineEdit->hide();
    ui->lineEdit_2->hide();
    ui->label_3->hide();
    ui->lineEdit_4->hide();
    ui->label_4->hide();
    ui->pushButton->hide();
    ui->label_5->hide();
    ui->label_6->hide();
    ui->lineEdit_5->hide();
    ui->lineEdit_6->hide();
    ui->lineEdit_6->setEchoMode(QLineEdit::Password);
    ui->lineEdit->installEventFilter(this);
    int i = QCoreApplication::argc();
    QStringList a = QApplication::arguments();
    if(i==2&&download_url==""){
        ui->radioButton->setChecked(true);
        ui->label->show();
        ui->lineEdit->show();
        ui->lineEdit->setText(a[1]);
        ui->label_3->show();
        ui->lineEdit_4->show();
    }
    if(download_url!=""){
        ui->radioButton->setChecked(true);
        ui->label->show();
        ui->lineEdit->show();
        ui->lineEdit->setText(download_url);
        download_url = "";
        ui->label_3->show();
        ui->lineEdit_4->show();
    }
}

cloud_addnew::~cloud_addnew()
{
    delete ui;
}

void cloud_addnew::on_pushButton_2_clicked(bool checked)
{
    cancel_new = false;
    file_path = ui->lineEdit_3->text();
    done(1);
    QByteArray download_array1 = file_path.toLatin1();
    char *download_path = download_array1.data();
    QString filename = ui->lineEdit_4->text();

    QByteArray download_array = filename.toLatin1();
    char *download_name = download_array.data();
    //char *addnew_url = (ui->lineEdit->text().toLatin1()).data();
    //char *task_type;


    unsigned char task_type;
    if(ui->radioButton->isChecked()){
            QByteArray url_array=ui->lineEdit->text().toLatin1();
            char *addnew_url = url_array.data();
            task_type = APX_TASK_TYPE_DOWN;
            file_name = ui->lineEdit_4->text().toLatin1();
            QSettings settings("/etc/download/settings.ini",QSettings::IniFormat);
            unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
            unsigned int upload = settings.value("settings/upload",1024).toInt();
            unsigned int download = settings.value("settings/download",1024).toInt();
            struct apx_trans_opt addnew_opt = {0};
            strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri));
            int check_uri = apx_task_uri_check(&addnew_opt);
            if(check_uri<0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("不能识别下载链接，请确定您的下载链接正确"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            addnew_opt.concurr = one_max_thread;
            addnew_opt.type = task_type;
            if(strcmp(download_name,"")==0){
                download_name = addnew_opt.fname;
                file_name = addnew_opt.fname;
            }
            strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname));
            strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath));

            addnew_opt.up_splimit = upload;
            addnew_opt.down_splimit = download;

            task_id  = apx_task_create(&addnew_opt);
            QSettings settings1("/etc/taskinfo.ini",QSettings::IniFormat);
            settings1.beginGroup(QString::number(task_id));
            settings1.setValue("url",addnew_url);
            settings1.setValue("task_type",task_type);
            settings1.setValue("concurr",one_max_thread);
            settings1.setValue("download_path",download_path);
            settings1.setValue("download_name",download_name);
            settings1.endGroup();
            char fpath[100]={0};
            char fname[10]={0};
            apx_task_file_name_get(task_id,fpath,100,fname,10);
            if(task_id==-1){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("uid不存在"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            if(task_id==-2){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("未知的协议"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            if(task_id==-3){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取下载文件大小失败"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            if(task_id==-4){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("申请内存失败"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            //apx_task_limit_set(task_id,download,upload);
            //apx_task_concur_set(task_id,one_max_thread);
            int res = apx_task_start(task_id);
            qDebug()<<res<<"************";


    }
    if(ui->radioButton_2->isChecked()){
        QByteArray url_array=ui->lineEdit_2->text().toLatin1();
        char *addnew_url = url_array.data();
        task_type = APX_TASK_TYPE_DOWN;
        file_name = ui->lineEdit_4->text().toLatin1();

        QSettings settings("/etc/download/config.ini",QSettings::IniFormat);
        unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
        unsigned int upload = settings.value("settings/upload",1024).toInt();
        unsigned int download = settings.value("settings/download",1024).toInt();
        struct apx_trans_opt addnew_opt = {0};
        addnew_opt.concurr = one_max_thread;
        addnew_opt.type = task_type;
        strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri));
        strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname));
        strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath));

        addnew_opt.up_splimit = upload;
        addnew_opt.down_splimit = download;
        task_id  = apx_task_create(&addnew_opt);
        bt_list btlist1;
        int i = btlist1.exec();
        if(i==0){
            done(0);
            return;
        }
        QByteArray download_array1 = file_name.toLatin1();
        char *bt_download_name = download_array1.data();
        strncpy(addnew_opt.fname,bt_download_name,sizeof(addnew_opt.fname));
        QSettings settings1("/etc/download/taskinfo.ini",QSettings::IniFormat);
        settings1.beginGroup(QString::number(task_id));
        settings1.setValue("url",addnew_url);
        settings1.setValue("task_type",task_type);
        settings1.setValue("concurr",one_max_thread);
        settings1.setValue("download_path",download_path);
        settings1.setValue("download_name",download_name);
        settings1.endGroup();
        if(task_id==-1){
            QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("uid不存在"));
            msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
            return;
        }
        if(task_id==-2){
            QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("未知的协议"));
            msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
            return;
        }
        if(task_id==-3){
            QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取下载文件大小失败"));
            msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
            return;
        }
        if(task_id==-4){
            QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("申请内存失败"));
            msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
            return;
        }
        //apx_task_limit_set(task_id,download,upload);
        //apx_task_concur_set(task_id,one_max_thread);
        int return1 = apx_task_start(task_id);
        qDebug()<<return1<<"***********";
    }
    if(ui->radioButton_3->isChecked()){
            QByteArray url_array=ui->lineEdit->text().toLatin1();
            char *addnew_url = url_array.data();
            task_type = APX_TASK_TYPE_DOWN;
            file_name = ui->lineEdit_4->text().toLatin1();
            QSettings settings("/etc/download/settings.ini",QSettings::IniFormat);
            unsigned int one_max_thread = settings.value("settings/one_max_thread",5).toInt();
            unsigned int upload = settings.value("settings/upload",1024).toInt();
            unsigned int download = settings.value("settings/download",1024).toInt();
            struct apx_trans_opt addnew_opt = {0};
            strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri));
            QString username = ui->lineEdit_5->text();
            QString password = ui->lineEdit_6->text();
            if (username != "" && password!=""){
                QByteArray username_arr = username.toLatin1();
                char *file_username = username_arr.data();
                QString password = ui->lineEdit_6->text();
                QByteArray password_arr = password.toLatin1();
                char *file_password = password_arr.data();
                strncpy(addnew_opt.ftp_user,file_username,sizeof(addnew_opt.ftp_user));
                strncpy(addnew_opt.ftp_passwd,file_password,sizeof(addnew_opt.ftp_passwd));
            }
            int check_uri = apx_task_uri_check(&addnew_opt);
            if(check_uri<0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("不能识别下载链接，请确定您的下载链接正确"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            addnew_opt.concurr = one_max_thread;
            addnew_opt.type = task_type;
            if(strcmp(download_name,"")==0){
                download_name = addnew_opt.fname;
                file_name = addnew_opt.fname;
            }
            strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname));
            strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath));


            addnew_opt.up_splimit = upload;
            addnew_opt.down_splimit = download;
            task_id  = apx_task_create(&addnew_opt);
            QSettings settings1("/etc/download/taskinfo.ini",QSettings::IniFormat);
            settings1.beginGroup(QString::number(task_id));
            settings1.setValue("url",addnew_url);
            settings1.setValue("task_type",task_type);
            settings1.setValue("concurr",one_max_thread);
            settings1.setValue("download_path",download_path);
            settings1.setValue("download_name",download_name);
            settings1.endGroup();
            char fpath[100]={0};
            char fname[10]={0};
            apx_task_file_name_get(task_id,fpath,100,fname,10);
            if(task_id==-1){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("uid不存在"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            if(task_id==-2){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("未知的协议"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            if(task_id==-3){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取下载文件大小失败"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            if(task_id==-4){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("申请内存失败"));
                msgbox.setIconPixmap(QPixmap("/etc/download/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                done(0);
                return;
            }
            //apx_task_limit_set(task_id,download,upload);
            //apx_task_concur_set(task_id,one_max_thread);
            int res = apx_task_start(task_id);
            qDebug()<<res<<"************";


    }

    this->hide();
}

void cloud_addnew::on_pushButton_3_clicked()
{
    cancel_new = true;
    this->hide();
}



void cloud_addnew::on_radioButton_clicked()
{
    ui->label->show();
    ui->lineEdit->show();
    ui->lineEdit_2->hide();
    ui->pushButton->hide();
    ui->label_4->hide();
    ui->label_3->show();
    ui->lineEdit_4->show();
    ui->label_5->hide();
    ui->label_6->hide();
    ui->lineEdit_5->hide();
    ui->lineEdit_6->hide();
}

void cloud_addnew::on_radioButton_2_clicked()
{
    ui->label->hide();
    ui->lineEdit->hide();
    ui->lineEdit_2->show();
    ui->pushButton->show();
    ui->label_4->show();
    ui->label_3->hide();
    ui->lineEdit_4->hide();
    ui->label_5->hide();
    ui->label_6->hide();
    ui->lineEdit_5->hide();
    ui->lineEdit_6->hide();
}

void cloud_addnew::on_radioButton_3_clicked()
{
    ui->label->show();
    ui->lineEdit->show();
    ui->lineEdit_2->hide();
    ui->pushButton->hide();
    ui->label_4->hide();
    ui->label_3->show();
    ui->lineEdit_4->show();
    ui->label_5->show();
    ui->label_6->show();
    ui->lineEdit_5->show();
    ui->lineEdit_6->show();
}

void cloud_addnew::on_pushButton_clicked()
{
    QString files = QFileDialog::getOpenFileName(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
    if(files.isEmpty())
        return;
    ui->lineEdit_2->setText(files);

};

void cloud_addnew::on_pushButton_4_clicked()
{
     QString files = QFileDialog::getExistingDirectory(this,tr("选择目录"),QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
     if(files.isEmpty())
         return;
     ui->lineEdit_3->setText(files);
}

void cloud_addnew::change_url(){
    if(download_url!=""){
        ui->radioButton->setChecked(true);
        ui->label->show();
        ui->lineEdit->show();
        ui->lineEdit->setText(download_url);
        ui->label_3->show();
        ui->lineEdit_4->show();
        download_url = "";
    }
}



void cloud_addnew::on_lineEdit_textChanged(const QString &arg1)
{
    /*struct apx_trans_opt addnew_opt = {0};
    QByteArray url_array=arg1.toLatin1();
    char *addnew_url = url_array.data();
    strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri));
    int check_uri = apx_task_uri_check(&addnew_opt);
    if(check_uri==0){
        ui->lineEdit_4->setText(QString(addnew_opt.fname));
    }*/
}

bool cloud_addnew::eventFilter(QObject *watched, QEvent *event){

        if(event->type()==QEvent::FocusOut){
            struct apx_trans_opt addnew_opt = {0};
            QByteArray url_array=ui->lineEdit->text().toLatin1();
            char *addnew_url = url_array.data();
            strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri));
             qDebug()<<"fname:"<<addnew_opt.fname<<" fsize"<<addnew_opt.fsize;
            int check_uri = apx_task_uri_check(&addnew_opt);
            qDebug()<<"fname:"<<addnew_opt.fname<<" fsize"<<addnew_opt.fsize;
            if(check_uri==0){
                ui->lineEdit_4->setText(QString(addnew_opt.fname));
            }
        }

    return QWidget::eventFilter(watched,event);
}
