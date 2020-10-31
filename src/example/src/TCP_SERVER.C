/***********************************************************************
�ļ����ƣ�TCP_SERVER.C
��    �ܣ����TCP�������շ�
��дʱ�䣺2013.4.25
�� д �ˣ���
ע    �⣺
***********************************************************************/
#include "main.h"
#include "TCP_SERVER.h"
#include "TCP_CLIENT.h"
#include "tcp.h"

typedef unsigned char u8;
const char *tcp_server_sendbuf="ENC28J60 TCP Server send data\r\n";
/***********************************************************************
�������ƣ�tcp_server_recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
��    �ܣ�TCP���ݽ��պͷ���
���������
���������
��дʱ�䣺2013.4.25
�� д �ˣ�
ע    �⣺����һ���ص���������һ��TCP�ε����������ʱ�ᱻ����
***********************************************************************/
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
{
	char *data;
	char *data_temp;
	struct pbuf *q;
	u32 data_len = 0;
	if(p != NULL)
	{
		tcp_recved(pcb, p->tot_len);				//��ȡ���ݳ��� tot_len��tcp���ݿ�ĳ���
		/******������ԭ������*******************/
		tcp_write(pcb,p->payload,p->tot_len,0); 	// payloadΪTCP���ݿ����ʼλ��
		memset(tcp_server_recvbuf, 0, TCP_SERVER_RX_BUFSIZE);  //���ݽ��ջ���������
		for (q = p; q != NULL; q = q->next)  //����������pbuf����
		{
			if (q->len > (TCP_SERVER_RX_BUFSIZE - data_len)) memcpy(tcp_server_recvbuf + data_len, q->payload, (TCP_SERVER_RX_BUFSIZE - data_len));//��������
			else memcpy(tcp_server_recvbuf + data_len, q->payload, q->len);
			data_len += q->len;
			if (data_len > TCP_SERVER_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����
		}
		tcp_recved(pcb, p->tot_len);//���ڻ�ȡ��������,֪ͨLWIP���Ի�ȡ��������
		pbuf_free(p);  	//�ͷ��ڴ�
    	err = ERR_OK;
	}
	else
	{
		tcp_close(pcb); 											/* ��ΪTCP��������Ӧ�����ر�������ӣ� */
	}
	err = ERR_OK;
	return err;
}
/***********************************************************************
�������ƣ�tcp_server_accept(void *arg, struct tcp_pcb *pcb,struct pbuf *p,err_t err)
��    �ܣ��ص�����
���������
���������
��дʱ�䣺2013.4.25
�� д �ˣ�
ע    �⣺����һ���ص���������һ�������Ѿ�����ʱ�ᱻ����
***********************************************************************/
static err_t tcp_server_accept(void *arg,struct tcp_pcb *pcb,err_t err)
{
	tcp_setprio(pcb, TCP_PRIO_MIN); 		/* ���ûص��������ȼ��������ڼ�������ʱ�ر���Ҫ,�˺����������*/
	tcp_recv(pcb,tcp_server_recv); 				/* ����TCP�ε�ʱ�Ļص����� */
	err = ERR_OK;
	return err;
}


/***********************************************************************
�������ƣ�TCP_server_init(void)
��    �ܣ����TCP�������ĳ�ʼ������Ҫ��ʹ��TCPͨѶ��������״̬
���������
���������
��дʱ�䣺2013.4.25
�� д �ˣ�
ע    �⣺
***********************************************************************/
void TCP_server_init(void)
{
	struct tcp_pcb *pcb;
	pcb = tcp_new(); 								/* ����ͨ�ŵ�TCP���ƿ�(pcb) */
	tcp_bind(pcb,IP_ADDR_ANY,TCP_LOCAL_PORT); 	    /* �󶨱���IP��ַ�Ͷ˿ںţ���Ϊtcp�������� */
	pcb = tcp_listen(pcb); 							/* �������״̬ */
	tcp_accept(pcb,tcp_server_accept); 			    /* ��������������ʱ�Ļص����� */
}


unsigned char connect_flag = 0;
err_t TCP_Client_Send_Data(struct tcp_pcb *cpcb,unsigned char *buff,unsigned int length)
{
	err_t err;
	err = tcp_write(cpcb,buff,length,TCP_WRITE_FLAG_COPY);	//��������
	tcp_output(cpcb);
	//tcp_close(cpcb);				//���������ݹر�����,���ݾ������ѡ��ʹ��
	return err;
}                  /***************TCP�ͻ��˷������ݺ���************************************/

struct tcp_pcb *Check_TCP_Connect(unsigned long remoteIP)
{
	struct tcp_pcb *cpcb = 0;
	connect_flag = 0;
	for(cpcb = tcp_active_pcbs;cpcb != NULL; cpcb = cpcb->next)
	{
//		if(cpcb->local_port == TCP_LOCAL_PORT && cpcb->remote_port == TCP_SERVER_PORT)		//���TCP_LOCAL_PORT�˿�ָ��������û�жϿ�
//		if(cpcb -> state == ESTABLISHED)
		if((cpcb->remote_ip.addr) == remoteIP)
		{
			//ipadd111=cpcb->remote_ip.addr;
			//ipadd111=cpcb->local_ip.addr;
			//sprintf(dist_str, "%x",ipadd111);
			//TcpTx(dist_str, 15);
			connect_flag = 1;  						//���ӱ�־
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