#ifndef ADDNEW_H
#define ADDNEW_H


#include <QDialog>
//#include <download_mainwindow.h>
namespace Ui {
class addnew;
}

class addnew : public QDialog
{
    Q_OBJECT

public:
    explicit addnew(QWidget *parent = 0);
    ~addnew();
    Ui::addnew *ui;
    void change_url();

public slots:

    bool eventFilter(QObject *, QEvent *);
    void closeEvent(QCloseEvent *event);
    int exec();

private slots:
    void on_pushButton_ok_clicked();

    void on_pushButton_cancel_clicked();

    void on_radioButton_http_clicked();

    void on_radioButton_bt_clicked();

    void on_pushButton_scan_bt_clicked();

    void on_pushButton_scan_clicked();

    void on_radioButton_ftp_clicked();

    void on_chk_offline_clicked();

    void on_chk_cloud_accelerate_clicked();

signals:
    void newConnection();

private:

};

#endif // ADDNEW_H
