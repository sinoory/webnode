#include "download_mainwindow.h"
#include "uilog.h"
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QProcess>
extern "C"{
    #include "unistd.h"
}
QFile outFile;
UIlog::UIlog()
{
    is_init =false;
}

void UIlog::uilog_init(){
    QString user_path = QDir::homePath();
    QDir *config_file = new QDir;
    QString file_pa = user_path + FILE_PATH;
    QByteArray file_pa_arry = file_pa.toLocal8Bit();
    char *file_pa_c = file_pa_arry.data();
    if(access(file_pa_c,0))
    {
        QProcess *p = new QProcess;
        p->execute("mkdir " + user_path + FILE_PATH);
        delete p;
    }

    outFile.setFileName(user_path + FILE_PATH + "/UIlog");

    if(outFile.open(QIODevice::WriteOnly |QIODevice::Append))
        is_init = true;

    else
        is_init = false;

    delete config_file;
}

void UIlog::uilog_write(QString log_info){

    if(is_init){
        QDateTime current_time = QDateTime::currentDateTime();
        QString current_date = current_time.toString("yyyy-MM-dd hh:mm:ss ddd");
        QTextStream ts(&outFile);
        ts<<current_date<<"      "<<log_info<<"\n";
        ts.flush();
    }

}

void UIlog::uilog_quit(){

    if(outFile.isOpen())
        outFile.close();

}
