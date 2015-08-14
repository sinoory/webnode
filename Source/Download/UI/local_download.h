#ifndef LOCAL_DOWNLOAD_H
#define LOCAL_DOWNLOAD_H

#include <QDialog>

namespace Ui {
class local_download;
}

class local_download : public QDialog
{
    Q_OBJECT

public:
    explicit local_download(QWidget *parent = 0);
    ~local_download();

    int exec();

private slots:
    void on_pushButton_overflow_clicked();

    void on_pushButton_cancel_clicked();

    void on_pushButton_ok_clicked();

private:
    Ui::local_download *ui;
};

#endif // LOCAL_DOWNLOAD_H
