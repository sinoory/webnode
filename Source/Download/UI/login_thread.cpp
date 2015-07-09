#include "login_thread.h"
#include "download_mainwindow.h"
#include <QMessageBox>
#include <QString>
#include <QPixmap>
extern "C"{
    #include "../include/apx_hftsc_api.h"
}
QString username="";
QString password="";

login_thread::~login_thread(){

}

void login_thread::run(){
    u32 ip;
    u16 port;

    if(apx_conf_serv_get(NULL,0,&ip,&port)!=0){
        QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("连接服务器失败！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        return;
    }

    QByteArray username_array = username.toLatin1();
    char *c_username = username_array.data();
    QByteArray password_array = password.toLatin1();
    char *c_password = password_array.data();

    int uid = apx_user_login(ip,port,c_username,c_password);
    emit login_signal(uid);
    username="";
    password="";
}

void login_thread::set_user_pass(QString user,QString pass){
    username = user;
    password = pass;
}


