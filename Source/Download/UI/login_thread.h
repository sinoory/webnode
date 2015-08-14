#ifndef LOGIN_THREAD_H
#define LOGIN_THREAD_H
#include <QThread>
#include <QString>

namespace Ui {
class login_thread;
}

class login_thread : public QThread
{
    Q_OBJECT
public:
    ~login_thread();
    void set_user_pass(QString user,QString pass);
protected:
    void run();
signals:
  void  login_signal(int);
};
#endif
