#include "start_task_thread.h"
#include <QProcess>

extern QString open_fname;
start_task_thread::~start_task_thread()
{
}

void start_task_thread::run(){		//重写线程的run方法，用于打开下载完成的任务

    QProcess *check = new QProcess;
    int open_result = check->execute(open_fname);
    emit start_task_signal(open_result);
    delete check;

}
