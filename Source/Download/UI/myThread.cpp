
#include "myThread.h"
#include <QApplication>
#include <QString>
#include <QProgressBar>
#include <QThread>
extern bool param_start;
extern bool run_download;
extern bool need_hide;
extern bool need_close;
extern bool need_click;

myThread::~myThread(){

}

void myThread::run(){		//为线程重写run方法，每隔一秒钟发一次信号通知主进程更新信息
    int j = 0;
    bool close_window = true;
    run_download = true;

    while(true){

        if(need_click){
            emit click_addnew_buttin_signal();
            need_click = false;
        }

        if(need_hide){
            emit hide_mainwindow_signal();
            need_hide = false;
        }

        if(need_close&&close_window){
            emit close_mainwindow_signal();
            need_close = false;
        }

         sleep(1);

         if(run_download){
            emit message1(j);
         }

         if(j>5)
             close_window = false;

         else
             j++;

    }

}
