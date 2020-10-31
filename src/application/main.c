/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   main loop for the DecaRanging application
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
/* Includes */
#include "compiler.h"
#include <strings.h>
#include "instance.h"
#include "deca_types.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "string.h"
#include "stdio.h"
#include "main.h"
#include <CoOS.h>
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
uint32_t timingdelay;

/* All RX errors mask. */
#define SYS_STATUS_ALL_RX_ERR  (SYS_STATUS_RXPHE | SYS_STATUS_RXFCE | SYS_STATUS_RXRFSL | SYS_STATUS_RXSFDTO \
                                | SYS_STATUS_AFFREJ | SYS_STATUS_LDEERR)

/* User defined RX timeouts (frame wait timeout and preamble detect timeout) mask. */
#define SYS_STATUS_ALL_RX_TO   (SYS_STATUS_RXRFTO | SYS_STATUS_RXPTO)

/* All TX events mask. */
#define SYS_STATUS_ALL_TX      (SYS_STATUS_AAT | SYS_STATUS_TXFRB | SYS_STATUS_TXPRS | \
                                SYS_STATUS_TXPHS | SYS_STATUS_TXFRS )

unsigned long  remoteIp=0x600aa8c0ul;

void Delay(uint32_t nCount)
{
  /* Capture the current local time */
  timingdelay = LocalTime + nCount;

  /* wait until the desired delay finish */
  while(timingdelay > LocalTime)
  {
  }
}

/**
  * @brief  Updates the system local time
  * @param  None
  * @retval None
  */
void Time_Update(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
}

/**
  * @brief  Handles the periodic tasks of the system
  * @param  None
  * @retval None
  */
void System_Periodic_Handle(void)
{
  /* Update the LCD display and the LEDs status */
  /* Manage the IP address setting */
  Display_Periodic_Handle(LocalTime);

  /* LwIP periodic services are done here */
  LwIP_Periodic_Handle(LocalTime);
}

struct tcp_pcb *connect;

void TcpTx(unsigned long remote_Ip,unsigned char* tmpdata,int len)
{
	connect = Check_TCP_Connect(remote_Ip);
	{
	 TCP_Client_Send_Data(connect,tmpdata,len);	}
}

#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436

static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0, 0, 0};
static uint8 tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0,0, 0, 0xC2, 1,1,1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8 tx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define FINAL_MSG_POLL_TX_TS_IDX 10
#define FINAL_MSG_RESP_RX_TS_IDX 14
#define FINAL_MSG_FINAL_TX_TS_IDX 18
#define FINAL_MSG_TS_LEN 4
/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 20
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

#define UUS_TO_DWT_TIME 65536

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of the receiver, as programmed for the DW1000's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 150
/* This is the delay from Frame RX timestamp to TX reply timestamp used for calculating/setting the DW1000's delayed TX function. This includes the
 * frame length of approximately 2.66 ms with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS 3100
/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 2700
/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 8

/* Time-stamps of frames transmission/reception, expressed in device time units.
 * As they are 40-bit wide, we need to define a 64-bit int type to handle them. */
typedef unsigned long long uint64;
static uint64 poll_tx_ts;
static uint64 resp_rx_ts;
static uint64 final_tx_ts;

/* Declaration of static functions. */
static uint64 get_tx_timestamp_u64(void);
static uint64 get_rx_timestamp_u64(void);
static void final_msg_set_ts(uint8 *ts_field, uint64 ts);

