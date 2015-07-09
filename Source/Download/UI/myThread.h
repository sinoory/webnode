#ifndef MYTHREAD_H
#define MYTHREAD_H
#include <QThread>
#include <QString>

namespace Ui {
class myThread;
}

class myThread : public QThread
{
    Q_OBJECT
public:
    ~myThread();
    void run();
signals:
  void  message1(int i);
  void s_addnew(int i);
  void close_mainwindow_signal();
  void hide_mainwindow_signal();
  void click_addnew_buttin_signal();
};


#endif // MYTHREAD_H
