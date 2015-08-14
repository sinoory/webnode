/*
 *  程序的主窗口
 *  主要处理任务的增加、删除、暂停、续传等事件
 *  同时也负责调用客户端的初始化、用户登陆和退出等功能
*/

#include "download_mainwindow.h"
#include "ui_download_mainwindow.h"
#include "uilog.h"
#include "myThread.h"
#include "login_thread.h"
#include <QTextCodec>
#include <QSpinBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QProgressBar>
#include <QThread>
#include <QAction>
#include <QMenu>
#include <QPoint>
#include <QTableWidgetItem>
#include <QContextMenuEvent>
#include <QTableWidgetItem>
#include <QCursor>
#include <QSettings>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFileInfo>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSystemTrayIcon>
extern "C"
{
        #include "../include/uci.h"
        #include "stdio.h"
        #include "../include/apx_hftsc_api.h"
        #include "../client/apx_proto_ctl.h"
}
#define MAIN_WINDOW_LENGTH 1070
#define MAIN_WINDOW_WIDTH 461

bool run_download;
static QString state;
static int row;
static int row_finish;
static int row_dustbin;
QString open_fname;
static QString user_path;
bool is_login;
int uid;
bool cancel_new;
bool param_start = false;
bool need_hide = false;
bool need_close = false;
bool need_click = false;
bool first_zero = false;
QString download_url;
QString file_path;
QString cloud_filename;
QString cloud_path;
QString fileId;
QString f_name;
QLocalServer *m_localServer;
QString file_name;
int task_id;
int check_uri;
myThread *mythread1;
start_task_thread *start_thread;
QProcess *proc;
QProcess *proc_ar;
network_thread *check_network;
login_thread *check_login;
UIlog log;
int check_type;
//QSystemTrayIcon *trayIcon;
//QMenu *trayIconMenu;


download_MainWindow::download_MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::download_MainWindow)
{
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    //初始化程序的主界面、初始化客户端管理模块
    //is_login = false;
    user_path = QDir::homePath();
    QDir *config_file = new QDir;
    QString file_pa = user_path + FILE_PATH;
    QByteArray file_pa_arry = file_pa.toLocal8Bit();
    char *file_pa_c = file_pa_arry.data();
    if(access(file_pa_c,0))
    {
        QProcess *p = new QProcess;
        p->execute("mkdir " + user_path + FILE_PATH);
    }

    delete config_file;

    QDir *default_file = new QDir;

    if(!default_file->exists(user_path + "/下载"))
    {
        default_file->mkdir(user_path + "/下载");
    }

    delete default_file;


    QSettings settings(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    int max_connection = settings.value("settings/max_connection",5).toInt();
    if(uid<0)
        uid=0;
    if(apx_user_limit_set(uid,0,0,max_connection)<0){
        QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("设置最大任务数失败！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
    }

    setMaximumSize(MAIN_WINDOW_LENGTH,MAIN_WINDOW_WIDTH);
    setMinimumSize(MAIN_WINDOW_LENGTH,MAIN_WINDOW_WIDTH);
    QString serverNames = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    log.uilog_write("mainWindown servername is : " + serverNames + "\n");
    qDebug()<<"create new server,server name is :"<<serverNames;
    m_localServer = new QLocalServer(this);
    download_url = "";
    connect(m_localServer,SIGNAL(newConnection()),this,SLOT(newLocalSocketConnection()));

    if(!m_localServer->listen(serverNames)){
        qDebug()<<"server error"<<m_localServer->serverError();
         //m_localServer->serverError();

         //if(m_localServer->serverError() == QAbstractSocket::AddressInUseError && QFile::exists(serverNames)){
         if(m_localServer->serverError() == QAbstractSocket::AddressInUseError ){
             qDebug()<<"remove server,restart server,server name is :" <<serverNames;
             QLocalServer::removeServer(serverNames);
             m_localServer->listen(serverNames);
         }

    }
    QTextCodec *codec = QTextCodec::codecForName("System");
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);

    ui->setupUi(this);

    QStringList list;
    ui->tbw_download->setColumnCount(8);
    list<<"文件名"<<"文件大小"<<"文件路径"<<"已下载"<<"速率"<<"剩余时间"<<"状态"<<"任务编号";
    ui->tbw_download->setHorizontalHeaderLabels(list);
    ui->tbw_finish->setColumnCount(5);
    QStringList list1;
    list1<<"文件名"<<"文件大小"<<"路径"<<"用时"<<"任务编号";
    ui->tbw_finish->setHorizontalHeaderLabels(list1);
    ui->tbw_dustbin->setColumnCount(6);
    QStringList list2;
    list2<<"文件名"<<"文件大小"<<"路径"<<"日期"<<"任务编号"<<"任务状态";
    ui->tbw_dustbin->setHorizontalHeaderLabels(list2);
    ui->tbw_download->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbw_download->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbw_finish->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbw_dustbin->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->txEdit_network_result->setDisabled(true);
    ui->txEdit_network_cloud_result->setDisabled(true);
    ui->tbw_download->setShowGrid(false);
    ui->tbw_download->horizontalHeader()->setStretchLastSection(true);
    ui->tbw_download->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tbw_download->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tbw_download->horizontalHeader()->setHighlightSections(false);
    ui->tbw_finish->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tbw_finish->horizontalHeader()->setHighlightSections(false);
    ui->tbw_dustbin->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tbw_dustbin->horizontalHeader()->setHighlightSections(false);
    QObject::connect(ui->tbw_download,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(show_contextMenu(const QPoint)));
    ui->btn_settings->setFocusPolicy(Qt::NoFocus);
    ui->btn_check->setFocusPolicy(Qt::NoFocus);
    ui->btn_addnew->setFocusPolicy(Qt::NoFocus);
    ui->btn_check_cloud->setFocusPolicy(Qt::NoFocus);
    ui->btn_download->setFocusPolicy(Qt::NoFocus);
    ui->btn_dustbin->setFocusPolicy(Qt::NoFocus);
    ui->btn_finish->setFocusPolicy(Qt::NoFocus);
    ui->btn_login->setFocusPolicy(Qt::NoFocus);
    ui->btn_network->setFocusPolicy(Qt::NoFocus);
    ui->btn_upload->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_cloud_check->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_download->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_filelists->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_upload->setFocusPolicy(Qt::NoFocus);
    ui->tbw_download->horizontalHeader()->resizeSection(0,135);
    ui->tbw_download->horizontalHeader()->resizeSection(1,135);
    ui->tbw_download->horizontalHeader()->resizeSection(2,135);
    ui->tbw_download->horizontalHeader()->resizeSection(3,135);
    ui->tbw_download->horizontalHeader()->resizeSection(4,135);
    ui->tbw_download->horizontalHeader()->resizeSection(5,135);
    ui->tbw_download->horizontalHeader()->resizeSection(6,135);
    //ui->tbw_download->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->tbw_download->setColumnHidden(7,true);
    read_all_config();

    QSettings settings2(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
    bool auto_start = settings2.value("settings/start_open").toBool();
    int download_row = ui->tbw_download->rowCount();
    for(int i=0;i<download_row;i++){
        int task_id1 = ui->tbw_download->item(i,7)->text().toInt();

        if(auto_start==false){

            if(apx_task_stop(task_id1)<0){
                log.uilog_write("stop task failed,task id is:" + QString::number(task_id1) + ".\n");
            }
        }
        else{

            if(apx_task_start(task_id1)<0){
                log.uilog_write("start task failed,task id is:" + QString::number(task_id1) + ".\n");
            }

        }

    }
    ui->tbw_finish->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbw_finish->setShowGrid(false);
    ui->tbw_finish->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tbw_finish->horizontalHeader()->setStretchLastSection(true);
    ui->tbw_finish->horizontalHeader()->resizeSection(0,230);
    ui->tbw_finish->horizontalHeader()->resizeSection(1,230);
    ui->tbw_finish->horizontalHeader()->resizeSection(2,230);
    ui->tbw_finish->horizontalHeader()->resizeSection(3,230);
    ui->tbw_finish->setColumnHidden(4,true);
    QObject::connect(ui->tbw_finish,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(show_contextMenu_finish(const QPoint)));
    ui->tbw_dustbin->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbw_dustbin->setShowGrid(false);
    ui->tbw_dustbin->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tbw_dustbin->horizontalHeader()->setStretchLastSection(true);
    ui->tbw_dustbin->horizontalHeader()->resizeSection(0,190);
    ui->tbw_dustbin->horizontalHeader()->resizeSection(1,190);
    ui->tbw_dustbin->horizontalHeader()->resizeSection(2,190);
    ui->tbw_dustbin->horizontalHeader()->resizeSection(3,190);
    ui->tbw_dustbin->horizontalHeader()->resizeSection(4,190);
    ui->tbw_dustbin->setColumnHidden(4,true);
    QObject::connect(ui->tbw_dustbin,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(show_contextMenu_dustbin(const QPoint)));
    ui->tbw_cloud_filelist->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->tbw_cloud_filelist,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(show_contextMenu_filelist(const QPoint)));
    ui->tbw_cloud_download->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->tbw_cloud_download,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(show_contextMenu_cloud_download(const QPoint)));
    ui->tbw_cloud_upload->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->tbw_cloud_upload,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(show_contextMenu_cloud_upload(const QPoint)));
    ui->tbw_download->verticalHeader()->setVisible(false);
    ui->tbw_finish->verticalHeader()->setVisible(false);
    ui->tbw_dustbin->verticalHeader()->setVisible(false);
    ui->tbw_cloud_download->verticalHeader()->setVisible(false);
    ui->tbw_cloud_filelist->verticalHeader()->setVisible(false);
    ui->tbw_cloud_upload->verticalHeader()->setVisible(false);
    int i = QApplication::argc();
    mythread1 = new myThread;
    QObject::connect(mythread1,SIGNAL(message1(int)),this,SLOT(hand_Message()),Qt::QueuedConnection);

    QStringList list3;
    ui->tbw_cloud_filelist->setColumnCount(5);
    list3<<"文件名"<<"文件大小"<<"创建时间"<<"任务编号"<<"task_id";
    ui->tbw_cloud_filelist->setHorizontalHeaderLabels(list3);
    ui->tbw_cloud_filelist->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbw_cloud_filelist->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbw_cloud_filelist->setShowGrid(false);
    ui->tbw_cloud_download->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbw_cloud_download->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbw_cloud_download->setShowGrid(false);
    ui->tbw_cloud_upload->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbw_cloud_upload->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbw_cloud_upload->setShowGrid(false);
    ui->tbw_cloud_download->setColumnCount(9);
    QStringList list4;
    list4<<"文件名"<<"文件大小"<<"文件路径"<<"已下载"<<"速率"<<"剩余时间"<<"状态"<<"任务编号"<<"fileId";
    ui->tbw_cloud_download->setHorizontalHeaderLabels(list4);
    ui->tbw_cloud_upload->setColumnCount(9);
    QStringList list5;
    list5<<"文件名"<<"文件大小"<<"文件路径"<<"已上传"<<"速率"<<"剩余时间"<<"状态"<<"任务编号"<<"fileId";
    ui->tbw_cloud_upload->setHorizontalHeaderLabels(list5);
    ui->tbw_cloud_filelist->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tbw_cloud_filelist->horizontalHeader()->setHighlightSections(false);
    ui->tbw_cloud_download->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tbw_cloud_download->horizontalHeader()->setHighlightSections(false);
    ui->tbw_cloud_upload->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    ui->tbw_cloud_upload->horizontalHeader()->setHighlightSections(false);
    ui->tbw_cloud_filelist->horizontalHeader()->setStretchLastSection(true);
    ui->tbw_cloud_filelist->horizontalHeader()->resizeSection(0,300);
    ui->tbw_cloud_filelist->horizontalHeader()->resizeSection(1,300);
    ui->tbw_cloud_filelist->horizontalHeader()->resizeSection(2,300);
    ui->tbw_cloud_filelist->setColumnHidden(3,true);
    ui->tbw_cloud_filelist->setColumnHidden(4,true);
    ui->tbw_cloud_download->horizontalHeader()->setStretchLastSection(true);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(0,135);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(1,135);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(2,135);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(3,135);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(4,135);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(5,135);
    ui->tbw_cloud_download->horizontalHeader()->resizeSection(6,135);
    ui->tbw_cloud_download->setColumnHidden(7,true);
    ui->tbw_cloud_download->setColumnHidden(8,true);
    ui->tbw_cloud_upload->horizontalHeader()->setStretchLastSection(true);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(0,135);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(1,135);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(2,135);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(3,135);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(4,135);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(5,135);
    ui->tbw_cloud_upload->horizontalHeader()->resizeSection(6,135);
    ui->tbw_cloud_upload->setColumnHidden(7,true);
    ui->tbw_cloud_upload->setColumnHidden(8,true);
    ui->btn_upload->hide();
    if(is_login){
        ui->label_username->hide();
        ui->label_password->hide();
        ui->username->hide();
        ui->password->hide();
        ui->btn_login->hide();
        ui->tbw_cloud_download->hide();
        ui->tbw_cloud_upload->hide();
        ui->btn_upload->show();
        QFile styleFile(":/qss/btn_cloud_filelist.qss");
        if(styleFile.open(QIODevice::ReadOnly)){
            QString qss = QLatin1String(styleFile.readAll());
            ui->tab_5->setStyleSheet(qss);
            styleFile.close();
        }
        get_cloud_list();
    }
    else{
        ui->tbw_cloud_filelist->hide();
        ui->tbw_cloud_download->hide();
        ui->tbw_cloud_upload->hide();
        ui->pushButton_filelists->hide();
        ui->pushButton_download->hide();
        ui->pushButton_upload->hide();
        ui->frame->hide();
        ui->password->setEchoMode(QLineEdit::Password);
    }
    ui->tbw_download->hide();
    ui->tbw_finish->hide();
    ui->tbw_dustbin->hide();
    ui->btn_check->hide();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_result->hide();
    ui->txEdit_network_cloud_result->hide();
    //ui->tabWidget->removeTab(1);
    ui->btn_download->click();
    QObject::connect(mythread1,SIGNAL(close_mainwindow_signal()),this,SLOT(close_mainwindow()));
    QObject::connect(mythread1,SIGNAL(hide_mainwindow_signal()),this,SLOT(hide()));
    QObject::connect(mythread1,SIGNAL(click_addnew_buttin_signal()),this,SLOT(click_addnew_buttin()));

    if(run_download==false){
        mythread1->start();
    }

    if (i>=2){
        need_click = true;
    }

    /*trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/home/appex/image/logo.png"));
    trayIcon->setToolTip("a trayicon example");
    trayIcon->show();
    //trayIcon->showMessage("download","download text",QSystemTrayIcon::Information,5000);
    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    action_show_window = new QAction(tr("主界面"),this);
    connect(action_show_window,SIGNAL(triggered()),this,SLOT(show_mainwindow()));
    action_quit = new QAction(tr("退出（&Q）"),this);
    connect(action_quit,SIGNAL(triggered()),this,SLOT(close_mainwindow()));
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(action_show_window);
    trayIconMenu->addAction(action_quit);
    trayIcon->setContextMenu(trayIconMenu);*/
}

