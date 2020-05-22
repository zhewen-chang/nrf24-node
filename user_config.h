#ifndef _USER_CONF_H
#define _USER_CONF_H

#define D1 				P00 				/* LED D1 			*/
#define D2 				P01 				/* LED D2 			*/
#define S1 				P12 				/* BUTTON S1 		*/
#define S2 				P13 				/* BUTTON S2 		*/
#define BZ 				P06					/* BUZZER 			*/

#define TX_PAYLOAD_LEN 	5  					/* Tx length     	*/
#define RF_CHANNEL 		76 					/* RF channel    	*/
#define RX_PAYLOAD_LEN 	2 					/* Rx length  	 	*/ 

/* extend for hal */
#define _BV(bit) 		(1 << (bit))
#define RX_P_NO 		1

#define RF_DR_LOW   	5
#define RF_DR_HIGH  	3
#define RF_PWR_LOW  	1
#define RF_PWR_HIGH 	2

extern xdata bool  radio_busy;
extern uint16_t PipeAndLen;
extern uint8_t RF_Recv_Flag;
extern xdata uint8_t  tx_payload[32];
extern xdata uint8_t  rx_payload[32];

void RfCofig(void);
void IO_Init(void);
void RF_SendDat(void);
void putstr(char *ch);
int32_t debugs(const char *fmt, ...);
void print_details(void);
void nrf_sleep(void);
void nrf_wakeup(void);

/* hal extend function */
hal_nrf_datarate_t hal_nrf_get_datarate(void);
uint8_t hal_nrf_get_pa_power(void);
hal_nrf_crc_mode_t hal_nrf_get_crc_length(void);

#endif