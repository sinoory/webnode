#ifndef CLOUD_UPLOAD_H
#define CLOUD_UPLOAD_H

#include <QDialog>

namespace Ui {
class cloud_upload;
}

class cloud_upload : public QDialog
{
    Q_OBJECT

public:
    explicit cloud_upload(QWidget *parent = 0);
    ~cloud_upload();

private slots:
    void on_pushButton_3_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::cloud_upload *ui;
};

#endif // CLOUD_UPLOAD_H