download_MainWindow::~download_MainWindow()
{
    delete ui;
    delete m_localServer;
    delete start_thread;
    delete proc;
    delete proc_ar;
    delete check_network;
    //delete trayIcon;
}

/*void download_MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason){
    if(reason==QSystemTrayIcon::Trigger||reason==QSystemTrayIcon::DoubleClick){
        if(!download_MainWindow::isVisible())
            download_MainWindow::show();
        download_MainWindow::activateWindow();
    }
}*/

void download_MainWindow::closeEvent(QCloseEvent *event){
    event->ignore();
    download_MainWindow::hide();
    //write_all_config();
}

void download_MainWindow::show_contextMenu(const QPoint& pos){      //为正在下载界面增加右键菜单

    if(pop_menu){
        delete pop_menu;
        pop_menu = NULL;
    }

    QMenu *pop_menu = new QMenu();
    action_stop = new QAction(this);
    action_start = new QAction(this);
    action_delete = new QAction(this);
    action_stop->setText(QString("暂停"));
    action_start->setText(QString("续传"));
    action_delete->setText(QString("删除"));
    QTableWidgetItem *item = ui->tbw_download->itemAt(pos);

    if(item != NULL){
        row = item->row();
        pop_menu->addAction(action_stop);
        pop_menu->addAction(action_start);
        pop_menu->addAction(action_delete);
        QObject::connect(action_stop,SIGNAL(triggered(bool)),this,SLOT(stop_task()));
        QObject::connect(action_start,SIGNAL(triggered(bool)),this,SLOT(start_task()));
        QObject::connect(action_delete,SIGNAL(triggered(bool)),this,SLOT(delete_task()));
        pop_menu->exec(QCursor::pos());
    }

}

