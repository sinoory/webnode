#ifndef DOWNLOAD_MAINWINDOW_H
#define DOWNLOAD_MAINWINDOW_H
#include "myThread.h"
#include "network_thread.h"
#include "login_thread.h"
#include "addnew.h"
#include "local_download.h"
#include "bt_list.h"
#include "settings.h"
#include "start_task_thread.h"
#include "cloud.h"
#include "cloud_upload.h"
#include <QMainWindow>
#include <QThread>
#include <QContextMenuEvent>
#include <QTableWidgetItem>
#include <QMenu>
#include <QAction>
#include <QPoint>
#include <QLocalServer>
#include <QSystemTrayIcon>

#define APX_NET_CHECK_LOCAL 0
#define APX_NET_CHECK_CLOUD 1
#define FILE_PATH "/.config/cdosbrowser/appex_config"
class QProgressBar;
namespace Ui {
class download_MainWindow;
class myThread;
}




class download_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit download_MainWindow(QWidget *parent = 0);
    ~download_MainWindow();

    void createActions();

    void closeEvent(QCloseEvent *event);

    void remove_reset_row(int check_uri);

    void update_cloud_download();

    void update_cloud_upload();

    QString time_from_int(int time_int);

    QString size_from_int(int size_int);

    QString rate_from_int(int rate_int);

protected:

    void get_cloud_list();

    //void contextMenuEvent(QContextMenuEvent *event);

private slots:

    void show_contextMenu(const QPoint&);

    void show_contextMenu_finish(const QPoint&);

    void show_contextMenu_dustbin(const QPoint&);

    void show_contextMenu_filelist(const QPoint&);

    void show_contextMenu_cloud_download(const QPoint&);

    void show_contextMenu_cloud_upload(const QPoint&);

    void hand_Message();

    void stop_task();

    void start_task();

    void delete_task();

    void open_task();

    void open_task_dir();

    void delete_finish_task();

    void recover_task();

    void delete_dustbin_task();

    void download_to_local();

    void delete_filelist();

    void stop_cloud_download_task();

    void start_cloud_download_task();

    void delete_cloud_download_task();

    void stop_cloud_upload_task();

    void start_cloud_upload_task();

    void delete_cloud_upload_task();

    void read_all_config();

    void write_all_config();

    void on_btn_settings_clicked();

    void newLocalSocketConnection();

    void on_btn_check_clicked();

    void hand_network_Message(QString);

    void hand_network_cloud_Message(QString);

    void network_deteck_stop();

    void network_deteck_stop_cloud();

    void check_open_result(int);

    void check_login_result(int);

    void on_btn_login_clicked();

    void on_pushButton_filelists_clicked();

    void on_pushButton_upload_clicked();

    void on_pushButton_download_clicked();

    void on_btn_upload_clicked();

    void on_btn_download_clicked();

    void on_btn_finish_clicked();

    void on_btn_dustbin_clicked();

    void on_btn_offline_clicked();

    void on_btn_network_clicked();

    void on_btn_addnew_clicked();

    void close_mainwindow();

    //void iconActivated(QSystemTrayIcon::ActivationReason);

    void show_mainwindow();

    void on_btn_check_cloud_clicked();

    void on_pushButton_cloud_check_clicked();

    void click_addnew_buttin();

signals:
    void newConnection();

    void close_mainwindow_signal();

private:
    Ui::download_MainWindow *ui;
    addnew addnew1;
    local_download local_download1;
    cloud cloud1;

    QProgressBar *ProgressBar;
    myThread *mythread;
    QMenu *pop_menu=NULL;
    QAction *action_stop;
    QAction *action_start;
    QAction *action_delete;
    QAction *action_open;
    QAction *action_open_dir;
    QAction *action_delete_finish;
    QAction *action_recover;
    QAction *action_delete_dustbin;
    QAction *action_quit;
    QAction *action_show_window;
    QAction *action_down_to_local;
    QAction *action_filelist_delete;
    QAction *action_cloud_download_delete;
    QAction *action_cloud_download_stop;
    QAction *action_cloud_download_start;
    QAction *action_cloud_upload_delete;
    QAction *action_cloud_upload_stop;
    QAction *action_cloud_upload_start;
    QContextMenuEvent *event;
    QPoint *point;
    QTableWidgetItem *item;

//    int row;

};

#endif // DOWNLOAD_MAINWINDOW_H
