#include "network_thread.h"
#include "download_mainwindow.h"
#include <QString>
#include <QThread>
extern "C"{
    #include "../include/apx_hftsc_api.h"
    #include "../client/apx_proto_ctl.h"
}
extern int check_type;
network_thread::~network_thread()
{
}

void network_thread::run(){		//网络故障检测的线程，用于检测网络是否正常，每检查完一项，向UI主线程发送信息，通知UI更新信息
    if(check_type == APX_NET_CHECK_LOCAL){
        char *pAddr = new char[16];
        char *pGateWay = new char[16];
        QString text;
        QString network_ok = "网络连接正常!";
        QString eth_work = "检查接口                                                                                                   正常\n";
        QString ip_work = "检查IP                                                                                                      已配置\n";
        QString dns_work = "检查DNS                                                                                                   正常\n";
        QString route_work = "检查路由与网关                                                                                        正常\n";
        QString eth_error = "检查接口                                                                                                  接口不存在或没有up\n";
        QString ip_error = "检查IP                                                                                                      IP未配置\n";
        QString dns_unset = "检查DNS                                                                                                  DNS未配置\n";
        QString dns_error = "检查DNS                                                                                                  DNS不通\n";
        QString route_unset = "检查路由和网关                                                                                         路由或网关未配置\n";
        QString route_error = "检查路由和网关                                                                                         路由或网关不通\n";
        apx_net_start();
        APX_NETWORK_E eth_result = apx_net_detect_interface();

        if(eth_result == APX_NET_INTER_ERR){
            text = eth_error;
            apx_net_end();
        }

        else{
            text = eth_work + "正在检查IP..";
        }

        emit network_signal(text);
        APX_NETWORK_E ip_result = apx_net_detect_ip();

        if(ip_result == APX_NET_IP_UNSET){
            text = eth_work + ip_error;
            apx_net_end();
            emit network_signal(text);
        }

        else if(ip_result == APX_NET_OK){
            text = eth_work + ip_work + "正在检查路由和网关..";
            emit network_signal(text);
        }

        else if(ip_result == APX_NET_UNKOWN){
            emit network_stop_signal();
            return;
        }

        APX_NETWORK_E route_result = apx_net_detect_route(pAddr,pGateWay);

        if(route_result == APX_NET_ROUTE_UNSET){
            text = eth_work + ip_work + route_unset;
            apx_net_end();
            emit network_signal(text);
            emit network_stop_signal();
            return;
        }

        else if(route_result == APX_NET_ROUTE_UNREACH){
            text = eth_work + ip_work + route_error;
            apx_net_end();
            emit network_signal(text);
            emit network_stop_signal();
            return;
        }

        else if(route_result == APX_NET_OK){
            text = eth_work + ip_work + route_work + "正在检查DNS..";
            emit network_signal(text);
        }

        else if(route_result == APX_NET_UNKOWN){
            emit network_stop_signal();
            return;
        }

        APX_NETWORK_E dns_result = APX_NET_OK;
        dns_result = apx_net_detect_dns(pAddr);

        if(dns_result == APX_NET_DNS_UNSET){
            text = eth_work + ip_work + route_work + dns_unset;
            emit network_signal(text);
        }

        else if (dns_result == APX_NET_DNS_UNREACH){
            text = eth_work + ip_work + route_work + dns_error;
            emit network_signal(text);
        }

        else if (dns_result == APX_NET_OK){
            text = eth_work + ip_work + route_work + dns_work + network_ok;
            emit network_signal(text);
        }

        apx_net_end();
        emit network_stop_signal();
        delete pAddr;
        delete pGateWay;
    }
    if(check_type == APX_NET_CHECK_CLOUD){
        QString text = "cloud check.\n";
        QString connect_server_ok = "连接服务器                                                                                                          成功\n";
        QString connect_server_error = "连接服务器                                                                                                          失败\n";
        QString get_date_ok = "获取服务器信息                                                                                                  成功\n";
        QString get_date_error = "获取服务器信息                                                                                                  失败\n";
        QString serverCon_ok = "存储服务器                                                                                                          正常\n";
        QString serverCon_error = "存储服务器                                                                                                          故障\n";
        QString storageSpace_ok = "存储空间                                                                                                              可用\n";
        QString storageSpace_error = "存储空间                                                                                                              已满\n";
        QString dbCon_ok = "数据库服务器                                                                                                      正常\n";
        QString dbCon_error = "数据库服务器                                                                                                      故障\n";
        QString dbStorage_ok = "数据库存储空间                                                                                                  可用\n";
        QString dbStorage_error = "数据库存储空间                                                                                                  已满\n";
        QString dnsResolve_ok = "DNS解析                                                                                                              正常\n";
        QString dnsResolve_error = "DNS解析                                                                                                              故障\n";

        cld_fault_st fault_info;
        memset(&fault_info,0,sizeof(cld_fault_st));
        int res = apx_cloud_fault_check(&fault_info);
        if(res<0){
            text = connect_server_error;
            emit network_cloud_signal(text);
            emit network_stop_signal_cloud();
            return;
        }
        else{
            text = connect_server_ok + "正在获取服务器信息..";
            emit network_cloud_signal(text);
        }
        sleep(1);
        /*if(http_code!=HTTP_OK){
            text = connect_server_ok + get_date_error;
            emit network_cloud_signal(text);
            emit network_stop_signal_cloud();
            return;
        }
        else{*/
            text = connect_server_ok + get_date_ok + "正在分析服务器信息..";
            emit network_cloud_signal(text);
        //}
        sleep(1);
        text = connect_server_ok + get_date_ok;
        if(fault_info.serverCon==0)
            text = text + serverCon_error;
        else
            text = text + serverCon_ok;
        if(fault_info.storageSpace==0)
            text = text + storageSpace_error;
        else
            text = text + storageSpace_ok;
        if(fault_info.dbCon==0)
            text = text + dbCon_error;
        else
            text = text + dbCon_ok;
        if(fault_info.dbStorage==0)
            text = text + dbStorage_error;
        else
            text = text + dbStorage_ok;
        if(fault_info.dnsResolve==0)
            text = text + dnsResolve_error;
        else
            text = text + dnsResolve_ok;
        emit network_cloud_signal(text);
        emit network_stop_signal_cloud();
    }
}
