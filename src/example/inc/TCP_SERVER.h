#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_
//#include "ip_addr.h"

#define IP4_ADDR1(ipaddr, a,b,c,d) \
        (ipaddr)->addr = htonl(((u32_t)((a) & 0xff) << 24) | \
                               ((u32_t)((b) & 0xff) << 16) | \
                               ((u32_t)((c) & 0xff) << 8) | \
                                (u32_t)((d) & 0xff))
#define TCP_LOCAL_PORT   1030


err_t TCP_Client_Send_Data(struct tcp_pcb *cpcb,unsigned char *buff,unsigned int length);
struct tcp_pcb *Check_TCP_Connect(unsigned long remoteIP);
void TCP_server_init(void);//tcp服务器初始化

#endif
