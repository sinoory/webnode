#ifndef CLOUD_ADDNEW_H
#define CLOUD_ADDNEW_H


#include <QDialog>
//#include <download_mainwindow.h>
namespace Ui {
class cloud_addnew;
}

class cloud_addnew : public QDialog
{
    Q_OBJECT

public:
    explicit cloud_addnew(QWidget *parent = 0);
    ~cloud_addnew();
    Ui::cloud_addnew *ui;
    void change_url();

public slots:

    bool eventFilter(QObject *, QEvent *);

private slots:
    void on_pushButton_2_clicked(bool checked);

    void on_pushButton_3_clicked();

    void on_radioButton_clicked();

    void on_radioButton_2_clicked();

    void on_pushButton_clicked();

    void on_pushButton_4_clicked();


    void on_lineEdit_textChanged(const QString &arg1);

    void on_radioButton_3_clicked();

signals:
    void newConnection();

private:
   // Ui::addnew *ui;
    //download_MainWindow mainui;
};

#endif // CLOUD_ADDNEW_H
