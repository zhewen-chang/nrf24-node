#include <reg24le1.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "hal_nrf.h"
#include "hal_nrf_hw.h"
#include "hal_clk.h"
#include "hal_delay.h"
#include "hal_uart.h"
#include "user_config.h"

xdata bool  radio_busy;
uint16_t PipeAndLen;
uint8_t RF_Recv_Flag;
xdata uint8_t  tx_payload[32];
xdata uint8_t  rx_payload[32];

void IO_Init(void)
{	
	P0DIR = 0x90;		/* P0.7, P0.4 input; others output. (1001 0000) */
	P1DIR = 0xFE;     	/* P1.2, P1.3 input for buttons 				*/
	D1 = 1;             /* LED 1 Off 									*/
	D2 = 1;	            /* LED 2 off 									*/
	BZ = 0;				/* BUZZER off									*/
}

void nrf_register(void){





}

uint8_t my_nrf_read_reg(uint8_t reg)
{
  uint8_t temp;

  CSN_LOW();

  HAL_NRF_HW_SPI_WRITE(reg);
  while(HAL_NRF_HW_SPI_BUSY) {}
  temp = HAL_NRF_HW_SPI_READ();

  HAL_NRF_HW_SPI_WRITE(0U);
  while(HAL_NRF_HW_SPI_BUSY) {}
  temp = HAL_NRF_HW_SPI_READ();

  CSN_HIGH();

  return temp;
}

void RfCofig(void)
{
	RFCKEN = 1;	     												/* Enable RF clock									*/

	hal_nrf_close_pipe(HAL_NRF_ALL);            					/* Close all chennel first 							*/
	hal_nrf_open_pipe(HAL_NRF_ALL,true);	   						/* Open  a11 chennel							    */
	hal_nrf_set_operation_mode(HAL_NRF_PTX);						/* Mode Rx 											*/
	hal_nrf_set_rf_channel(RF_CHANNEL);		    		 			/* RF channel: 76, Tx and Rx must in same channel 	*/
	hal_nrf_set_datarate(HAL_NRF_1MBPS);	  						/* RF Speed: 1MBPS 								*/
	hal_nrf_set_output_power(HAL_NRF_0DBM);	  						/* Power: 0DBM 										*/
	hal_nrf_set_crc_mode(HAL_NRF_CRC_16BIT);       					/* CRC Mode: 16bit, must same as Tx					*/
	hal_nrf_set_rx_payload_width(HAL_NRF_PIPE0, RX_PAYLOAD_LEN); 	/* Set Rx length 									*/
	hal_nrf_set_rx_payload_width(HAL_NRF_PIPE1, RX_PAYLOAD_LEN); 	/* Set Rx length 									*/
	hal_nrf_set_power_mode(HAL_NRF_PWR_UP);	    					/* Power up 										*/
	hal_nrf_setup_dynamic_payload(63);                             	/*DYNP=0x3f                                          */
	hal_nrf_enable_ack_payload(1);                                  /*open enable ack payload                            */
	hal_nrf_enable_dynamic_payload(1);
	hal_nrf_enable_continious_wave (0);
	hal_nrf_set_address(HAL_NRF_PIPE0,"1Node");                     /*set pipe0 address*/  
	hal_nrf_set_address(HAL_NRF_PIPE1,"2Node");                     /*set pipe1 address*/
	hal_nrf_set_address(HAL_NRF_TX, "gaway");                       /*set TX address*/
	
	
	RF = 1;      													/* Enable RF interrupt 								*/
	EA = 1;	     													/* Enable all interrupts 							*/
	
	CE_HIGH();  													/* Enable Rx 										*/
}

void RF_SendDat(void)
{
	CE_LOW();
	hal_nrf_set_operation_mode(HAL_NRF_PTX);   						/* Mode Tx 						*/
	hal_nrf_write_tx_payload(tx_payload,TX_PAYLOAD_LEN); 
	CE_PULSE();	            										/* RF transmission 				*/
    radio_busy = true;

	while(radio_busy)		   										/* Wait for transmission done 	*/
		;

	hal_nrf_set_operation_mode(HAL_NRF_PRX);   						/* Mode: Tx 					*/
	CE_HIGH();
	//debugs("CONFIG\t\t= 0x%b02x\r\n",my_nrf_read_reg (CONFIG));
}

void putstr(char *ch)
{
	while (*ch != '\0') {
		hal_uart_putchar((uint8_t) *ch++);
	}
}

int debugs(const char *fmt, ...)
{
	int printed;
	char printf_buf[300];
	va_list args;

	va_start(args, fmt);
	printed = vsprintf(printf_buf, fmt, args);
	va_end(args);

	putstr(printf_buf);

	return printed;
}

typedef union {
  uint8_t value;
	struct {
		const uint8_t : 1;
		uint8_t rf_pwr : 2;
		uint8_t rf_dr_high : 1;
		uint8_t pll_lock : 1;
		uint8_t rf_dr_low : 1;
    const uint8_t : 1;
    uint8_t cont_wave : 1;
	} bits;
} rf_setup_t;

typedef union {
  uint8_t value;
	struct {
		uint8_t prim_rx : 1;
		uint8_t pwr_up : 1;
		uint8_t crc0 : 1;
		uint8_t en_crc : 1;
		uint8_t mask_max_rt : 1;
		uint8_t mask_tx_ds : 1;
		uint8_t mask_rx_dr : 1;
		const uint8_t : 1;
	} bits;
} config_t;

