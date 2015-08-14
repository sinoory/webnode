#ifndef START_TASK_THREAD_H
#define START_TASK_THREAD_H

#include <QThread>
#include <QString>

namespace Ui {
class start_task_thread;
}

class start_task_thread : public QThread
{
    Q_OBJECT
public:
    ~start_task_thread();
    void run();
signals:
  void  start_task_signal(int);
};
#endif // START_TASK_THREAD_H
