#ifndef CLOUD_H
#define CLOUD_H

#include <QDialog>

namespace Ui {
class cloud;
}

class cloud : public QDialog
{
    Q_OBJECT

public:
    explicit cloud(QWidget *parent = 0);
    ~cloud();
public slots:
    int exec();
private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::cloud *ui;
};

#endif // CLOUD_H