static dwt_config_t config = {
    2,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_512,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    0,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    0,
    (512 + 8 - 8 + 50)    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

void sleep_ms(unsigned int time_ms)
{
    /* This assumes that the tick has a period of exactly one millisecond. See CLOCKS_PER_SEC define. */
    unsigned long end = portGetTickCount() + time_ms;
    while ((signed long)(portGetTickCount() - end) <= 0)
        ;
}


//


//
//void taskD(void* pdata) {
//	unsigned int led_num;
//
//	for (;;) {
//		led_num++;
//		CoTickDelay(2000);
//		TcpTx("IDDD",12);
//		CoTickDelay(2000);
//	}
//}
//void taskE(void* pdata) {
//	unsigned int led_num;
//
//	for (;;) {
//		led_num++;
//		CoTickDelay(2300);
//		TcpTx("IEEE",12);
//		CoTickDelay(2300);
//	}
//}
//void taskF(void* pdata) {
//	unsigned int led_num;
//
//	for (;;) {
//		led_num++;
//		CoTickDelay(5500);
//		TcpTx("IFFF",12);
//		CoTickDelay(5500);
//	}
//}
//void taskG(void* pdata) {
//	unsigned int led_num;
//
//	for (;;) {
//		led_num++;
//		CoTickDelay(4400);
//		TcpTx("IGGG",12);
//		CoTickDelay(4400);
//	}
//}
//void taskH(void* pdata) {
//	unsigned int led_num;
//
//	for (;;) {
//		led_num++;
//		CoTickDelay(1000);
//		TcpTx("IHHH",12);
//		CoTickDelay(3300);
//	}
//}
//
///*---------------------------- Symbol Define -------------------------------*/
#define STACK_SIZE_TASKA 128              /*!< Define "taskA" task size */
#define STACK_SIZE_TASKB 128              /*!< Define "taskA" task size */
#define STACK_SIZE_TASKC 128              /*!< Define "taskA" task size */
//#define STACK_SIZE_TASKD 128
//#define STACK_SIZE_TASKE 128              /*!< Define "taskA" task size */
//#define STACK_SIZE_TASKF 128              /*!< Define "taskA" task size */
//#define STACK_SIZE_TASKG 128             /*!< Define "taskA" task size */
//#define STACK_SIZE_TASKH 128
#define STACK_SIZE_TASKN 128
//
///*---------------------------- Variable Define -------------------------------*/
OS_STK     taskA_stk[STACK_SIZE_TASKA];	  /*!< Define "taskA" task stack */
OS_STK     taskB_stk[STACK_SIZE_TASKB];	  /*!< Define "taskB" task stack */
OS_STK     taskC_stk[STACK_SIZE_TASKC];	  /*!< Define "led" task stack   */
//OS_STK     taskD_stk[STACK_SIZE_TASKD];
//OS_STK     taskE_stk[STACK_SIZE_TASKE];	  /*!< Define "taskA" task stack */
//OS_STK     taskF_stk[STACK_SIZE_TASKF];	  /*!< Define "taskB" task stack */
//OS_STK     taskG_stk[STACK_SIZE_TASKG];	  /*!< Define "led" task stack   */
//OS_STK     taskH_stk[STACK_SIZE_TASKH];
OS_STK     TaskN_stk[STACK_SIZE_TASKN];
OS_TID TaskAID;
OS_TID TaskBID;
OS_TID TaskCID;
OS_TID TaskNID;
OS_EventID mailbox;

OS_MutexID mutexId;
//#define TCP_SERVER_RX_BUFSIZE	2000	//定义tcp server最大接收数据长度
//extern u8 tcp_server_recvbuf[TCP_SERVER_RX_BUFSIZE];
//OS_EventID semaphore;
//
////OS_TID AA;
////char dist_str1[40] = { 0 };
////unsigned char aaa;
//
void taskA(void* pdata) {
	unsigned int led_num=1;
	StatusType result;
	int pridat=1;
	for (;;) {
		led_num++;
		result = CoPostMail(mailbox, &tcp_server_recvbuf[0]);
		tcp_server_recvbuf[0]='\0';
		if (result != E_OK)
		{
			if (result == E_INVALID_ID)
			{
				//printf("Invalid event ID !\n");
			}
		}
		CoTickDelay(1000);
	}
}
void taskB(void* pdata) {
	unsigned int led_num;
	StatusType result;
	int a=20;
	int *h=&a;
	//sprintf(dist_str, "D%lu ",ipadd111);
	//TcpTx(dist_str, 15);
//	char ipadd111[]={'1','1','0','0','0','0','2'};
//	char *p;
//	p=&ipadd111;
	unsigned char *msg;
	for (;;) {
		led_num++;
		msg = CoPendMail(mailbox, 0, &result);
		if (result != E_OK) {
			if (result == E_INVALID_ID) {
				//TcpTx("Invalid event ID !\n",40);
				//printf("Invalid event ID !");
			}
		}
		else {
			if ((*msg) == '1') {
				CoEnterMutexSection(mutexId);
				TcpTx(remoteIp,"remote ip test", 20);
				dwt_writetxdata(sizeof(tx_resp_msg), tx_resp_msg, 0);
				dwt_writetxfctrl(sizeof(tx_resp_msg), 0);
				dwt_starttx(DWT_START_TX_IMMEDIATE);
				while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS))
				{
				}

				if (status_reg&SYS_STATUS_TXFRS)
				{
					//TcpTx("txok",20);
					CoTickDelay(1000);
				}
				CoLeaveMutexSection(mutexId);
			}
		}
	}
}
void taskC(void* pdata) {
	unsigned int led_num;
	mailbox = CoCreateMbox(EVENT_SORT_TYPE_FIFO);
	if (mailbox != E_OK)
	{
	if (mailbox == E_CREATE_FAIL)
	{
	   //printf("Create mailbox fail");
	}
	}
	else
	{
	   //printf("MailBox I");
	}
    mutexId=CoCreateMutex();
	CoExitTask();
}
void TaskN(void *pdata)
{

	TCP_server_init();
	CoExitTask();
}
#pragma GCC optimize ("O3")
int main(void)
{

		peripherals_init();
		spi_peripheral_init();
	    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
		SPI_ChangeRate(SPI_BaudRatePrescaler_32);
		if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
		{
			while (1)
			{ };
		}
		SPI_ChangeRate(SPI_BaudRatePrescaler_4);

			    /* Configure DW1000. See NOTE 5 below. */
		dwt_configure(&config,DWT_LOADANTDLY);
		dwt_setleds(1);
		LwIP_Init();
		//TCP_Client_Init(TCP_LOCAL_PORT,TCP_SERVER_PORT,TCP_SERVER_IP);
//		sleep_ms(2000);
//		TcpTx("TcpInitSucc",40);

	CoInitOS();				 /*!< Initial CooCox CoOS          */

							 /*!< Create three tasks	*/
	TaskAID=CoCreateTask(taskA, 0, 3, &taskA_stk[STACK_SIZE_TASKA - 1], STACK_SIZE_TASKA);
	TaskBID=CoCreateTask(taskB, 0, 2, &taskB_stk[STACK_SIZE_TASKB - 1], STACK_SIZE_TASKB);
	TaskCID=CoCreateTask(taskC, 0, 1, &taskC_stk[STACK_SIZE_TASKC - 1], STACK_SIZE_TASKC);
//	CoCreateTask(taskD, 0, 0, &taskD_stk[STACK_SIZE_TASKD - 1], STACK_SIZE_TASKD);
//	CoCreateTask(taskE, 0, 4, &taskE_stk[STACK_SIZE_TASKE - 1], STACK_SIZE_TASKE);
//	CoCreateTask(taskF, 0, 2, &taskF_stk[STACK_SIZE_TASKF - 1], STACK_SIZE_TASKF);
	//CoCreateTask(taskG, 0, 1, &taskG_stk[STACK_SIZE_TASKG - 1], STACK_SIZE_TASKG);
	TaskNID=CoCreateTask(TaskN, 0, 0, &TaskN_stk[STACK_SIZE_TASKN - 1], STACK_SIZE_TASKN);
	CoStartOS();

	}


static uint64 get_tx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readtxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn get_rx_timestamp_u64()
 *
 * @brief Get the RX time-stamp in a 64-bit variable.
 *        /!\ This function assumes that length of time-stamps is 40 bits, for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
static uint64 get_rx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;
    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn final_msg_set_ts()
 *
 * @brief Fill a given timestamp field in the final message with the given value. In the timestamp fields of the final
 *        message, the least significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to fill
 *         ts  timestamp value
 *
 * @return none
 */
static void final_msg_set_ts(uint8 *ts_field, uint64 ts)
{
    int i;
    for (i = 0; i < FINAL_MSG_TS_LEN; i++)
    {
        ts_field[i] = (uint8) ts;
        ts >>= 8;
    }
}






