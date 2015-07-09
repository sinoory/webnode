#ifndef NETWORK_THREAD_H
#define NETWORK_THREAD_H
#include <QThread>
#include <QString>

namespace Ui {
class network_thread;
}

class network_thread : public QThread
{
    Q_OBJECT
public:
    ~network_thread();
protected:
    void run();
signals:
  void  network_signal(QString);
  void network_cloud_signal(QString);
  void  network_stop_signal();
  void  network_stop_signal_cloud();
};
#endif
