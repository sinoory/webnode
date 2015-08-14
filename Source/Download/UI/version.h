#ifndef VERSION_H
#define VERSION_H

#include <QDialog>

namespace Ui {
class version;
}

class version : public QDialog
{
    Q_OBJECT

public:
    explicit version(QWidget *parent = 0);
    ~version();

private slots:
    void on_pushButton_clicked();

private:
    Ui::version *ui;
};

#endif // VERSION_H
