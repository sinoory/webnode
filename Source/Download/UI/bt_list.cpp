/*
 *程序的bt文件列表界面
 *当选择bt下载时将会弹出该界面用于选择下载哪些bt种子中的文件
 */
#include "download_mainwindow.h"
#include "bt_list.h"
#include "ui_bt_list.h"
#include <QSettings>
#include <QDir>
extern "C"{
    #include "../include/apx_hftsc_api.h"
}
extern int task_id;
extern QString file_name;
QString user_path;
QTableWidgetItem *check;


bt_list::bt_list(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bt_list)		//初始化bt文件列表界面
{
    ui->setupUi(this);
    setMaximumSize(550,320);
    setMinimumSize(550,320);
    QStringList list;
    ui->tableWidget->setColumnCount(2);
    list<<""<<"文件名";
    ui->tableWidget->setHorizontalHeaderLabels(list);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->horizontalHeader()->resizeSection(0,25);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->verticalHeader()->setVisible(false);
}

bt_list::~bt_list()
{
    delete ui;
    delete check;
}

int bt_list::exec(){		//重写exec方法，将检测到的bt种子包含的文件显示在界面上
    struct btfile bt_file;
    apx_task_btfile_get(task_id,&bt_file);
    ui->tableWidget->insertRow(0);
    file_name = QString(bt_file.fn);
    user_path = QDir::homePath();
    QSettings settings2(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
    settings2.beginGroup(QString::number(task_id));
    settings2.setValue("download_name",file_name);
    settings2.endGroup();
    ui->tableWidget->setItem(0,1,new QTableWidgetItem(QString(bt_file.fn)));

    for(int j=0;j<bt_file.size;j++){
        int row = j + 1;
        ui->tableWidget->insertRow(row);
        check = new QTableWidgetItem();
        check->setCheckState(Qt::Unchecked);
        ui->tableWidget->setItem(row,0,check);
        ui->tableWidget->setItem(row,1,new QTableWidgetItem(QString(bt_file.file[j])));
    }
    /*for(int j=1;j<=bt_file.size;j++){
        ui->tableWidget->insertRow(j);
        check = new QTableWidgetItem();
        check->setCheckState(Qt::Unchecked);
        ui->tableWidget->setItem(j,0,check);
        QStringList btlist = QString(bt_file.file[j]).split("/");
        ui->tableWidget->setItem(j,1,new QTableWidgetItem(QString(btlist.last())));
    }*/
    return QDialog::exec();

}

void bt_list::on_pushButton_clicked()		//将选择下载的文件的参数传入到apx_task_btfile_selected
{
    QString select = "";
    int row_num = ui->tableWidget->rowCount();

    for(int i=1;i<row_num;i++){

        if(ui->tableWidget->item(i,0)->checkState() == Qt::Checked){

            if(select!="")
                select = select + "," + QString::number(i);

            else
                select = QString::number(i);

        }
    }
    QSettings settings(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
    settings.beginGroup(QString::number(task_id));
    settings.setValue("bt_select",select);
    settings.endGroup();
    QByteArray ba = select.toLatin1();
    char *bt_selected = ba.data();
    apx_task_btfile_selected(task_id,bt_selected);
    done(1);
}
