#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
extern "C"
{
        #include "../include/apx_hftsc_api.h"
}

namespace Ui {
class settings;
}

class settings : public QDialog
{
    Q_OBJECT

public:
    explicit settings(QWidget *parent = 0);
    ~settings();
    u32 ipToU32(QString ip);

private slots:
    void on_pushButton_scan_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_cancel_clicked();

private:
    Ui::settings *ui;
};

#endif // SETTINGS_H