typedef union {
  uint8_t value;
	struct {
		uint8_t tx_full : 1;
		uint8_t rx_p_no : 3;
		uint8_t max_rt : 1;
		uint8_t tx_ds : 1;
		uint8_t rx_dr : 1;
		const uint8_t : 1;
	} bits;
} status_t;

void printDetails(void){
	rf_setup_t setup;
	config_t config;
	status_t status;
	uint8_t p0[5], p1[5], p2, p3, p4, p5, tx[5];
	
	setup.value=my_nrf_read_reg (RF_SETUP);
	config.value=my_nrf_read_reg (CONFIG);
	status.value=my_nrf_read_reg(STATUS);


	debugs("\r\n================ NRF Configuration ================\r\n");
	debugs("STATUS\t\t= 0x%b02x RX_DR=%bu TX_DS=%bu MAX_RT=%bu RX_P_NO=%bu TX_FULL=%bu\r\n",status.value,status.bits.rx_dr,status.bits.tx_ds,status.bits.max_rt,status.bits.rx_p_no,status.bits.tx_full);
	hal_nrf_get_address(HAL_NRF_PIPE0, p0);
	hal_nrf_get_address(HAL_NRF_PIPE1, p1);
    debugs("RX_ADDR_P0-1\t= 0x%b02x%b02x%b02x%b02x%b02x 0x%b02x%b02x%b02x%b02x%b02x\r\n", p1[4], p1[3], p1[2], p1[1], p1[0], p0[4], p0[3], p0[2], p0[1], p0[0]);
	
	hal_nrf_get_address(HAL_NRF_PIPE2, &p2);
	hal_nrf_get_address(HAL_NRF_PIPE3, &p3);
	hal_nrf_get_address(HAL_NRF_PIPE4, &p4);
	hal_nrf_get_address(HAL_NRF_PIPE5, &p5);
    debugs("RX_ADDR_P2-5\t= 0x%b02x 0x%b02x 0x%b02x 0x%b02x\r\n", my_nrf_read_reg(RX_ADDR_P2), p3, p4, p5);

	hal_nrf_get_address(HAL_NRF_TX, tx);
    debugs("TX_ADDR\t\t= 0x%b02x%b02x%b02x%b02x%b02x\r\n", tx[4], tx[3], tx[2], tx[1], tx[0]);
	debugs("RX_PW_P0-P5\t\t= 0x%b02x 0x%b02x 0x%b02x 0x%b02x 0x%b02x 0x%b02x\r\n",my_nrf_read_reg(RX_PW_P0),my_nrf_read_reg(RX_PW_P1),my_nrf_read_reg(RX_PW_P2),my_nrf_read_reg(RX_PW_P3),my_nrf_read_reg(RX_PW_P4),my_nrf_read_reg(RX_PW_P5));
	debugs("EN_AA\t\t\t= 0x%b02x\r\n", my_nrf_read_reg(EN_AA));
	debugs("EN_RXADDR\t\t= 0x%b02x\r\n",my_nrf_read_reg (EN_RXADDR));
	debugs("RF_CH\t\t\t= 0x%b02x\r\n", my_nrf_read_reg (RF_CH));
	debugs("RF_SETUP\t\t= 0x%b02x\r\n",my_nrf_read_reg (RF_SETUP));
	debugs("CONFIG\t\t= 0x%b02x\r\n",my_nrf_read_reg (CONFIG));
	debugs("DYNPD/FEATURE\t= 0x%b02x 0x%b02x\r\n",my_nrf_read_reg (DYNPD),my_nrf_read_reg(FEATURE));
	if((setup.bits.rf_dr_high|setup.bits.rf_dr_low<<1)==0)
	{
		debugs("DATA RATE\t\t= 1Mbps\r\n");
	}
	else if((setup.bits.rf_dr_high|setup.bits.rf_dr_low<<1)==1)
	{
		debugs("DATA RATE\t\t= 2Mbps\r\n");
	}
	else if((setup.bits.rf_dr_high|setup.bits.rf_dr_low<<1)==2)
	{
		debugs("DATA RATE\t\t= 250kbps\r\n");
	}
	else
	{
		debugs("DATA RATE\t\t= It's BUGS\r\n");
	}

	debugs("Model\t\t\t= nRF24LE1\r\n");
	
	if((config.bits.en_crc)==0)
	{
		debugs("CRC LENGTH\t\t= OFF\r\n");
	}
	else if((config.bits.crc0<<1|config.bits.en_crc)==1)
	{
		debugs("CRC LENGTH\t\t= 8BIT\r\n");
	}
	else if((config.bits.crc0<<1|config.bits.en_crc)==3)
	{
		debugs("CRC LENGTH\t\t= 16BIT\r\n");
	}

	if((setup.bits.rf_pwr)==0)
	{
		debugs("PA Power\t\t= PA_OFF\r\n");
	}
	else if((setup.bits.rf_pwr)==1)
	{
		debugs("PA Power\t\t= PA_LOW\r\n");
	}
	else if((setup.bits.rf_pwr)==2)
	{
		debugs("PA Power\t\t= PA_HIGH\r\n");
	}
	else
	{
		debugs("PA Power\t\t= PA_MAX\r\n");
	}

	

}