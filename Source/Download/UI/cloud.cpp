#include "cloud.h"
#include "ui_cloud.h"
#include <QStringList>
#include "addnew.h"
#include "cloud_upload.h"


cloud::cloud(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cloud)
{
    ui->setupUi(this);
}

int cloud::exec(){
    QStringList list;
    ui->tableWidget->setColumnCount(3);
    list<<"文件名"<<"文件大小"<<"超期时间";
    ui->tableWidget->setHorizontalHeaderLabels(list);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_3->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_3->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_2->setColumnCount(7);
    QStringList list1;
    list1<<"文件名"<<"文件大小"<<"文件路径"<<"已下载"<<"速率"<<"剩余时间"<<"状态";
    ui->tableWidget_2->setHorizontalHeaderLabels(list1);
    ui->tableWidget_3->setColumnCount(7);
    QStringList list2;
    list2<<"文件名"<<"文件大小"<<"文件路径"<<"已上传"<<"速率"<<"剩余时间"<<"状态";
    ui->tableWidget_3->setHorizontalHeaderLabels(list2);
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tableWidget->horizontalHeader()->setHighlightSections(false);
    ui->tableWidget_2->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tableWidget_2->horizontalHeader()->setHighlightSections(false);
    ui->tableWidget_3->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tableWidget_3->horizontalHeader()->setHighlightSections(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->resizeSection(0,300);
    ui->tableWidget->horizontalHeader()->resizeSection(1,300);
    ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_2->horizontalHeader()->resizeSection(0,130);
    ui->tableWidget_2->horizontalHeader()->resizeSection(1,130);
    ui->tableWidget_2->horizontalHeader()->resizeSection(2,130);
    ui->tableWidget_2->horizontalHeader()->resizeSection(3,130);
    ui->tableWidget_2->horizontalHeader()->resizeSection(4,130);
    ui->tableWidget_2->horizontalHeader()->resizeSection(5,130);
    ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_3->horizontalHeader()->resizeSection(0,130);
    ui->tableWidget_3->horizontalHeader()->resizeSection(1,130);
    ui->tableWidget_3->horizontalHeader()->resizeSection(2,130);
    ui->tableWidget_3->horizontalHeader()->resizeSection(3,130);
    ui->tableWidget_3->horizontalHeader()->resizeSection(4,130);
    ui->tableWidget_3->horizontalHeader()->resizeSection(5,130);
    return QDialog::exec();
}

cloud::~cloud()
{
    delete ui;
}

void cloud::on_pushButton_2_clicked()
{
    cloud_upload upload1;
    upload1.exec();
}

void cloud::on_pushButton_clicked()
{
    addnew cloud_addnew1;
    cloud_addnew1.exec();
}
