/***********************************************************************
文件名称：TCP_SERVER.C
功    能：完成TCP的数据收发
编写时间：2013.4.25
编 写 人：赵
注    意：
***********************************************************************/
#include "main.h"
#include "TCP_SERVER.h"
#include "TCP_CLIENT.h"
#include "tcp.h"

typedef unsigned char u8;
const char *tcp_server_sendbuf="ENC28J60 TCP Server send data\r\n";
/***********************************************************************
函数名称：tcp_server_recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
功    能：TCP数据接收和发送
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：这是一个回调函数，当一个TCP段到达这个连接时会被调用
***********************************************************************/
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
{
	char *data;
	char *data_temp;
	struct pbuf *q;
	u32 data_len = 0;
	if(p != NULL)
	{
		tcp_recved(pcb, p->tot_len);				//获取数据长度 tot_len：tcp数据块的长度
		/******将数据原样返回*******************/
		tcp_write(pcb,p->payload,p->tot_len,0); 	// payload为TCP数据块的起始位置
		memset(tcp_server_recvbuf, 0, TCP_SERVER_RX_BUFSIZE);  //数据接收缓冲区清零
		for (q = p; q != NULL; q = q->next)  //遍历完整个pbuf链表
		{
			if (q->len > (TCP_SERVER_RX_BUFSIZE - data_len)) memcpy(tcp_server_recvbuf + data_len, q->payload, (TCP_SERVER_RX_BUFSIZE - data_len));//拷贝数据
			else memcpy(tcp_server_recvbuf + data_len, q->payload, q->len);
			data_len += q->len;
			if (data_len > TCP_SERVER_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出
		}
		tcp_recved(pcb, p->tot_len);//用于获取接收数据,通知LWIP可以获取更多数据
		pbuf_free(p);  	//释放内存
    	err = ERR_OK;
	}
	else
	{
		tcp_close(pcb); 											/* 作为TCP服务器不应主动关闭这个连接？ */
	}
	err = ERR_OK;
	return err;
}
/***********************************************************************
函数名称：tcp_server_accept(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
功    能：回调函数
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：这是一个回调函数，当一个连接已经接受时会被调用
***********************************************************************/
static err_t tcp_server_accept(void *arg,struct tcp_pcb *pcb,err_t err)
{
	tcp_setprio(pcb, TCP_PRIO_MIN); 		/* 设置回调函数优先级，当存在几个连接时特别重要,此函数必须调用*/
	tcp_recv(pcb,tcp_server_recv); 				/* 设置TCP段到时的回调函数 */
	err = ERR_OK;
	return err;
}


/***********************************************************************
函数名称：TCP_server_init(void)
功    能：完成TCP服务器的初始化，主要是使得TCP通讯快进入监听状态
输入参数：
输出参数：
编写时间：2013.4.25
编 写 人：
注    意：
***********************************************************************/
void TCP_server_init(void)
{
	struct tcp_pcb *pcb;
	pcb = tcp_new(); 								/* 建立通信的TCP控制块(pcb) */
	tcp_bind(pcb,IP_ADDR_ANY,TCP_LOCAL_PORT); 	    /* 绑定本地IP地址和端口号（作为tcp服务器） */
	pcb = tcp_listen(pcb); 							/* 进入监听状态 */
	tcp_accept(pcb,tcp_server_accept); 			    /* 设置有连接请求时的回调函数 */
}


unsigned char connect_flag = 0;
err_t TCP_Client_Send_Data(struct tcp_pcb *cpcb,unsigned char *buff,unsigned int length)
{
	err_t err;
	err = tcp_write(cpcb,buff,length,TCP_WRITE_FLAG_COPY);	//发送数据
	tcp_output(cpcb);
	//tcp_close(cpcb);				//发送完数据关闭连接,根据具体情况选择使用
	return err;
}                  /***************TCP客户端发送数据函数************************************/

struct tcp_pcb *Check_TCP_Connect(unsigned long remoteIP)
{
	struct tcp_pcb *cpcb = 0;
	connect_flag = 0;
	for(cpcb = tcp_active_pcbs;cpcb != NULL; cpcb = cpcb->next)
	{
//		if(cpcb->local_port == TCP_LOCAL_PORT && cpcb->remote_port == TCP_SERVER_PORT)		//如果TCP_LOCAL_PORT端口指定的连接没有断开
//		if(cpcb -> state == ESTABLISHED)
		if((cpcb->remote_ip.addr) == remoteIP)
		{
			//ipadd111=cpcb->remote_ip.addr;
			//ipadd111=cpcb->local_ip.addr;
			//sprintf(dist_str, "%x",ipadd111);
			//TcpTx(dist_str, 15);
			connect_flag = 1;  						//连接标志
			break;
	    }
    }
	return cpcb;
}
//#define IP4_ADDR(ipaddr, a,b,c,d) \
//        (ipaddr)->addr = htonl(((u32_t)((a) & 0xff) << 24) | \
//                               ((u32_t)((b) & 0xff) << 16) | \
//                               ((u32_t)((c) & 0xff) << 8) | \
//                                (u32_t)((d) & 0xff))
