/***********************************************************************
文件名称main.h
功    能：
编写时间：
编 写 人：
注    意：
***********************************************************************/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

 #include "stm32f10x.h"
//#include "stm32f107.h"
#include "port1.h"
#include "stm32_eth.h"
#include "netconf.h"
//#include "helloworld.h"
#include "httpd.h"
#include "tftpserver.h"
#include "lwip/tcp.h"
#include "TCP_CLIENT.h"
#include "TCP_SERVER.h"

#define SYSTEMTICK_PERIOD_MS  1

#define __IO volatile
#define TCP_SERVER_RX_BUFSIZE	2000	//定义tcp server最大接收数据长度
//u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];
unsigned char tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];
unsigned long ipadd111;
char dist_str[40];
void Time_Update(void);
void Delay(uint32_t nCount);
void System_Periodic_Handle(void);
void Delay_s(unsigned long ulVal); /* 利用循环产生一定的延时 */
#ifdef __cplusplus
}
#endif
#endif 
