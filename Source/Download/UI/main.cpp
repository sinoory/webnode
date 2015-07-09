/*
 *程序入口
 *通过qt的localsocket和localserver
 *实现程序的单实例运行
 *并实现浏览器插件调用客户端（带参数启动）的操作
 */

#include "download_mainwindow.h"
#include "addnew.h"
#include <QApplication>
#include <QFile>
#include <QTextCodec>
#include <QSharedMemory>
#include <QLocalSocket>
#include <QLocalServer>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QTranslator>
#include <QSettings>
#include <QDir>
#include <QByteArray>
#include "uilog.h"
extern "C"{
    #include "../include/apx_hftsc_api.h"
}
extern bool is_login;
extern int uid;
extern void apx_hftsc_exit();
int main(int argc, char *argv[])
{
    is_login = false;
    QApplication download_app(argc, argv);
    QString translator_FileName = "zh_CN.qm";
    QTranslator *translator = new QTranslator(&download_app);
    if(translator->load(translator_FileName))
        download_app.installTranslator(translator);
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QString serverName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    UIlog log;
    log.uilog_init();
    log.uilog_write("serverName:" + serverName + ".\n");
    QLocalSocket socket;
    log.uilog_write("connect to server.\n");
    socket.connectToServer(serverName);
    log.uilog_write("wait for connected.\n");
    QStringList args = QCoreApplication::arguments();

    if(socket.waitForConnected(2000)){
        qDebug()<<"socket connect to server successed,server name is :" + serverName;
        log.uilog_write("open textstream.\n");
        QTextStream stream(&socket);
        log.uilog_write("args.count: " + QString::number(args.count()) + "\n");

        if(args.count()==2){
            stream<<args.last();
            stream.flush();
            socket.waitForBytesWritten();
        }else if(args.count()==3){
            stream<<args[1]<<"|"<<args[2];
            stream.flush();
            socket.waitForBytesWritten();
        }
        else if(args.count()==4){
            stream<<args[1]<<"|"<<args[2]<<"|"<<args[3];
            stream.flush();
            socket.waitForBytesWritten();
        }

        log.uilog_write("quit.\n");
        download_app.quit();
        delete translator;
        return 1;
    }
    qDebug()<<"socket connect to server failed,server name is :" + serverName;
    if(args.count()==2&&args[1]=="quit"){
        qDebug()<<"quit command.\n";
        return 1;
    }
    log.uilog_write("open window.\n");

     QString user_path = QDir::homePath();
    QString conf_file_path = user_path + FILE_PATH + "/conf_file";
    QByteArray conf_file_array = conf_file_path.toLatin1();
    char *conf_file_c = conf_file_array.data();
    int init_result = apx_hftsc_init(conf_file_c);

    if(init_result != 0){
        log.uilog_write("apx_hftsc_init failed.\n");
        QMessageBox msgbox(QMessageBox::NoIcon,QString("??"),QString("????????"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("??"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        exit(-1);
    }

    QSettings settings1(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    bool auto_login = settings1.value("settings/auto_login",false).toBool();
    QString username = settings1.value("settings/username","").toString();
    QString password = settings1.value("settings/password","").toString();

    u32 ip;
    u16 port;

    if(apx_conf_serv_get(NULL,0,&ip,&port)!=0){
        log.uilog_write("apx_conf_serv_get failed.\n");
        QMessageBox msgbox(QMessageBox::NoIcon,QString("??"),QString("????????"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("??"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        uid = apx_user_login(0,0,NULL,NULL);

        if(uid<0){
            log.uilog_write("login failed.\n");
        }
        else
            log.uilog_write("login successed,uid is:" + QString::number(uid) + "\n");

    }

    else{

        if(auto_login){
            QByteArray username_array = username.toLatin1();
            QByteArray password_array = password.toLatin1();
            char *username_char = username_array.data();
            char *password_char = password_array.data();
            uid = apx_user_login(ip,port,username_char,password_char);

            if(uid>0){
                is_login = true;
                log.uilog_write("login successed,uid is:" + QString::number(uid) + "\n");
            }

        }

        else{
            uid = apx_user_login(ip,port,NULL,NULL);

            if(uid<0)
                log.uilog_write("login failed.\n");

            else
                log.uilog_write("login successed,uid is:" + QString::number(uid) + "\n");
        }

    }
    if(apx_task_restore(0)<0){
        log.uilog_write("restore task failed.\n");
    }
    download_MainWindow mainwindow;
    mainwindow.show();
    QFile styleFile(":/qss/skin.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        download_app.setStyleSheet(qss);
        styleFile.close();
    }

    download_app.exec();
    apx_hftsc_exit();
    log.uilog_write("exit from download,sava config.\n");
    delete translator;

    return 0;
}


