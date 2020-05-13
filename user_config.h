#ifndef _USER_CONF_H
#define _USER_CONF_H

#define D1 P00 				/* LED D1 			*/
#define D2 P01 				/* LED D2 			*/
#define S1 P12 				/* BUTTON S1 		*/
#define S2 P13 				/* BUTTON S2 		*/
#define BZ P06				/* BUZZER 			*/

#define TX_PAYLOAD_LEN 	5  	/* Tx length     	*/
#define RF_CHANNEL 		76 	/* RF channel    	*/
#define RX_PAYLOAD_LEN 	2 	/* Rx length  	 	*/ 

#define NODE_ID "001"
//#define PWRDWN 0xA4

extern xdata bool  radio_busy;
extern uint16_t PipeAndLen;
extern uint8_t RF_Recv_Flag;
extern xdata uint8_t  tx_payload[32];
extern xdata uint8_t  rx_payload[32];

void RfCofig(void);
void IO_Init(void);
void RF_SendDat(void);
void putstr(char *ch);
int debugs(const char *fmt, ...);
void printDetails(void);
void nrf_sleep(void);
void nrf_wakeup(void);

#endif