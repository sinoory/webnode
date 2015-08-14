#ifndef BT_LIST_H
#define BT_LIST_H

#include <QDialog>

namespace Ui {
class bt_list;
}

class bt_list : public QDialog
{
    Q_OBJECT

public:
    explicit bt_list(QWidget *parent = 0);
    ~bt_list();

private:
    Ui::bt_list *ui;
public slots:
    int exec();
private slots:
    void on_pushButton_clicked();
};

#endif // BT_LIST_H