void download_MainWindow::stop_task(){      //正在下载界面暂停任务操作

        int total_row = ui->tbw_download->rowCount();

        for(int i = 0;i<total_row;i++){

            if(ui->tbw_download->item(i,0)->isSelected()){

                int task_id_download = ui->tbw_download->item(i,7)->text().toInt();
                /*int bp_result = apx_task_bpcontinue_get(task_id_download);
                if(bp_result==0){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),QString("任务编号为" + QString::number(task_id_download) + "的任务不支持断点续传，请勿暂停该任务！"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    continue;
                }*/
                int result = apx_task_stop(task_id_download);
                qDebug()<<"*apx_task_stop,result is: " <<result;
                if(result==0){
                    state = "暂停";
                    ui->tbw_download->setItem(i,6,new QTableWidgetItem(QString(state)));
                    //write_all_config();
                }

                else{
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("停止任务失败"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                }

            }

        }

}

void download_MainWindow::start_task(){         //正在下载界面续传任务操作

    int total_row = ui->tbw_download->rowCount();

    for(int i = 0;i<total_row;i++){

        if(ui->tbw_download->item(i,0)->isSelected()){

            int task_id_download = ui->tbw_download->item(i,7)->text().toInt();
            int result = apx_task_start(task_id_download);
            qDebug()<<"*apx_task_start,result is: " <<result;

            if(result==0){
                state = "启动";
                ui->tbw_download->setItem(i,6,new QTableWidgetItem(QString(state)));
                //write_all_config();
            }

            else{
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("启动任务失败"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
            }

        }

    }

}

void download_MainWindow::delete_task(){        //正在下载界面删除任务操作

    int total_row = ui->tbw_download->rowCount();

    for(int i = 0;i<total_row;i++){

        if(ui->tbw_download->item(i,0)->isSelected()){

            int task_id_download = ui->tbw_download->item(i,7)->text().toInt();
            QString name = ui->tbw_download->item(i,0)->text();
            QString size = ui->tbw_download->item(i,1)->text();
            QString path = ui->tbw_download->item(i,2)->text();
            QString state = "未完成";
            QString task_id = ui->tbw_download->item(i,7)->text();
            QDateTime current_date_time = QDateTime::currentDateTime();
            QString time = current_date_time.toString("yyyy-MM-dd hh:mm:ss ddd");
            /*int bp_result = apx_task_bpcontinue_get(task_id_download);
            if(bp_result==0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("提示"),QString("检测到存在不支持断点续传的任务,任务编号:" + QString::number(task_id_download)+  "，删除后将无法恢复，是否继续？"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/info.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                QPushButton *button_ok = msgbox.addButton(tr("是"),QMessageBox::AcceptRole);
                QPushButton *button_cancel = msgbox.addButton(tr("否"),QMessageBox::RejectRole);
                int msg_result = msgbox.exec();
                if(msgbox.clickedButton() == button_ok){

                }

                if(msgbox.clickedButton() == button_cancel || msg_result == 1){
                    continue;
                }
            }*/
            ui->tbw_dustbin->insertRow(0);
            ui->tbw_dustbin->setItem(0,0,new QTableWidgetItem(QString(name)));
            ui->tbw_dustbin->setItem(0,1,new QTableWidgetItem(QString(size)));
            ui->tbw_dustbin->setItem(0,2,new QTableWidgetItem(QString(path)));
            ui->tbw_dustbin->setItem(0,3,new QTableWidgetItem(QString(time)));
            ui->tbw_dustbin->setItem(0,4,new QTableWidgetItem(QString(task_id)));
            ui->tbw_dustbin->setItem(0,5,new QTableWidgetItem(QString(state)));
            ui->tbw_download->removeRow(i);
            //write_all_config();
            total_row--;
            i--;
            int result = apx_task_stop(task_id_download);

            if(result<0){
                log.uilog_write("stop task failed,task id is " + task_id_download);
                continue;
            }

            if(apx_task_delete(task_id_download)<0){
                log.uilog_write("delete task failed,task id is " + task_id_download);
            }

        }

    }

}

void download_MainWindow::show_contextMenu_finish(const QPoint& pos){       //为已下载界面增加右键菜单

    if(pop_menu){
        delete pop_menu;
        pop_menu = NULL;
    }

    QMenu *pop_menu = new QMenu();
    action_open = new QAction(this);
    action_open_dir = new QAction(this);
    action_delete_finish = new QAction(this);
    action_open->setText(QString("打开"));
    action_open_dir->setText(QString("打开目录"));
    action_delete_finish->setText(QString("删除"));
    QTableWidgetItem *item = ui->tbw_finish->itemAt(pos);

    if(item != NULL){
        row_finish = item->row();
        pop_menu->addAction(action_open);
        pop_menu->addAction(action_open_dir);
        pop_menu->addAction(action_delete_finish);
        QObject::connect(action_open,SIGNAL(triggered(bool)),this,SLOT(open_task()));
        QObject::connect(action_open_dir,SIGNAL(triggered(bool)),this,SLOT(open_task_dir()));
        QObject::connect(action_delete_finish,SIGNAL(triggered(bool)),this,SLOT(delete_finish_task()));
        pop_menu->exec(QCursor::pos());
    }

}

void download_MainWindow::open_task(){      //已下载界面打开任务操作
    QString name = ui->tbw_finish->item(row_finish,0)->text();
    QString path = ui->tbw_finish->item(row_finish,2)->text();
    open_fname = "gnome-open " + path + "/" + name + " &>/dev/null\n";
    start_thread = new start_task_thread;
    QObject::connect(start_thread,SIGNAL(start_task_signal(int)),this,SLOT(check_open_result(int)));
    start_thread->start();
}

void download_MainWindow::open_task_dir(){
    QString path = ui->tbw_finish->item(row_finish,2)->text();
    open_fname = "gnome-open " + path + " &>/dev/null\n";
    start_thread = new start_task_thread;
    QObject::connect(start_thread,SIGNAL(start_task_signal(int)),this,SLOT(check_open_result(int)));
    start_thread->start();
}

void download_MainWindow::check_open_result(int result){        //检查打开任务是否成功

    if(result!=0){
        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("打开任务失败"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
    }

}

void download_MainWindow::delete_finish_task(){     //已下载界面删除任务操作
    int total_row = ui->tbw_finish->rowCount();
    for(int i=0;i<total_row;i++){
        int delete_id = ui->tbw_finish->item(i,4)->text().toInt();

        if(ui->tbw_finish->item(i,0)->isSelected()){
            if(delete_id<=0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("任务不存在！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                return;
            }

            /*if(apx_task_delete(delete_id)<0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("删除任务失败！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                return;
            }*/

            QString name = ui->tbw_finish->item(i,0)->text();
            QString size = ui->tbw_finish->item(i,1)->text();
            QString path = ui->tbw_finish->item(i,2)->text();
            QString taskid = ui->tbw_finish->item(i,4)->text();
            QDateTime date_time = QDateTime::currentDateTime();
            QString time = date_time.toString("yyyy-MM-dd hh:mm:ss ddd");
            QString state = "已完成";
            ui->tbw_dustbin->insertRow(0);
            ui->tbw_dustbin->setItem(0,0,new QTableWidgetItem(name));
            ui->tbw_dustbin->setItem(0,1,new QTableWidgetItem(size));
            ui->tbw_dustbin->setItem(0,2,new QTableWidgetItem(path));
            ui->tbw_dustbin->setItem(0,3,new QTableWidgetItem(time));
            ui->tbw_dustbin->setItem(0,4,new QTableWidgetItem(taskid));
            ui->tbw_dustbin->setItem(0,5,new QTableWidgetItem(state));
            ui->tbw_finish->removeRow(i);
            //write_all_config();
            i--;
            total_row--;
        }
    }
    /*QMessageBox msgbox(QMessageBox::NoIcon,tr("提示"),tr("是否删除任务？"));
    msgbox.setIconPixmap(QPixmap(":/home/appex/image/info.png"));
    msgbox.setStyleSheet("QMessageBox{background-color:white}");
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    dynamic_cast<QGridLayout *>(msgbox.layout())->addLayout(pLayout,1,1);
    QCheckBox *pCheckBox = new QCheckBox(tr("同时删除文件"));
    pLayout->addWidget(pCheckBox);
    QPushButton *button_ok = msgbox.addButton(tr("是"),QMessageBox::AcceptRole);
    QPushButton *button_cancel = msgbox.addButton(tr("否"),QMessageBox::RejectRole);
    int msg_result = msgbox.exec();
    int total_row = ui->tbw_finish->rowCount();

    for(int i = 0;i<total_row;i++){
        int delete_id = ui->tbw_finish->item(i,4)->text().toInt();

        if(ui->tbw_finish->item(i,0)->isSelected()){

            if(delete_id<=0){
                QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("任务不存在！"));
                msgbox.setStandardButtons(QMessageBox::Ok);
                msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                msgbox.setStyleSheet("QMessageBox{background-color:white}");
                msgbox.exec();
                return;
            }

            if(msgbox.clickedButton() == button_ok){

                if(pCheckBox->isChecked()){

                    if(apx_task_destroy(delete_id)<0){
                         log.uilog_write("destory task failed,task id is " + delete_id);
                    }

                }

                else{

                    if(apx_task_release(delete_id)<0)
                        log.uilog_write("release task failed.task id is" + delete_id);

                }

            }

            if(msgbox.clickedButton() == button_cancel || msg_result == 1){
               return;
            }

            ui->tbw_finish->removeRow(i);
            write_all_config();
            total_row--;
            i--;
        }

    }

    delete pCheckBox;*/

}

void download_MainWindow::show_contextMenu_dustbin(const QPoint& pos){		//为垃圾箱界面增加右键菜单

    if(pop_menu){
        delete pop_menu;
        pop_menu = NULL;
    }

    QMenu *pop_menu = new QMenu();
    action_recover = new QAction(this);
    action_delete_dustbin = new QAction(this);
    action_recover->setText(QString("恢复"));
    action_delete_dustbin->setText(QString("删除"));
    QTableWidgetItem *item = ui->tbw_dustbin->itemAt(pos);

    if(item != NULL){
        row_dustbin = item->row();
        pop_menu->addAction(action_recover);
        pop_menu->addAction(action_delete_dustbin);
        QObject::connect(action_recover,SIGNAL(triggered(bool)),this,SLOT(recover_task()));
        QObject::connect(action_delete_dustbin,SIGNAL(triggered(bool)),this,SLOT(delete_dustbin_task()));
        pop_menu->exec(QCursor::pos());
    }

}

void download_MainWindow::recover_task(){		//垃圾箱界面恢复任务操作
    int row_count = ui->tbw_dustbin->rowCount();
    for(int i = 0;i<row_count;i++){
        if(ui->tbw_dustbin->item(i,0)->isSelected()){
            QString name = ui->tbw_dustbin->item(i,0)->text();
            QString size = ui->tbw_dustbin->item(i,1)->text();
            QString path = ui->tbw_dustbin->item(i,2)->text();
            QString file = path + "/" + name;
            int task_id = ui->tbw_dustbin->item(i,4)->text().toInt();
            QString state = ui->tbw_dustbin->item(i,5)->text();

            if (state == "未完成"){
                /*int bp_result = apx_task_bpcontinue_get(task_id);
                if(bp_result==0){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("该任务不支持断点续传，无法恢复，请重新下载"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    return;
                }*/
                int res = apx_task_recover(task_id);
                if(res<0){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("恢复任务失败"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    return;
                }

                if(apx_task_start(task_id)<0){
                    QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("重启任务失败"));
                    msgbox.setStandardButtons(QMessageBox::Ok);
                    msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                    msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                    msgbox.setStyleSheet("QMessageBox{background-color:white}");
                    msgbox.exec();
                    return;
                }

                ui->tbw_dustbin->removeRow(i);
                QString rate = "0.0B/S";
                QString average_rate = "0.0B/S";
                QString state = "暂停";
                ui->tbw_download->insertRow(0);
                ui->tbw_download->setItem(0,0,new QTableWidgetItem(QString(name)));
                ui->tbw_download->setItem(0,1,new QTableWidgetItem(QString(size)));
                ui->tbw_download->setItem(0,2,new QTableWidgetItem(QString(path)));
                ui->tbw_download->setItem(0,4,new QTableWidgetItem(QString(rate)));
                ui->tbw_download->setItem(0,5,new QTableWidgetItem(QString(average_rate)));
                ui->tbw_download->setItem(0,6,new QTableWidgetItem(QString(state)));
                ui->tbw_download->setItem(0,7,new QTableWidgetItem(QString::number(task_id,10)));
                ProgressBar = new QProgressBar;
                ProgressBar->setRange(0,100);
                int finish_size =0;
                ProgressBar->setValue(finish_size);
                ui->tbw_download->setCellWidget(0,3,ProgressBar);
                //write_all_config();

            }

            else if(state == "已完成"){

                int id = ui->tbw_dustbin->item(i,4)->text().toInt();

                if(QFile::exists(file)){
                    time_t time;

                    /*if(apx_task_recover(id)<0){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("恢复任务失败！"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();
                    }*/
                    apx_task_time_get(id,NULL,NULL,NULL,NULL,&time);
                    QString finish_time = QString::number(time,'f',1) + "s";
                    ui->tbw_dustbin->removeRow(i);
                    ui->tbw_finish->insertRow(0);
                    ui->tbw_finish->setItem(0,0,new QTableWidgetItem(QString(name)));
                    ui->tbw_finish->setItem(0,1,new QTableWidgetItem(QString(size)));
                    ui->tbw_finish->setItem(0,2,new QTableWidgetItem(QString(path)));
                    ui->tbw_finish->setItem(0,3,new QTableWidgetItem(QString(finish_time)));
                    ui->tbw_finish->setItem(0,4,new QTableWidgetItem(QString::number(task_id)));
                    //write_all_config();
                }
                else{
                    if(apx_task_release(id)<0)
                        log.uilog_write("release file failed when recover file.\n");
                    QSettings settings1(user_path + FILE_PATH + "/settings.ini",QSettings::IniFormat);
                    unsigned int one_max_thread = settings1.value("settings/one_max_thread",5).toInt();
                    unsigned int download = settings1.value("settings/download",0).toInt();
                    unsigned int upload = settings1.value("settings/upload",0).toInt();
                    QSettings settings(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
                    QString url = settings.value(QString::number(task_id) + "/url").toString();
                    unsigned char task_type = settings.value(QString::number(task_id) + "/task_type").toInt();
                    unsigned char concurr = settings.value(QString::number(task_id) + "/concurr").toInt();
                    QString path = settings.value(QString::number(task_id) + "/download_path").toString();
                    QString name = settings.value(QString::number(task_id) + "/download_name").toString();
                    QString bt_select = settings.value(QString::number(task_id) + "/bt_select","0").toString();

                    QByteArray url_array = url.toLocal8Bit();
                    char *addnew_url = url_array.data();
                    QByteArray path_array = path.toLocal8Bit();
                    char *download_path = path_array.data();
                    QByteArray name_array = name.toLocal8Bit();
                    char *download_name = name_array.data();
                    struct apx_trans_opt addnew_opt;
                    memset(&addnew_opt,0,sizeof(struct apx_trans_opt));
                    addnew_opt.concurr = one_max_thread;
                    addnew_opt.type = task_type;
                    strncpy(addnew_opt.uri,addnew_url,sizeof(addnew_opt.uri)-1);
                    strncpy(addnew_opt.fname,download_name,sizeof(addnew_opt.fname)-1);
                    strncpy(addnew_opt.fpath,download_path,sizeof(addnew_opt.fpath)-1);
                    addnew_opt.up_splimit = upload;
                    addnew_opt.down_splimit = download;
                    task_id  = apx_task_create(&addnew_opt);

                    if(task_id==-1){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("uid不存在"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();

                        return;
                    }

                    if(task_id==-2){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("未知的协议"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();

                        return;
                    }

                    if(task_id==-3){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("获取下载文件大小失败"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();

                        return;
                    }

                    if(task_id==-4){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("申请内存失败"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();

                        return;
                    }

                    if(bt_select=="0"){
                        QSettings settings2(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
                        settings2.beginGroup(QString::number(task_id));
                        settings2.setValue("url",addnew_url);
                        settings2.setValue("task_type",task_type);
                        settings2.setValue("concurr",concurr);
                        settings2.setValue("download_path",download_path);
                        settings2.setValue("download_name",download_name);
                        settings2.endGroup();
                    }
                    else{
                        QByteArray ba = bt_select.toLatin1();
                        char *bt_selected = ba.data();
                        apx_task_btfile_selected(task_id,bt_selected);
                        QSettings settings2(user_path + FILE_PATH + "/taskinfo.ini",QSettings::IniFormat);
                        settings2.beginGroup(QString::number(task_id));
                        settings2.setValue("url",addnew_url);
                        settings2.setValue("task_type",task_type);
                        settings2.setValue("concurr",concurr);
                        settings2.setValue("download_path",download_path);
                        settings2.setValue("download_name",download_name);
                        settings2.setValue("bt_select",bt_select);
                        settings2.endGroup();
                    }

                    if(apx_task_start(task_id)<0){
                        QMessageBox msgbox(QMessageBox::NoIcon,tr("错误"),tr("启动任务失败"));
                        msgbox.setStandardButtons(QMessageBox::Ok);
                        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
                        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                        msgbox.setStyleSheet("QMessageBox{background-color:white}");
                        msgbox.exec();
                    }

                    ui->tbw_dustbin->removeRow(i);
                    QString rate = "0.0B/S";
                    QString average_rate = "0.0B/S";
                    QString state = "暂停";
                    ui->tbw_download->insertRow(0);
                    ui->tbw_download->setItem(0,0,new QTableWidgetItem(QString(name)));
                    ui->tbw_download->setItem(0,1,new QTableWidgetItem(QString(size)));
                    ui->tbw_download->setItem(0,2,new QTableWidgetItem(QString(path)));
                    ui->tbw_download->setItem(0,4,new QTableWidgetItem(QString(rate)));
                    ui->tbw_download->setItem(0,5,new QTableWidgetItem(QString(average_rate)));
                    ui->tbw_download->setItem(0,6,new QTableWidgetItem(QString(state)));
                    ui->tbw_download->setItem(0,7,new QTableWidgetItem(QString::number(task_id,10)));
                    ProgressBar = new QProgressBar;
                    ProgressBar->setRange(0,100);
                    int finish_size =0;
                    ProgressBar->setValue(finish_size);
                    ui->tbw_download->setCellWidget(0,3,ProgressBar);
                    //write_all_config();

                }
            }
            row_count--;
            i--;
        }
    }

}

void download_MainWindow::delete_dustbin_task(){		//垃圾箱界面删除任务操作
     int check_code = 0;
     int total_row = ui->tbw_dustbin->rowCount();
     for(int i = 0;i<total_row;i++){
         int delete_id = ui->tbw_dustbin->item(i,4)->text().toInt();
         QString state = ui->tbw_dustbin->item(i,5)->text();

         if(ui->tbw_dustbin->item(i,0)->isSelected()){

             if(delete_id<=0){
                 QMessageBox msgbox1(QMessageBox::NoIcon,tr("错误"),tr("任务不存在！"));
                 msgbox1.setStandardButtons(QMessageBox::Ok);
                 msgbox1.setButtonText(QMessageBox::Ok,QString("确定"));
                 msgbox1.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
                 msgbox1.setStyleSheet("QMessageBox{background-color:white}");
                 msgbox1.exec();
                 return;
             }

             if(state=="未完成"){
                 int res = apx_task_destroy(delete_id);
                 if(res<0){
                      log.uilog_write("destory task failed,task id is " + delete_id);
                 }
             }
             else{
                 QMessageBox msgbox(QMessageBox::NoIcon,tr("提示"),tr("是否删除任务？"));
                 msgbox.setIconPixmap(QPixmap(":/home/appex/image/info.png"));
                 msgbox.setStyleSheet("QMessageBox{background-color:white}");
                 QVBoxLayout *pLayout = new QVBoxLayout(this);
                 dynamic_cast<QGridLayout *>(msgbox.layout())->addLayout(pLayout,1,1);
                 QCheckBox *pCheckBox = new QCheckBox(tr("同时删除文件"));
                 pCheckBox->setChecked(true);
                 pLayout->addWidget(pCheckBox);
                 QPushButton *button_ok = msgbox.addButton(tr("是"),QMessageBox::AcceptRole);
                 QPushButton *button_cancel = msgbox.addButton(tr("否"),QMessageBox::RejectRole);
                 if(check_code==0){
                     int msg_result = msgbox.exec();
                     if(msgbox.clickedButton() == button_cancel || msg_result == 1){
                        return;
                     }
                 }

                 if(msgbox.clickedButton()==button_ok)
                     check_code=1;

                 else
                     check_code=-1;

                 if(check_code==1){

                     if(pCheckBox->isChecked()){

                         int res = apx_task_destroy(delete_id);

                         if(res<0){
                              log.uilog_write("destory task failed,task id is " + delete_id);
                         }

                     }

                     else if(check_code==-1){

                         if(apx_task_release(delete_id)<0)
                             log.uilog_write("release task failed.task id is" + delete_id);

                     }
                     delete pCheckBox;

                 }

             }
             ui->tbw_dustbin->removeRow(i);
             //write_all_config();
             total_row--;
             i--;
         }

     }



}

void download_MainWindow::show_contextMenu_filelist(const QPoint& pos){      //为文件列表界面增加右键菜单

    if(pop_menu){
        delete pop_menu;
        pop_menu = NULL;
    }

    QMenu *pop_menu = new QMenu();
    action_down_to_local = new QAction(this);
    action_filelist_delete = new QAction(this);
    action_down_to_local->setText(QString("下载到本地"));
    action_filelist_delete->setText(QString("删除"));
    QTableWidgetItem *item = ui->tbw_cloud_filelist->itemAt(pos);

    if(item != NULL){
        row = item->row();
        pop_menu->addAction(action_down_to_local);
        pop_menu->addAction(action_filelist_delete);
        QObject::connect(action_down_to_local,SIGNAL(triggered(bool)),this,SLOT(download_to_local()));
        QObject::connect(action_filelist_delete,SIGNAL(triggered(bool)),this,SLOT(delete_filelist()));
        pop_menu->exec(QCursor::pos());
    }

}

void download_MainWindow::download_to_local(){
    int row_filelist = ui->tbw_cloud_filelist->rowCount();
    for(int i = 0;i<row_filelist;i++){
        if(ui->tbw_cloud_filelist->item(i,0)->isSelected()){
            f_name = ui->tbw_cloud_filelist->item(i,0)->text();
            fileId = ui->tbw_cloud_filelist->item(i,4)->text();
        }
    }
    int result = local_download1.exec();
    if(result==0)
        return;

    if(result==2)
        remove_reset_row(check_uri);
    int row1 = ui->tbw_download->rowCount();
    QString s_file_size = "0KB";
    QString rate = "0.0KB/S";
    QString average_rate = "00 : 00 : 00";
    QString state = "等待";
    ui->tbw_download->insertRow(row1);
    ui->tbw_download->setItem(row1,0,new QTableWidgetItem(QString(file_name)));
    ui->tbw_download->setItem(row1,1,new QTableWidgetItem(QString(s_file_size)));
    ui->tbw_download->setItem(row1,2,new QTableWidgetItem(QString(file_path)));
    ui->tbw_download->setItem(row1,4,new QTableWidgetItem(QString(rate)));
    ui->tbw_download->setItem(row1,5,new QTableWidgetItem(QString(average_rate)));
    ui->tbw_download->setItem(row1,6,new QTableWidgetItem(QString(state)));
    ui->tbw_download->setItem(row1,7,new QTableWidgetItem(QString::number(task_id,10)));
    int finish_size = 0;
    ProgressBar = new QProgressBar;
    ProgressBar->setRange(0,100);
    ProgressBar->setValue(finish_size);
    ui->tbw_download->setCellWidget(row1,3,ProgressBar);

}

void download_MainWindow::delete_filelist(){
    int row_count = ui->tbw_cloud_filelist->rowCount();
    for(int i = 0;i<row_count;i++){
        if(ui->tbw_cloud_filelist->item(i,0)->isSelected()){
            QString file_id = ui->tbw_cloud_filelist->item(i,4)->text();
            QByteArray file_id_array = file_id.toLocal8Bit();
            char *file_id_c = file_id_array.data();
            apx_cloud_task_del(file_id_c);
            ui->tbw_cloud_filelist->removeRow(i);
            i--;
            row_count--;
        }
    }
}

void download_MainWindow::show_contextMenu_cloud_download(const QPoint& pos){      //为文件列表界面增加右键菜单

    if(pop_menu){
        delete pop_menu;
        pop_menu = NULL;
    }

    QMenu *pop_menu = new QMenu();
    action_cloud_download_start = new QAction(this);
    action_cloud_download_stop = new QAction(this);
    action_cloud_download_delete = new QAction(this);
    action_cloud_download_start->setText(QString("续传"));
    action_cloud_download_stop->setText(QString("暂停"));
    action_cloud_download_delete->setText(QString("删除"));
    QTableWidgetItem *item = ui->tbw_cloud_download->itemAt(pos);

    if(item != NULL){
        row = item->row();
        pop_menu->addAction(action_cloud_download_stop);
        pop_menu->addAction(action_cloud_download_start);
        pop_menu->addAction(action_cloud_download_delete);
        QObject::connect(action_cloud_download_stop,SIGNAL(triggered(bool)),this,SLOT(stop_cloud_download_task()));
        QObject::connect(action_cloud_download_start,SIGNAL(triggered(bool)),this,SLOT(start_cloud_download_task()));
        QObject::connect(action_cloud_download_delete,SIGNAL(triggered(bool)),this,SLOT(delete_cloud_download_task()));
        pop_menu->exec(QCursor::pos());
    }

}

void download_MainWindow::stop_cloud_download_task(){
    int row_count = ui->tbw_cloud_download->rowCount();
    for(int i=0;i<row_count;i++){
        if(ui->tbw_cloud_download->item(i,0)->isSelected()){
            QString file_id = ui->tbw_cloud_download->item(i,8)->text();
            QByteArray file_id_array = file_id.toLocal8Bit();
            char *file_id_c = file_id_array.data();
            cld_list_st info;
            memset(&info,0,sizeof(cld_list_st));
            if(apx_cloud_task_stop(file_id_c,&info)<0)
                log.uilog_write("stop task failed,task id is");
            else{
                int state = info.u.taskinfo.status;
                if(state==3){
                    QString s_rate = "0 B/S";
                    QString string_rest_time = "00 : 00 : 00";
                    QString s_state = "暂停";
                    ui->tbw_cloud_download->setItem(i,4,new QTableWidgetItem(QString(s_rate)));
                    ui->tbw_cloud_download->setItem(i,5,new QTableWidgetItem(QString(string_rest_time)));
                    ui->tbw_cloud_download->setItem(i,6,new QTableWidgetItem(QString(s_state)));
                }
            }
        }
    }
}

void download_MainWindow::start_cloud_download_task(){
    int row_count = ui->tbw_cloud_download->rowCount();
    for(int i=0;i<row_count;i++){
        if(ui->tbw_cloud_download->item(i,0)->isSelected()){
            int taskid = ui->tbw_cloud_download->item(i,7)->text().toInt();
            QString file_id = ui->tbw_cloud_download->item(i,8)->text();
            QByteArray file_id_array = file_id.toLocal8Bit();
            char *file_id_c = file_id_array.data();
            if(apx_cloud_task_start(file_id_c)<0)
                log.uilog_write("start task failed,task id is:" + QString::number(taskid));
        }
    }
}

void download_MainWindow::delete_cloud_download_task(){
    int row_count = ui->tbw_cloud_download->rowCount();
    for(int i = 0;i<row_count;i++){
        if(ui->tbw_cloud_download->item(i,0)->isSelected()){
            /*QString file_id = ui->tbw_cloud_download->item(i,8)->text();
            QByteArray file_id_array = file_id.toLocal8Bit();
            char *file_id_c = file_id_array.data();
            int http_code = 0;
            int task_status = 0;
            if(apx_cloud_task_del(file_id_c,&http_code,&task_status)<0)
                log.uilog_write("delete task failed,task id is:" + QString::number(taskid));*/
            QString file_id = ui->tbw_cloud_download->item(i,8)->text();
            QByteArray array_file_id = file_id.toLocal8Bit();
            char *file_id_c = array_file_id.data();
            if(apx_cloud_task_del(file_id_c)<0)
                log.uilog_write("delete task failed,task id is:" );
            ui->tbw_cloud_download->removeRow(i);
            i--;
            row_count--;
        }
    }
}

void download_MainWindow::show_contextMenu_cloud_upload(const QPoint& pos){      //为文件列表界面增加右键菜单

    if(pop_menu){
        delete pop_menu;
        pop_menu = NULL;
    }

    QMenu *pop_menu = new QMenu();
    action_cloud_upload_start = new QAction(this);
    action_cloud_upload_stop = new QAction(this);
    action_cloud_upload_delete = new QAction(this);
    action_cloud_upload_start->setText(QString("续传"));
    action_cloud_upload_stop->setText(QString("暂停"));
    action_cloud_upload_delete->setText(QString("删除"));
    QTableWidgetItem *item = ui->tbw_cloud_upload->itemAt(pos);

    if(item != NULL){
        row = item->row();
        pop_menu->addAction(action_cloud_upload_stop);
        pop_menu->addAction(action_cloud_upload_start);
        pop_menu->addAction(action_cloud_upload_delete);
        QObject::connect(action_cloud_upload_stop,SIGNAL(triggered(bool)),this,SLOT(stop_cloud_upload_task()));
        QObject::connect(action_cloud_upload_start,SIGNAL(triggered(bool)),this,SLOT(start_cloud_upload_task()));
        QObject::connect(action_cloud_upload_delete,SIGNAL(triggered(bool)),this,SLOT(delete_cloud_upload_task()));
        pop_menu->exec(QCursor::pos());
    }

}

void download_MainWindow::stop_cloud_upload_task(){
    int row_count = ui->tbw_cloud_upload->rowCount();
    for(int i=0;i<row_count;i++){
        int taskid = ui->tbw_cloud_upload->item(i,7)->text().toInt();
        if(ui->tbw_cloud_upload->item(i,0)->isSelected()){
            apx_task_stop(taskid);
        }
    }
}

void download_MainWindow::start_cloud_upload_task(){
    int row_count = ui->tbw_cloud_upload->rowCount();
    for(int i=0;i<row_count;i++){
        int taskid = ui->tbw_cloud_upload->item(i,7)->text().toInt();
        if(ui->tbw_cloud_upload->item(i,0)->isSelected()){
            apx_task_start(taskid);
        }
    }
}

void download_MainWindow::delete_cloud_upload_task(){
    int row_count = ui->tbw_cloud_upload->rowCount();
    for(int i = 0;i<row_count;i++){
        int taskid = ui->tbw_cloud_upload->item(i,7)->text().toInt();
        if(ui->tbw_cloud_upload->item(i,0)->isSelected()){
            apx_task_release(taskid);
            ui->tbw_cloud_upload->removeRow(i);
            i--;
            row_count--;
        }
    }
}

void download_MainWindow::hand_Message(){	//接收线程发出的信号，更新主界面上的信息

    QString name = "";
    time_t during_time;
    QString size = "0KB";
    unsigned int rate;
    double average_rate;
    int row1 = ui->tbw_download->rowCount()-1;

    for(int k=0;k<=row1;k++){
        QProgressBar *ProgressBar;
        ProgressBar = (QProgressBar *)ui->tbw_download->cellWidget(k,3);
        int finish_size = ProgressBar->value();
        int task_id_download = ui->tbw_download->item(k,7)->text().toInt();
        int file_state = apx_task_state_get(task_id_download);
        QString state1 = ui->tbw_download->item(k,6)->text();
        if(file_state == APX_TASK_STATE_ACTIVE){
            state1 = "启动";
        }

        else if(file_state == APX_TASK_STATE_STOP){
            state1 = "暂停";
        }

        else if(file_state == APX_TASK_STATE_WAIT){
            state1 = "等待";
        }

        else if (file_state == APX_TASK_STATE_FINISHED){
            state1 = "完成";
        }
        ui->tbw_download->setItem(k,6,new QTableWidgetItem(QString(state1)));
        //qDebug()<<"task id is :" <<task_id_download<<", task state is:"<<file_state;
        if(finish_size<100 && file_state!=APX_TASK_STATE_FINISHED){
            int file_speed_res = apx_task_speed_get(task_id_download,&rate,NULL);

            unsigned long long total_size;
            unsigned long long local_size;
            int file_size_res = apx_task_file_size_get(task_id_download,&total_size,&local_size,NULL);
            //qDebug()<<"total_size is: " <<total_size<<"local_size is: "<<local_size;
            int residue_size = total_size-local_size;
            int i_time = 0;
            QString time = "00 : 00 : 00";
            if(rate>0){
                i_time = residue_size/rate;
                time = time_from_int(i_time);
                first_zero = true;
            }
            else if(first_zero){
                first_zero = false;
                return;
            }
            if(time=="00 : 00 : 00"&&residue_size!=0&&rate>0)
                time = "未知";
            int file_time_res = apx_task_time_get(task_id_download,NULL,NULL,NULL,NULL,&during_time);

            if(during_time==0){
                average_rate=0;
            }

            else{
                average_rate = (double)local_size/during_time;
            }

            QString s_average;

            if(average_rate>=1048576){
                average_rate = average_rate/1048576;
                s_average = QString::number(average_rate,'f',1) + "MB/S";
            }

            else if(average_rate<1048576 && average_rate>=1024){
                average_rate = average_rate/1024;
                s_average = QString::number(average_rate,'f',1) + "KB/S";
            }

            else if(average_rate<1024 && average_rate>=0){
                s_average = QString::number(average_rate,'f',1) + "B/S";
            }

        if(file_state == APX_TASK_STATE_ACTIVE){
            state1 = "启动";
        }

        else if(file_state == APX_TASK_STATE_STOP){
            state1 = "暂停";
        }

        else if(file_state == APX_TASK_STATE_WAIT){
            state1 = "等待";
        }

        else if (file_state == APX_TASK_STATE_FINISHED){
            state1 = "完成";
        }

        if(state1!="暂停"){

            if(local_size == 0 || total_size == 0){
                finish_size = 0;
            }

            else{
                finish_size = (local_size*100)/total_size;
            }

        }

        double d_rate = (double)rate;
        QString s_rate;

        if(d_rate>=1048576){
            d_rate = d_rate/1048576;
             s_rate = QString::number(d_rate,'f',1) + "MB/S";
        }

        else if(d_rate<1048576 && d_rate>=1024){
            d_rate = d_rate/1024;
             s_rate = QString::number(d_rate,'f',1) + "KB/S";
        }

        else if(d_rate>=0 && d_rate<1024){
             s_rate = QString::number(d_rate,'f',1) + "B/S";
        }

        double file_size = (double)total_size;
        QString s_file_size;

        if(file_size>=1073741824){
            file_size = file_size/1073741824;
            s_file_size = QString::number(file_size,'f',1) + "GB";
        }

        else if(file_size<1073741824 && file_size>=1048576){
            file_size = file_size/1048576;
            s_file_size = QString::number(file_size,'f',1) + "MB";
        }

        else if(file_size<1048576&&file_size>=1024){
            file_size =file_size/1024;
            s_file_size = QString::number(file_size,'f',1) + "KB";
        }

        else if(file_size<1024 &&file_size>=0){
            s_file_size = QString::number(file_size,'f',1) + "B";
        }

        if(file_size_res < 0){
            s_file_size = "未知";
            s_average = "未知";
        }

        if(file_time_res < 0){
            s_average = "未知";
        }

        if(file_speed_res < 0){
            s_rate = "未知";
        }

        ui->tbw_download->setItem(k,1,new QTableWidgetItem(QString(s_file_size)));
        ui->tbw_download->setItem(k,4,new QTableWidgetItem(QString(s_rate)));
        ui->tbw_download->setItem(k,5,new QTableWidgetItem(QString(time)));

        if(finish_size<0||finish_size>100)
            finish_size=0;
        ProgressBar = new QProgressBar;
        ProgressBar->setRange(0,100);
        ProgressBar->setValue(finish_size);
        ui->tbw_download->setCellWidget(k,3,ProgressBar);
        }

        if(finish_size==100 ||file_state==APX_TASK_STATE_FINISHED ){
             name = ui->tbw_download->item(k,0)->text();
             size = ui->tbw_download->item(k,1)->text();
             QString path = ui->tbw_download->item(k,2)->text();
             QString taskid = ui->tbw_download->item(k,7)->text();
             int id = taskid.toInt();
             time_t time;
             apx_task_time_get(id,NULL,NULL,NULL,NULL,&time);
             if(time>10000000||time<0)
                 time=1;
             unsigned long long  total_size;
             apx_task_file_size_get(id,&total_size,NULL,NULL);
             if(total_size>=1073741824){
                 total_size = total_size/1073741824;
                 size = QString::number(total_size,'f',1) + "GB";
             }

             else if(total_size<1073741824 && total_size>=1048576){
                 total_size = total_size/1048576;
                 size = QString::number(total_size,'f',1) + "MB";
             }

             else if(total_size<1048576&&total_size>=1024){
                 total_size =total_size/1024;
                 size = QString::number(total_size,'f',1) + "KB";
             }

             else if(total_size<1024 ){
                 size = QString::number(total_size,'f',1) + "B";
             }
             QString finish_time = QString::number(time,'f',1) + "s";
             ui->tbw_finish->insertRow(0);
             ui->tbw_finish->setItem(0,0,new QTableWidgetItem(QString(name)));
             ui->tbw_finish->setItem(0,1,new QTableWidgetItem(QString(size)));
             ui->tbw_finish->setItem(0,2,new QTableWidgetItem(QString(path)));
             ui->tbw_finish->setItem(0,3,new QTableWidgetItem(QString(finish_time)));
             ui->tbw_finish->setItem(0,4,new QTableWidgetItem(QString(taskid)));
             ui->tbw_download->removeRow(k);
              //write_all_config();
             return;
        }

    }
    update_cloud_download();
    update_cloud_upload();

}

void download_MainWindow::update_cloud_download(){
    int row = ui->tbw_cloud_download->rowCount();
    for(int i = 0;i<row;i++){
        QString taskid = ui->tbw_cloud_download->item(i,8)->text();
        QByteArray taskid_array = taskid.toLatin1();
        char *taskid_c = taskid_array.data();
        QProgressBar *ProgressBar;
        ProgressBar = (QProgressBar *)ui->tbw_cloud_download->cellWidget(i,3);
        int finish_size = ProgressBar->value();
        cld_list_st info;
        memset(&info,0,sizeof(cld_list_st));
        apx_cloud_task_status(taskid_c,&info);
        int task_state = info.u.taskinfo.status;
        QString state ;
        switch (task_state) {
        case 0:
            state = "等待";
            break;
        case 1:
            state = "启动";
            break;
        case 2:
            state = "完成";
            break;
        case 3:
            state = "暂停";
            break;
        case 4:
            state = "删除";
            break;
        case 5:
            state = "意外中止";
            break;
        case 6:
            state = "处理中";
            break;
        default:
            state = "未知";
            break;
        }

        int rate = 0;
        int total_size = 0;
        int local_size = 0;

        rate = info.u.taskinfo.speed;
        total_size = info.u.taskinfo.size;
        local_size = info.u.taskinfo.download_size;
        qDebug()<<"total_size:"<<total_size<<"local_size"<<local_size;
        QString s_total_size = size_from_int(total_size);

        if(finish_size!=100&&task_state!=2){

            int rest_size = 0;
            rest_size = total_size - local_size;
            int rest_time = 0;
            if(rate!=0)
                rest_time = rest_size/rate;
            QString string_rest_time = time_from_int(rest_time);
            QString s_rate = rate_from_int(rate);
            ui->tbw_cloud_download->setItem(i,1,new QTableWidgetItem(QString(s_total_size)));
            ui->tbw_cloud_download->setItem(i,4,new QTableWidgetItem(QString(s_rate)));
            ui->tbw_cloud_download->setItem(i,5,new QTableWidgetItem(QString(string_rest_time)));
            ui->tbw_cloud_download->setItem(i,6,new QTableWidgetItem(QString(state)));
            finish_size = (100*(long)local_size)/total_size;
            qDebug()<<"finish_size:"<<finish_size;
            if(finish_size>0&&finish_size<100){
                ProgressBar = new QProgressBar;
                ProgressBar->setRange(0,100);
                ProgressBar->setValue(finish_size);
                ui->tbw_cloud_download->setCellWidget(i,3,ProgressBar);
            }
        }
        else{
            QString name = ui->tbw_cloud_download->item(i,0)->text();

            QDateTime d_time = QDateTime::currentDateTime();
            QString date_time = d_time.toString("yyyy-MM-dd hh:mm:ss ddd");
            ui->tbw_cloud_filelist->insertRow(0);
            ui->tbw_cloud_filelist->setItem(0,0,new QTableWidgetItem(QString(name)));
            ui->tbw_cloud_filelist->setItem(0,1,new QTableWidgetItem(QString(s_total_size)));
            ui->tbw_cloud_filelist->setItem(0,2,new QTableWidgetItem(QString(date_time)));
            ui->tbw_cloud_filelist->setItem(0,4,new QTableWidgetItem(QString(taskid)));
            ui->tbw_cloud_download->removeRow(i);
            return;
        }







        /*int task_state = apx_task_state_get(taskid);
        if(task_state!=APX_TASK_STATE_FINISHED&&finish_size<100){
            QString state;
            if(task_state == APX_TASK_STATE_ACTIVE){
                state = "启动";
            }

            else if(task_state == APX_TASK_STATE_STOP){
                state = "暂停";
            }

            else if(task_state == APX_TASK_STATE_WAIT){
                state = "等待";
            }

            else if (task_state == APX_TASK_STATE_FINISHED){
                state = "完成";
            }
            unsigned long long total_size;
            unsigned long long local_size;
            unsigned long long residue_size;
            unsigned int download_rate;
            QString time = "00 : 00 : 00";
            QString s_rate = "0B/S";
            QString s_file_size = "0.0B";
            apx_task_speed_get(taskid,&download_rate,NULL);

            int residue_time=0;
            apx_task_file_size_get(taskid,&total_size,&local_size,NULL);
            qDebug()<<"total_size is: " + QString::number(total_size) + ",local_size is: " + QString::number(local_size);
            if(total_size>0){
                finish_size = (int)(local_size*100/total_size);
                residue_size = total_size-local_size;
                if(download_rate!=0){
                    residue_time = (int)residue_size/download_rate;
                    time = time_from_int(residue_time);
                }
            }
            if(download_rate>=1048576){
                download_rate = download_rate/1048576;
                 s_rate = QString::number(download_rate,'f',1) + "MB/S";
            }

            else if(download_rate<1048576 && download_rate>=1024){
                download_rate = download_rate/1024;
                 s_rate = QString::number(download_rate,'f',1) + "KB/S";
            }

            else if(download_rate<1024){
                 s_rate = QString::number(download_rate,'f',1) + "B/S";
            }

            if(total_size>=1073741824){
                total_size = total_size/1073741824;
                s_file_size = QString::number(total_size,'f',1) + "GB";
            }

            else if(total_size<1073741824 && total_size>=1048576){
                total_size = total_size/1048576;
                s_file_size = QString::number(total_size,'f',1) + "MB";
            }

            else if(total_size<1048576&&total_size>=1024){
                total_size =total_size/1024;
                s_file_size = QString::number(total_size,'f',1) + "KB";
            }

            else if(total_size<1024){
                s_file_size = QString::number(total_size,'f',1) + "B";
            }

            ui->tbw_cloud_download->setItem(i,1,new QTableWidgetItem(QString(s_file_size)));
            ui->tbw_cloud_download->setItem(i,4,new QTableWidgetItem(QString(s_rate)));
            ui->tbw_cloud_download->setItem(i,5,new QTableWidgetItem(QString(time)));
            ui->tbw_cloud_download->setItem(i,6,new QTableWidgetItem(QString(state)));

            if(finish_size<0||finish_size>100)
                finish_size=0;
            ProgressBar = new QProgressBar;
            ProgressBar->setRange(0,100);
            ProgressBar->setValue(finish_size);
            ui->tbw_cloud_download->setCellWidget(i,3,ProgressBar);*/
        }
        /*if(finish_size==100 || task_state==APX_TASK_STATE_FINISHED){
            int taskid = ui->tbw_cloud_upload->item(i,7)->text().toInt();
            unsigned long long total_size = 0;
            QString s_file_size = "";
            apx_task_file_size_get(taskid,&total_size,NULL,NULL);
            if(total_size>=1073741824){
                total_size = total_size/1073741824;
                s_file_size = QString::number(total_size,'f',1) + "GB";
            }

            else if(total_size<1073741824 && total_size>=1048576){
                total_size = total_size/1048576;
                s_file_size = QString::number(total_size,'f',1) + "MB";
            }

            else if(total_size<1048576&&total_size>=1024){
                total_size =total_size/1024;
                s_file_size = QString::number(total_size,'f',1) + "KB";
            }

            else if(total_size<1024){
                s_file_size = QString::number(total_size,'f',1) + "B";
            }
            QString name = ui->tbw_cloud_download->item(i,0)->text();
            QString size = ui->tbw_cloud_download->item(i,1)->text();
            QDateTime d_time = QDateTime::currentDateTime();
            QString date_time = d_time.toString("yyyy-MM-dd hh:mm:ss ddd");
            ui->tbw_cloud_filelist->insertRow(0);
            ui->tbw_cloud_filelist->setItem(0,0,new QTableWidgetItem(QString(name)));
            ui->tbw_cloud_filelist->setItem(0,1,new QTableWidgetItem(QString(size)));
            ui->tbw_cloud_filelist->setItem(0,2,new QTableWidgetItem(QString(date_time)));
            ui->tbw_cloud_download->removeRow(i);
            return;
        }
    }*/
}

void download_MainWindow::update_cloud_upload(){
    int row = ui->tbw_cloud_upload->rowCount();
    for(int i = 0;i<row;i++){
        int taskid = ui->tbw_cloud_upload->item(i,7)->text().toInt();
        QProgressBar *ProgressBar;
        ProgressBar = (QProgressBar *)ui->tbw_cloud_upload->cellWidget(i,3);
        int finish_size = ProgressBar->value();
        int task_state = apx_task_state_get(taskid);
        if(task_state!=APX_TASK_STATE_FINISHED&&finish_size<100){
            QString state;
            if(task_state == APX_TASK_STATE_ACTIVE){
                state = "启动";
            }

            else if(task_state == APX_TASK_STATE_STOP){
                state = "暂停";
            }

            else if(task_state == APX_TASK_STATE_WAIT){
                state = "等待";
            }

            else if (task_state == APX_TASK_STATE_FINISHED){
                state = "完成";
            }
            unsigned long long total_size;
            unsigned long long local_size;
            unsigned long long residue_size;
            unsigned int upload_rate;
            QString time = "00 : 00 : 00";
            QString s_rate = "0B/S";
            QString s_file_size = "0.0B";
            apx_task_speed_get(taskid,NULL,&upload_rate);

            int residue_time=0;
            apx_task_file_size_get(taskid,&total_size,NULL,&local_size);
            qDebug()<<"total_size is: " + QString::number(total_size) + ",up_size is: " + QString::number(local_size);
            if(total_size>0){
                finish_size = (int)(local_size*100/total_size);
                residue_size = total_size-local_size;
                if(upload_rate!=0){
                    residue_time = (int)residue_size/upload_rate;
                    time = time_from_int(residue_time);
                }
            }
            if(upload_rate>=1048576){
                upload_rate = upload_rate/1048576;
                 s_rate = QString::number(upload_rate,'f',1) + "MB/S";
            }

            else if(upload_rate<1048576 && upload_rate>=1024){
                upload_rate = upload_rate/1024;
                 s_rate = QString::number(upload_rate,'f',1) + "KB/S";
            }

            else if(upload_rate<1024){
                 s_rate = QString::number(upload_rate,'f',1) + "B/S";
            }

            if(total_size>=1073741824){
                total_size = total_size/1073741824;
                s_file_size = QString::number(total_size,'f',1) + "GB";
            }

            else if(total_size<1073741824 && total_size>=1048576){
                total_size = total_size/1048576;
                s_file_size = QString::number(total_size,'f',1) + "MB";
            }

            else if(total_size<1048576&&total_size>=1024){
                total_size =total_size/1024;
                s_file_size = QString::number(total_size,'f',1) + "KB";
            }

            else if(total_size<1024 ){
                s_file_size = QString::number(total_size,'f',1) + "B";
            }

            ui->tbw_cloud_upload->setItem(i,1,new QTableWidgetItem(QString(s_file_size)));
            ui->tbw_cloud_upload->setItem(i,4,new QTableWidgetItem(QString(s_rate)));
            ui->tbw_cloud_upload->setItem(i,5,new QTableWidgetItem(QString(time)));
            ui->tbw_cloud_upload->setItem(i,6,new QTableWidgetItem(QString(state)));
            //qDebug()<<"finish_size is: "<<finish_size;
            if(finish_size<0||finish_size>100)
                finish_size=0;
            ProgressBar = new QProgressBar;
            ProgressBar->setRange(0,100);
            ProgressBar->setValue(finish_size);
            ui->tbw_cloud_upload->setCellWidget(i,3,ProgressBar);
        }
        if(finish_size==100 || task_state==APX_TASK_STATE_FINISHED){
            u64 total_size = 0;
            apx_task_file_size_get(taskid,&total_size,NULL,NULL);
            QString size = size_from_int((int)total_size);
            QString name = ui->tbw_cloud_upload->item(i,0)->text();
            QDateTime d_time = QDateTime::currentDateTime();
            QString file_id = ui->tbw_cloud_upload->item(i,8)->text();
            int Id = ui->tbw_cloud_upload->item(i,7)->text().toInt();
            QString date_time = d_time.toString("yyyy-MM-dd hh:mm:ss");
            ui->tbw_cloud_filelist->insertRow(0);
            ui->tbw_cloud_filelist->setItem(0,0,new QTableWidgetItem(QString(name)));
            ui->tbw_cloud_filelist->setItem(0,1,new QTableWidgetItem(QString(size)));
            ui->tbw_cloud_filelist->setItem(0,2,new QTableWidgetItem(QString(date_time)));
            ui->tbw_cloud_filelist->setItem(0,4,new QTableWidgetItem(QString(file_id)));
            ui->tbw_cloud_filelist->setItem(0,3,new QTableWidgetItem(QString::number(Id)));
            ui->tbw_cloud_upload->removeRow(i);
            //write_all_config();
            return;
        }
    }
}

QString download_MainWindow::time_from_int(int time_int){
    QString time;
    time_int = time_int + 1;
    if(time_int>=3600){
        int hour = time_int/3600;
        int minute = (time_int-hour*3600)/60;
        int second = time_int-hour*3600-minute*60 ;
        if(minute<10&&second<10)
            time = QString::number(hour) + " : 0" + QString::number(minute) + " : 0" + QString::number(second);
        else if(minute<10&&second>=10)
            time = QString::number(hour) + " : 0" + QString::number(minute) + " : " + QString::number(second);
        else if(minute>=10&&second<10)
            time = QString::number(hour) + " : " + QString::number(minute) + " : 0" + QString::number(second);
        else
            time = QString::number(hour) + " : " + QString::number(minute) + " : " + QString::number(second);
    }
    if(time_int<3600&&time_int>=60){
        int minute = time_int/60;
        int second = time_int - minute*60;
        if(minute<10&&second<10)
            time = "00 : 0" + QString::number(minute) + " : 0" + QString::number(second);
        else if(minute<10&&second>=10)
            time = "00 : 0" + QString::number(minute) + " : " + QString::number(second);
        else if(minute>=10&&second<10)
            time = "00 : " + QString::number(minute) + " : 0" + QString::number(second);
        else
            time = "00 : " + QString::number(minute) + " : " + QString::number(second);
    }
    if(time_int<60){
        if(time_int<10)
            time = "00 : 00 : 0" + QString::number(time_int);
        else
            time = "00 : 00 : " + QString::number(time_int);
    }
    return time;
}

QString download_MainWindow::size_from_int(int size_int){
    int total_size = size_int;
    QString s_file_size = "";
    if(size_int>=1073741824){
        total_size = total_size/1073741824;
        s_file_size = QString::number(total_size,'f',1) + "GB";
    }

    else if(size_int<1073741824 && size_int>=1048576){
        total_size = total_size/1048576;
        s_file_size = QString::number(total_size,'f',1) + "MB";
    }

    else if(size_int<1048576&&size_int>=1024){
        total_size =total_size/1024;
        s_file_size = QString::number(total_size,'f',1) + "KB";
    }

    else if(size_int<1024){
        s_file_size = QString::number(total_size,'f',1) + "B";
    }
    return s_file_size;
}

QString download_MainWindow::rate_from_int(int rate_int){
    int download_rate = rate_int;
    QString s_rate = "";
    if(rate_int>=1048576){
        download_rate = download_rate/1048576;
         s_rate = QString::number(download_rate,'f',1) + "MB/S";
    }

    else if(rate_int<1048576 && rate_int>=1024){
        download_rate = download_rate/1024;
         s_rate = QString::number(download_rate,'f',1) + "KB/S";
    }

    else if(rate_int<1024){
         s_rate = QString::number(download_rate,'f',1) + "B/S";
    }
    return s_rate;

}

void download_MainWindow::on_btn_addnew_clicked()		//点击“新增”按钮执行的操作
{
    unsigned long long size;
    int finish_size = 0;
    QString rate = "0.0B/S";
    QString average_rate = "00 : 00 : 00";
    state = "启动";
    if(download_url!="")
        need_hide = true;
    addnew1.change_url();
    int result = addnew1.exec();
    if(!download_MainWindow::isVisible()){
        download_MainWindow::show();
        download_MainWindow::showMinimized();
    }

    if (cancel_new){

        /*if(param_start){
            download_MainWindow::hide();
            param_start = false;
            return;
        }*/

       if(param_start==false&&QApplication::argc()>=2){

            download_MainWindow::showMinimized();
            need_close = true;

            return;
        }
        else
            return;

    }
    //qDebug()<<"param_start:"<<param_start;
    if(param_start==false&&QApplication::argc()>=2){
        download_MainWindow::showMinimized();
        need_hide = true;
    }

    param_start = true;

    if(result==0){
        return;
    }

    if(result==2)
        remove_reset_row(check_uri);

    if(result==3){
        int row1 = ui->tbw_cloud_download->rowCount();
        ui->tbw_cloud_download->insertRow(row1);
        ui->tbw_cloud_download->setItem(row1,0,new QTableWidgetItem(QString(file_name)));
        apx_task_file_size_get(task_id,&size,NULL,NULL);
        double file_size = (double)size;
        QString s_file_size;

        if(file_size>=1073741824){
            file_size = file_size/1073741824;
            s_file_size = QString::number(file_size,'f',1) + "GB";
        }

        else if(file_size<1073741824 && file_size>=1048576){
            file_size = file_size/1048576;
            s_file_size = QString::number(file_size,'f',1) + "MB";
        }

        else if(file_size<1048576&&file_size>=1024){
            file_size =file_size/1024;
            s_file_size = QString::number(file_size,'f',1) + "KB";
        }

        else if(file_size<1024 &&file_size>=0){
            s_file_size = QString::number(file_size,'f',1) + "B";
        }
        ui->tbw_cloud_download->setItem(row1,1,new QTableWidgetItem(QString(s_file_size)));
        ui->tbw_cloud_download->setItem(row1,2,new QTableWidgetItem(QString(file_path)));
        ui->tbw_cloud_download->setItem(row1,4,new QTableWidgetItem(QString(rate)));
        ui->tbw_cloud_download->setItem(row1,5,new QTableWidgetItem(QString(average_rate)));
        ui->tbw_cloud_download->setItem(row1,6,new QTableWidgetItem(QString(state)));
        ui->tbw_cloud_download->setItem(row1,7,new QTableWidgetItem(QString::number(task_id,10)));
        ui->tbw_cloud_download->setItem(row1,8,new QTableWidgetItem(QString(fileId)));
        ProgressBar = new QProgressBar;
        ProgressBar->setRange(0,100);
        ProgressBar->setValue(finish_size);
        ui->tbw_cloud_download->setCellWidget(row1,3,ProgressBar);
        //write_all_config();
    }
    else{
        int row1 = ui->tbw_download->rowCount();
        ui->tbw_download->insertRow(row1);
        ui->tbw_download->setItem(row1,0,new QTableWidgetItem(QString(file_name)));
        apx_task_file_size_get(task_id,&size,NULL,NULL);
        double file_size = (double)size;
        QString s_file_size;

        if(file_size>=1073741824){
            file_size = file_size/1073741824;
            s_file_size = QString::number(file_size,'f',1) + "GB";
        }

        else if(file_size<1073741824 && file_size>=1048576){
            file_size = file_size/1048576;
            s_file_size = QString::number(file_size,'f',1) + "MB";
        }

        else if(file_size<1048576&&file_size>=1024){
            file_size =file_size/1024;
            s_file_size = QString::number(file_size,'f',1) + "KB";
        }

        else if(file_size<1024 &&file_size>=0){
            s_file_size = QString::number(file_size,'f',1) + "B";
        }

        ui->tbw_download->setItem(row1,1,new QTableWidgetItem(QString(s_file_size)));
        ui->tbw_download->setItem(row1,2,new QTableWidgetItem(QString(file_path)));
        ui->tbw_download->setItem(row1,4,new QTableWidgetItem(QString(rate)));
        ui->tbw_download->setItem(row1,5,new QTableWidgetItem(QString(average_rate)));
        ui->tbw_download->setItem(row1,6,new QTableWidgetItem(QString(state)));
        ui->tbw_download->setItem(row1,7,new QTableWidgetItem(QString::number(task_id,10)));
        ProgressBar = new QProgressBar;
        ProgressBar->setRange(0,100);
        ProgressBar->setValue(finish_size);
        ui->tbw_download->setCellWidget(row1,3,ProgressBar);
        //write_all_config();
    }
}


void download_MainWindow::remove_reset_row(int check_uri){
    int download_num = ui->tbw_download->rowCount();

    for(int i = 0;i<download_num;i++){

        if(ui->tbw_download->item(i,7)->text().toInt()==check_uri){
            ui->tbw_download->removeRow(i);
            return;
        }

    }

    int finish_num = ui->tbw_finish->rowCount();

    for(int j=0;j<finish_num;j++){

        if(ui->tbw_finish->item(j,4)->text().toInt()==check_uri){
            ui->tbw_finish->removeRow(j);
            return;
        }

    }

    int dustbin_num = ui->tbw_dustbin->rowCount();

    for(int k=0;k<dustbin_num;k++){

        if(ui->tbw_dustbin->item(k,4)->text().toInt()==check_uri){
            ui->tbw_dustbin->removeRow(k);
            return;
        }

    }

}

void download_MainWindow::read_all_config(){		//读取config.ini文件中的任务信息，将信息显示到界面上
    /*QSettings settings(user_path + "/.appex_config/config.ini",QSettings::IniFormat);
    int download_number = settings.value("num/download").toInt();
    int finish_number = settings.value("num/finish").toInt();
    int dustbin_number = settings.value("num/dustbin").toInt();

    for(int i=0;i<download_number;i++){
        QString group_name = QString("download%1").arg(QString(i));
        QString name = settings.value(group_name+"/name").toString();
        QString size = settings.value(group_name + "/size").toString();
        QDateTime time = settings.value(group_name+"/time").toDateTime();
        QString path = settings.value(group_name+"/path").toString();
        QString state = settings.value(group_name + "/state").toString();
        QString taskid1 = settings.value(group_name + "/task_id").toString();
        int finish_size = settings.value(group_name + "/finish_size").toInt();
        QString rate = settings.value(group_name + "/rate").toString();
        QString average_rate = settings.value(group_name + "/average_rate").toString();
        ui->tbw_download->insertRow(i);
        ui->tbw_download->setItem(i,0,new QTableWidgetItem(QString(name)));
        ui->tbw_download->setItem(i,1,new QTableWidgetItem(QString(size)));
        ui->tbw_download->setItem(i,2,new QTableWidgetItem(QString(path)));
        ui->tbw_download->setItem(i,6,new QTableWidgetItem(QString(state)));
        ui->tbw_download->setItem(i,7,new QTableWidgetItem(QString(taskid1)));
        ui->tbw_download->setItem(i,4,new QTableWidgetItem(QString(rate)));
        ui->tbw_download->setItem(i,5,new QTableWidgetItem(QString(average_rate)));
        ProgressBar = new QProgressBar;
        ProgressBar->setRange(0,100);
        ProgressBar->setValue(finish_size);
        ui->tbw_download->setCellWidget(i,3,ProgressBar);
    }

    for(int j=0;j<finish_number;j++){
        QString group_name = QString("finish%1").arg(QString(j));
        QString name = settings.value(group_name+"/name").toString();
        QString size = settings.value(group_name+"/size").toString();
        QString path = settings.value(group_name+"/path").toString();
        QString time = settings.value(group_name+"/time").toString();
        QString taskid1 = settings.value(group_name+"/task_id").toString();
        ui->tbw_finish->insertRow(j);
        ui->tbw_finish->setItem(j,0,new QTableWidgetItem(QString(name)));
        ui->tbw_finish->setItem(j,1,new QTableWidgetItem(QString(size)));
        ui->tbw_finish->setItem(j,2,new QTableWidgetItem(QString(path)));
        ui->tbw_finish->setItem(j,3,new QTableWidgetItem(QString(time)));
        ui->tbw_finish->setItem(j,4,new QTableWidgetItem(QString(taskid1)));
    }

    for(int k=0;k<dustbin_number;k++){
        QString group_name = QString("dustbin%1").arg(QString(k));
        QString name = settings.value(group_name+"/name").toString();
        QString size = settings.value(group_name+"/size").toString();
        QString path = settings.value(group_name+"/path").toString();
        QString time = settings.value(group_name+"/time").toString();
        QString taskid1 = settings.value(group_name+"/task_id").toString();
        QString state1 = settings.value(group_name+"/task_state").toString();
        ui->tbw_dustbin->insertRow(k);
        ui->tbw_dustbin->setItem(k,0,new QTableWidgetItem(QString(name)));
        ui->tbw_dustbin->setItem(k,1,new QTableWidgetItem(QString(size)));
        ui->tbw_dustbin->setItem(k,2,new QTableWidgetItem(QString(path)));
        ui->tbw_dustbin->setItem(k,3,new QTableWidgetItem(QString(time)));
        ui->tbw_dustbin->setItem(k,4,new QTableWidgetItem(QString(taskid1)));
        ui->tbw_dustbin->setItem(k,5,new QTableWidgetItem(QString(state1)));
    }*/
    struct uci_package *pkg = NULL;
    struct uci_context *ctx = NULL;
    struct uci_element *e = NULL;
    struct uci_ptr ptr;
    char file_name[128] = {0};
    char file_path[128] = {0};
    char buf[128]  = {0};
    int task_state = 0;
    QString name = "";
    int finish_size = 0;
    QString path = "";
    QString s_file_size = "";
    ctx = uci_alloc_context();
    if(ctx == NULL){
        log.uilog_write("get uci context failed.\n");
        return;
    }
    QString config_file_path = user_path + FILE_PATH;
    QByteArray user_path_arr = config_file_path.toLocal8Bit();
    strcpy(file_path,user_path_arr.data());
    if (uci_set_confdir(ctx, file_path) != UCI_OK)
    {
           uci_free_context(ctx);
           return;
    }
    strcpy(file_name,"taskinfo");
    if (uci_load(ctx, "taskinfo", &pkg) != UCI_OK)
    {
           uci_free_context(ctx);
           return;
    }

    uci_foreach_element(&pkg->sections, e)
    {
           struct uci_section *s = uci_to_section(e);
           if (strncmp(s->type, "task", strlen("task")) != 0)
                    continue;
            int taskid1 = atoi(s->e.name);
            snprintf(buf,sizeof(buf)-1,"taskinfo.%s.TaskState",s->e.name);
           if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
               uci_perror(ctx,"NULL");
               continue;
           }
           else
               task_state =  atoi(ptr.o->v.string);
           QString state = "";
           if(task_state == APX_TASK_STATE_ACTIVE){
               state = "启动";
           }

           else if(task_state == APX_TASK_STATE_STOP){
               state = "暂停";
           }

           else if(task_state == APX_TASK_STATE_WAIT){
               state = "等待";
           }

           else if (task_state == APX_TASK_STATE_FINISHED){
               state = "完成";
           }
           snprintf(buf,sizeof(buf)-1,"taskinfo.%s.DownloadFile",s->e.name);
           if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
               uci_perror(ctx,"NULL");
               continue;
           }
           else
               name =  QString(ptr.o->v.string);
           snprintf(buf,sizeof(buf)-1,"taskinfo.%s.DownloadPath",s->e.name);
           if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
               uci_perror(ctx,"NULL");
               continue;
           }
           else
               path =  QString(ptr.o->v.string);
           snprintf(buf,sizeof(buf)-1,"taskinfo.%s.TaskDownSize",s->e.name);
           if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
               uci_perror(ctx,"NULL");
               continue;
           }
           else{
               double down_size =  atof(ptr.o->v.string);
           snprintf(buf,sizeof(buf)-1,"taskinfo.%s.TaskTotalSize",s->e.name);
           if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
               uci_perror(ctx,"NULL");
               continue;
           }
           else{
               double file_size =  atof(ptr.o->v.string);
               finish_size = (int)(down_size*100/file_size);

               if(file_size>=1073741824){
                   file_size = file_size/1073741824;
                   s_file_size = QString::number(file_size,'f',1) + "GB";
               }

               else if(file_size<1073741824 && file_size>=1048576){
                   file_size = file_size/1048576;
                   s_file_size = QString::number(file_size,'f',1) + "MB";
               }

               else if(file_size<1048576&&file_size>=1024){
                   file_size =file_size/1024;
                   s_file_size = QString::number(file_size,'f',1) + "KB";
               }

               else if(file_size<1024 &&file_size>=0){
                   s_file_size = QString::number(file_size,'f',1) + "B";
               }
           }
           int task_type = 0;
           snprintf(buf,sizeof(buf)-1,"taskinfo.%s.TaskType",s->e.name);
           if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
               uci_perror(ctx,"NULL");
               continue;
           }
           else
               task_type =  atof(ptr.o->v.string);
           if((task_state==APX_TASK_STATE_ACTIVE||task_state==APX_TASK_STATE_STOP||task_state==APX_TASK_STATE_START||task_state==APX_TASK_STATE_WAIT)&&(task_type==APX_TASK_TYPE_DOWN)){
               QString rate = "0B/S";
               QString average_rate = "0B/S";
               int download_row = ui->tbw_download->rowCount();
               ui->tbw_download->insertRow(download_row);
               ui->tbw_download->setItem(download_row,0,new QTableWidgetItem(name));
               ui->tbw_download->setItem(download_row,1,new QTableWidgetItem(s_file_size));
               ui->tbw_download->setItem(download_row,2,new QTableWidgetItem(path));
               ui->tbw_download->setItem(download_row,6,new QTableWidgetItem(QString(state)));
               ui->tbw_download->setItem(download_row,7,new QTableWidgetItem(QString::number(taskid1)));
               ui->tbw_download->setItem(download_row,4,new QTableWidgetItem(QString(rate)));
               ui->tbw_download->setItem(download_row,5,new QTableWidgetItem(QString(average_rate)));
               ProgressBar = new QProgressBar;
               ProgressBar->setRange(0,100);
               ProgressBar->setValue(finish_size);
               ui->tbw_download->setCellWidget(download_row,3,ProgressBar);
           }

           else if(task_state== APX_TASK_STATE_FINISHED&&task_type==APX_TASK_TYPE_DOWN){
               QString time = "";
               snprintf(buf,sizeof(buf)-1,"taskinfo.%s.DuringTime",s->e.name);
               if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
                   uci_perror(ctx,"NULL");
                   continue;
               }
               else{
                   double during_time =  atof(ptr.o->v.string);
                   time = QString::number(during_time) + "s";
               }
               int finish_row = ui->tbw_finish->rowCount();
               ui->tbw_finish->insertRow(finish_row);
               ui->tbw_finish->setItem(finish_row,0,new QTableWidgetItem(QString(name)));
               ui->tbw_finish->setItem(finish_row,1,new QTableWidgetItem(QString(s_file_size)));
               ui->tbw_finish->setItem(finish_row,2,new QTableWidgetItem(QString(path)));
               ui->tbw_finish->setItem(finish_row,3,new QTableWidgetItem(QString(time)));
               ui->tbw_finish->setItem(finish_row,4,new QTableWidgetItem(QString::number(taskid1)));
           }

           else if((task_state == APX_TASK_STATE_TOBEDEL||task_state==APX_TASK_STATE_FINTOBEDEL)&&(task_type==APX_TASK_TYPE_DOWN)){
               QString time;
               snprintf(buf,sizeof(buf)-1,"taskinfo.%s.EndTime",s->e.name);
               if(uci_lookup_ptr(ctx,&ptr,buf,true)!=0){
                   uci_perror(ctx,"NULL");
                   continue;
               }
               else{
                   double end_time =  atof(ptr.o->v.string);
                   QDateTime date_time = QDateTime::fromTime_t(end_time);
                   time = date_time.toString("yyyy-MM-dd hh:mm:ss ddd");
               }
               QString state1 = "未完成";
               if(task_state==APX_TASK_STATE_FINTOBEDEL)
                   state1 = "已完成";
               int dustbin_row = ui->tbw_dustbin->rowCount();
               ui->tbw_dustbin->insertRow(dustbin_row);
               ui->tbw_dustbin->setItem(dustbin_row,0,new QTableWidgetItem(QString(name)));
               ui->tbw_dustbin->setItem(dustbin_row,1,new QTableWidgetItem(QString(s_file_size)));
               ui->tbw_dustbin->setItem(dustbin_row,2,new QTableWidgetItem(QString(path)));
               ui->tbw_dustbin->setItem(dustbin_row,3,new QTableWidgetItem(QString(time)));
               ui->tbw_dustbin->setItem(dustbin_row,4,new QTableWidgetItem(QString::number(taskid1)));
               ui->tbw_dustbin->setItem(dustbin_row,5,new QTableWidgetItem(QString(state1)));
           }

       }

    }
    uci_unload(ctx,pkg);
    uci_free_context(ctx);
}

void download_MainWindow::write_all_config(){		//读取界面上的正在下载、已下载、垃圾箱中的任务信息，将任务信息保存到config.ini文件中
    QSettings settings(user_path + FILE_PATH + "/config.ini",QSettings::IniFormat);
    settings.clear();
    int  download_number = ui->tbw_download->rowCount();
    int finish_number = ui->tbw_finish->rowCount();
    int dustbin_number = ui->tbw_dustbin->rowCount();
    settings.beginGroup("num");
    settings.setValue("download",download_number);
    settings.setValue("finish",finish_number);
    settings.setValue("dustbin",dustbin_number);
    settings.endGroup();

    for(int i=0;i<download_number;i++){
        QString name = ui->tbw_download->item(i,0)->text();
        QString size = ui->tbw_download->item(i,1)->text();
        QString path = ui->tbw_download->item(i,2)->text();
        QString rate = ui->tbw_download->item(i,4)->text();
        QString average_rate = ui->tbw_download->item(i,5)->text();
        QString state = ui->tbw_download->item(i,6)->text();
        QString taskid1 = ui->tbw_download->item(i,7)->text();
        QProgressBar *ProgressBar;
        ProgressBar = (QProgressBar *)ui->tbw_download->cellWidget(i,3);
        int finish_size = ProgressBar->value();
        QString group_name = QString("download%1").arg(QString(i));
        settings.beginGroup(group_name);
        settings.setValue("name",name);
        settings.setValue("size",size);
        settings.setValue("path",path);
        settings.setValue("state",state);
        settings.setValue("task_id",taskid1);
        settings.setValue("finish_size",finish_size);
        settings.setValue("rate",rate);
        settings.setValue("average_rate",average_rate);
        settings.endGroup();
    }

    for(int j=0;j<finish_number;j++){
        QString name = ui->tbw_finish->item(j,0)->text();
        QString size = ui->tbw_finish->item(j,1)->text();
        QString path = ui->tbw_finish->item(j,2)->text();
        QString time = ui->tbw_finish->item(j,3)->text();
        QString taskid1 = ui->tbw_finish->item(j,4)->text();
        QString group_name = QString("finish%1").arg(QString(j));
        settings.beginGroup(group_name);
        settings.setValue("name",name);
        settings.setValue("size",size);
        settings.setValue("path",path);
        settings.setValue("time",time);
        settings.setValue("task_id",taskid1);
        settings.endGroup();
    }

    for(int k=0;k<dustbin_number;k++){
        QString name = ui->tbw_dustbin->item(k,0)->text();
        QString size = ui->tbw_dustbin->item(k,1)->text();
        QString path = ui->tbw_dustbin->item(k,2)->text();
        QString time = ui->tbw_dustbin->item(k,3)->text();
        QString taskid2 = ui->tbw_dustbin->item(k,4)->text();
        QString state1 = ui->tbw_dustbin->item(k,5)->text();
        QString group_name = QString("dustbin%1").arg(QString(k));
        settings.beginGroup(group_name);
        settings.setValue("name",name);
        settings.setValue("size",size);
        settings.setValue("path",path);
        settings.setValue("time",time);
        settings.setValue("task_id",taskid2);
        settings.setValue("task_state",state1);
        settings.endGroup();
    }

}

void download_MainWindow::on_btn_settings_clicked()		//点击“设置”界面执行的操作
{
        settings set;
        set.exec();
}


void download_MainWindow::newLocalSocketConnection(){		//接收localsocket发过来的信息，如果发来的url不为空，则弹出“新建”界面
     QLocalSocket *socket = m_localServer->nextPendingConnection();

     if(!socket)
         return;

     socket->waitForReadyRead(1000);
     QTextStream stream(socket);
     download_url = stream.readAll();
     bool is_visible = true;
     if(!download_MainWindow::isVisible()){
         download_MainWindow::show();
         download_MainWindow::showMinimized();
         is_visible = false;
     }
     if(download_url!=""){
         QStringList download_list = download_url.split("|");
         if(download_list.count()==1 && download_list[0]=="quit"){
             close_mainwindow();
             return;
         }
         param_start = true;
         ui->btn_addnew->click();
         if(!is_visible)
             download_MainWindow::hide();
     }
     else {
         if(!download_MainWindow::isVisible())
             download_MainWindow::show();
         this->activateWindow();
         ui->tabWidget->setCurrentIndex(0);
         ui->btn_download->click();
     }

     delete socket;
}

void download_MainWindow::on_btn_check_clicked()		//点击界面上的“开始检测”按钮，执行网络故障检测的操作
{
    check_type = APX_NET_CHECK_LOCAL;
    ui->btn_check->setEnabled(false);
    ui->btn_check->setText("检测中..");
    check_network = new network_thread;
    QObject::connect(check_network,SIGNAL(network_signal(QString)),this,SLOT(hand_network_Message(QString)));
    QObject::connect(check_network,SIGNAL(network_stop_signal()),this,SLOT(network_deteck_stop()));
    check_network->start();

}

void download_MainWindow::on_btn_check_cloud_clicked()
{
    check_type = APX_NET_CHECK_CLOUD;
    ui->btn_check_cloud->setEnabled(false);
    ui->btn_check_cloud->setText("检测中..");
    check_network = new network_thread;
    QObject::connect(check_network,SIGNAL(network_signal(QString)),this,SLOT(hand_network_Message(QString)));
    QObject::connect(check_network,SIGNAL(network_cloud_signal(QString)),this,SLOT(hand_network_cloud_Message(QString)));
    QObject::connect(check_network,SIGNAL(network_stop_signal_cloud()),this,SLOT(network_deteck_stop_cloud()));
    check_network->start();
}

void download_MainWindow::hand_network_Message(QString text){		//在界面上同步更新网络故障检测的结果
    ui->txEdit_network_result->setText(text);
}

void download_MainWindow::hand_network_cloud_Message(QString text){
    ui->txEdit_network_cloud_result->setText(text);
}

void download_MainWindow::network_deteck_stop(){		//网络故障检测结束进行的操作，将检测按钮重新设置成可点击的
    ui->btn_check->setEnabled(true);
    ui->btn_check->setText("重新检测本机");
}

void download_MainWindow::network_deteck_stop_cloud(){
    ui->btn_check_cloud->setEnabled(true);
    ui->btn_check_cloud->setText("重新检测云端");
}

void download_MainWindow::on_btn_login_clicked()		//点击离线空间界面的登陆按钮进行的操作
{
    /*u32 ip;
    u16 port;

    if(apx_conf_serv_get(NULL,0,&ip,&port)!=0){
        QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("连接服务器失败！"));
        msgbox.setStandardButtons(QMessageBox::Ok);
        msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
        msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
        msgbox.setStyleSheet("QMessageBox{background-color:white}");
        msgbox.exec();
        return;
    }

    QString username = ui->username->text();
    QString password = ui->password->text();
    QByteArray username_array = username.toLatin1();
    char *c_username = username_array.data();
    QByteArray password_array = password.toLatin1();
    char *c_password = password_array.data();*/
    check_login = new login_thread;
    QObject::connect(check_login,SIGNAL(login_signal(int)),this,SLOT(check_login_result(int)));
    QString username = ui->username->text();
    QString password = ui->password->text();
    check_login->set_user_pass(username,password);
    check_login->start();
    ui->btn_login->setEnabled(false);
    ui->btn_login->setText(QString("登录中.."));

    //uid = apx_user_login(ip,port,c_username,c_password);
}

void download_MainWindow::check_login_result(int userid){
    uid = userid;
    if(uid<=0){
        if(uid==0){
            QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("用户名或密码错误！"));
            msgbox.setStandardButtons(QMessageBox::Ok);
            msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
        }
        else if(uid==-1){
            QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("获取内存失败！"));
            msgbox.setStandardButtons(QMessageBox::Ok);
            msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
        }
        else if(uid==-2){
            QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("保存用户信息失败！"));
            msgbox.setStandardButtons(QMessageBox::Ok);
            msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
        }
        else if(uid==-3){
            QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("该用户已登录！"));
            msgbox.setStandardButtons(QMessageBox::Ok);
            msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
        }
        else if(uid==-4){
            QMessageBox msgbox(QMessageBox::NoIcon,QString("错误"),QString("连接服务器失败！"));
            msgbox.setStandardButtons(QMessageBox::Ok);
            msgbox.setButtonText(QMessageBox::Ok,QString("确定"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/error.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            msgbox.exec();
        }
        uid=0;
        ui->btn_login->setEnabled(true);
        ui->btn_login->setText(QString("登录"));
        return;
    }
    ui->frame->show();
    ui->pushButton_filelists->show();
    ui->pushButton_download->show();
    ui->pushButton_upload->show();
    ui->label_username->hide();
    ui->label_password->hide();
    ui->username->hide();
    ui->password->hide();
    ui->btn_login->hide();
    ui->btn_upload->show();
    ui->pushButton_filelists->click();
    get_cloud_list();
    is_login = true;
}

void download_MainWindow::on_pushButton_filelists_clicked()		//点击离线空间界面的文件列表按钮进行的操作，隐藏其他列表，重写QSS样式表，显示按钮被点击后的效果
{
    ui->tbw_cloud_filelist->show();
    ui->tbw_cloud_download->hide();
    ui->tbw_cloud_upload->hide();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_cloud_result->hide();
    QFile styleFile(":/qss/btn_cloud_filelist.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tab_5->setStyleSheet(qss);
        styleFile.close();
    }

}

void download_MainWindow::on_pushButton_download_clicked()		//点击离线空间界面的正在下载按钮进行的操作，隐藏其他列表，重写QSS样式表，显示按钮被点击后的效果
{
    ui->tbw_cloud_filelist->hide();
    ui->tbw_cloud_download->show();
    ui->tbw_cloud_upload->hide();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_cloud_result->hide();
    QFile styleFile(":/qss/btn_cloud_download.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tab_5->setStyleSheet(qss);
        styleFile.close();
    }

}


void download_MainWindow::on_pushButton_cloud_check_clicked()
{
    ui->tbw_cloud_filelist->hide();
    ui->tbw_cloud_download->hide();
    ui->tbw_cloud_upload->hide();
    ui->btn_check_cloud->show();
    ui->txEdit_network_cloud_result->show();
    QFile styleFile(":/qss/btn_cloud_check.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tab_5->setStyleSheet(qss);
        styleFile.close();
    }
}

void download_MainWindow::on_pushButton_upload_clicked()		//点击离线空间界面的正在上传按钮进行的操作，隐藏其他列表，重写QSS样式表，显示按钮被点击后的效果
{
    ui->tbw_cloud_filelist->hide();
    ui->tbw_cloud_download->hide();
    ui->tbw_cloud_upload->show();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_cloud_result->hide();
    QFile styleFile(":/qss/btn_cloud_upload.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tab_5->setStyleSheet(qss);
        styleFile.close();
    }

}

void download_MainWindow::on_btn_upload_clicked()		//点击“上传”按钮进行的操作
{
    cloud_upload cloud_upload1;
    int result = cloud_upload1.exec();
    if(result==0)
        return;
    QString files = cloud_path + "/" + cloud_filename;
    QFile file(files);
    double size = 0;
    QString s_file_size;
    if(file.open(QIODevice::ReadOnly))
        size = (double)file.size();
    if(size>=1073741824){
        size = size/1073741824;
        s_file_size = QString::number(size,'f',1) + "GB";
    }

    else if(size<1073741824 && size>=1048576){
        size = size/1048576;
        s_file_size = QString::number(size,'f',1) + "MB";
    }

    else if(size<1048576&&size>=1024){
        size = size/1024;
        s_file_size = QString::number(size,'f',1) + "KB";
    }

    else if(size<1024 &&size>=0){
        s_file_size = QString::number(size,'f',1) + "B";
    }
    QString rate = "0B/S";
    QString time = "未知";
    QString state = "等待";
    int upload_row = ui->tbw_cloud_upload->rowCount();
    ui->tbw_cloud_upload->insertRow(upload_row);
    ui->tbw_cloud_upload->setItem(upload_row,0,new QTableWidgetItem(cloud_filename));
    ui->tbw_cloud_upload->setItem(upload_row,1,new QTableWidgetItem(s_file_size));
    ui->tbw_cloud_upload->setItem(upload_row,2,new QTableWidgetItem(cloud_path));
    ui->tbw_cloud_upload->setItem(upload_row,4,new QTableWidgetItem(rate));
    ui->tbw_cloud_upload->setItem(upload_row,5,new QTableWidgetItem(time));
    ui->tbw_cloud_upload->setItem(upload_row,6,new QTableWidgetItem(state));
    ui->tbw_cloud_upload->setItem(upload_row,7,new QTableWidgetItem(QString::number(task_id)));
    ui->tbw_cloud_upload->setItem(upload_row,8,new QTableWidgetItem(fileId));
    int finish_size = 0;
    ProgressBar = new QProgressBar;
    ProgressBar->setRange(0,100);
    ProgressBar->setValue(finish_size);
    ui->tbw_cloud_upload->setCellWidget(upload_row,3,ProgressBar);
}

void download_MainWindow::on_btn_download_clicked()
{
    ui->tbw_download->show();
    ui->tbw_finish->hide();
    ui->tbw_dustbin->hide();
    ui->btn_check->hide();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_result->hide();
    QFile styleFile(":/qss/btn_download.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tabWidget->setStyleSheet(qss);
        styleFile.close();
    }

}

void download_MainWindow::on_btn_finish_clicked()
{
    ui->tbw_download->hide();
    ui->tbw_finish->show();
    ui->tbw_dustbin->hide();
    ui->btn_check->hide();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_result->hide();
    QFile styleFile(":/qss/btn_finish.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tabWidget->setStyleSheet(qss);
        styleFile.close();
    }

}

void download_MainWindow::on_btn_dustbin_clicked()
{
    ui->tbw_download->hide();
    ui->tbw_finish->hide();
    ui->tbw_dustbin->show();
    ui->btn_check->hide();
    ui->btn_check_cloud->hide();
    ui->txEdit_network_result->hide();
    QFile styleFile(":/qss/btn_dustbin.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tabWidget->setStyleSheet(qss);
        styleFile.close();
    }

}

void download_MainWindow::on_btn_offline_clicked()
{
    ui->tabWidget->addTab(ui->tab_5,QString("离线空间"));
    ui->tabWidget->setCurrentIndex(1);
}

void download_MainWindow::on_btn_network_clicked()
{
    ui->tbw_download->hide();
    ui->tbw_finish->hide();
    ui->tbw_dustbin->hide();
    ui->btn_check->show();
    //ui->btn_check_cloud->show();
    ui->txEdit_network_result->show();
    QFile styleFile(":/qss/btn_network.qss");
    if(styleFile.open(QIODevice::ReadOnly)){
        QString qss = QLatin1String(styleFile.readAll());
        ui->tabWidget->setStyleSheet(qss);
        styleFile.close();
    }

}

void download_MainWindow::close_mainwindow(){
    /*int rowcount = ui->tbw_download->rowCount();
    for(int i=0;i<rowcount;i++){
        int taskid = ui->tbw_download->item(i,7)->text().toInt();
        int bp_result = apx_task_bpcontinue_get(taskid);
        if(bp_result==0){
            QMessageBox msgbox(QMessageBox::NoIcon,tr("提示"),QString("检测到存在不支持断点续传的任务,任务编号:" + QString::number(taskid)+  "，关闭程序将会删除该任务，是否继续？"));
            msgbox.setIconPixmap(QPixmap(":/home/appex/image/info.png"));
            msgbox.setStyleSheet("QMessageBox{background-color:white}");
            QPushButton *button_ok = msgbox.addButton(tr("是"),QMessageBox::AcceptRole);
            QPushButton *button_cancel = msgbox.addButton(tr("否"),QMessageBox::RejectRole);
            int msg_result = msgbox.exec();
            if(msgbox.clickedButton() == button_ok){
                apx_task_destroy(taskid);
                ui->tbw_download->removeRow(i);
                i--;
                rowcount--;
            }

            if(msgbox.clickedButton() == button_cancel || msg_result == 1){
                return;
            }
        }
    }*/

    qApp->quit();
    log.uilog_write("quit from addnew UI.\n");
}

void download_MainWindow::show_mainwindow(){
    if(!download_MainWindow::isVisible())
        download_MainWindow::show();
    download_MainWindow::activateWindow();
}

void download_MainWindow::click_addnew_buttin(){
    this->hide();
    ui->btn_addnew->click();
}


void download_MainWindow::get_cloud_list(){
    //put list to cloud filelist
    cld_tasklist_st tasklist;
    apx_cloud_task_list(0,0,&tasklist);
    u16 task_number = tasklist.total;
    QString name;
    unsigned long long size;
    QString file_Id;
    char *time;
    QString ctime;
    for(int i=0;i<task_number;i++){
        if(tasklist.plist == NULL)
            break;
        if(tasklist.plist[i].type==2){
            name = QString(QLatin1String(tasklist.plist[i].u.fileinfo.name));
            size = tasklist.plist[i].u.fileinfo.size;
            file_Id = tasklist.plist[i].u.fileinfo.fileId;
            time = (char *)tasklist.plist[i].u.fileinfo.ctime;
            ctime = QString(time);
        }
        else{
            name = QString(QLatin1String((char*)tasklist.plist[i].u.taskinfo.name));
            size = tasklist.plist[i].u.taskinfo.size;
            file_Id = QString((char *)tasklist.plist[i].u.taskinfo.fileId);
            time = (char *)tasklist.plist[i].u.taskinfo.ctime;
            ctime = QString(time);
            if(tasklist.plist[i].u.taskinfo.status!=2){
                QString s_size = size_from_int(size);
                QString file_path=QDir::homePath();
                QString rate = "0 B/S";
                QString average_rate = "00 : 00 : 00";
                int finish_size = 0;
                int row1 = ui->tbw_cloud_download->rowCount();
                ui->tbw_cloud_download->setItem(row1,0,new QTableWidgetItem(QString(name)));
                ui->tbw_cloud_download->setItem(row1,1,new QTableWidgetItem(QString(s_size)));
                ui->tbw_cloud_download->setItem(row1,2,new QTableWidgetItem(QString(file_path)));
                ui->tbw_cloud_download->setItem(row1,4,new QTableWidgetItem(QString(rate)));
                ui->tbw_cloud_download->setItem(row1,5,new QTableWidgetItem(QString(average_rate)));
                ui->tbw_cloud_download->setItem(row1,6,new QTableWidgetItem(QString(state)));
                ui->tbw_cloud_download->setItem(row1,8,new QTableWidgetItem(QString(fileId)));
                ProgressBar = new QProgressBar;
                ProgressBar->setRange(0,100);
                ProgressBar->setValue(finish_size);
                ui->tbw_cloud_download->setCellWidget(row1,3,ProgressBar);
            }
        }
        QString s_size = size_from_int(size);
        ui->tbw_cloud_filelist->insertRow(i);
        ui->tbw_cloud_filelist->setItem(i,0,new QTableWidgetItem(name));
        ui->tbw_cloud_filelist->setItem(i,1,new QTableWidgetItem(s_size));
        ui->tbw_cloud_filelist->setItem(i,2,new QTableWidgetItem(ctime));
        ui->tbw_cloud_filelist->setItem(i,4,new QTableWidgetItem(file_Id));

    }
}


