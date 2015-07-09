/*
 *显示程序的版本界面
 */

#include "version.h"
#include "ui_version.h"

version::version(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::version)
{
    setMaximumSize(339,201);
    setMinimumSize(339,201);
    ui->setupUi(this);
    ui->label_2->setPixmap(QPixmap(":/home/appex/image/logo.png"));
}

version::~version()
{
    delete ui;
}

void version::on_pushButton_clicked()
{
    this->close();
}
