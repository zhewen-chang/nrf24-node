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
	P0DIR	= 0x15;       /*open uart*/
	WUCON 	= 0xF3;     //which interrupt to wakeup
	WUOPC0 	= 0x01;     //which pin use to WU interrupt
	WUPIN 	= 0x01;		//open WU interrupt				
}

uint8_t my_nrf_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t retval;
	/*lint -esym(550,dummy) symbol not accessed*/
	/*lint -esym(438,dummy) last assigned value not used*/
	/*lint -esym(838,dummy) previously assigned value not used*/
	uint8_t volatile dummy;

	CSN_LOW();

	HAL_NRF_HW_SPI_WRITE((W_REGISTER + reg));
	while(HAL_NRF_HW_SPI_BUSY) {}
	retval = HAL_NRF_HW_SPI_READ();

	HAL_NRF_HW_SPI_WRITE(value);
	while(HAL_NRF_HW_SPI_BUSY) {}
	dummy = HAL_NRF_HW_SPI_READ();

	CSN_HIGH();

	return retval;
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

void nrf_sleep(void)
{	
	hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);	 					/* RF power down   									*/			
}

void nrf_wakeup(void)
{
	RfCofig();
}

void RfCofig(void)
{
	RFCKEN = 1;	     												/* Enable RF clock									*/

	hal_nrf_close_pipe(HAL_NRF_ALL);            					/* Close all chennel first 							*/
	hal_nrf_open_pipe(HAL_NRF_PIPE0,true);	   						/* Open all chennel							    	*/
	hal_nrf_set_operation_mode(HAL_NRF_PTX);						/* Mode Rx 											*/
	hal_nrf_set_rf_channel(RF_CHANNEL);		    		 			/* RF channel: 76, Tx and Rx must in same channel 	*/
	hal_nrf_set_datarate(HAL_NRF_1MBPS);	  						/* RF Speed: 1MBPS 									*/
	hal_nrf_set_output_power(HAL_NRF_18DBM);	  						/* Power: 0DBM 										*/
	hal_nrf_set_crc_mode(HAL_NRF_CRC_16BIT);       					/* CRC Mode: 16bit, must same as Tx					*/
	hal_nrf_set_rx_payload_width(HAL_NRF_PIPE0, RX_PAYLOAD_LEN); 	/* Set Rx length 									*/
	hal_nrf_set_power_mode(HAL_NRF_PWR_UP);	    					/* Power up 										*/
	hal_nrf_setup_dynamic_payload(63);                             	/* DYNP=0x3f                                        */
	hal_nrf_enable_ack_payload(0);                                  /* open enable ack payload                          */
	hal_nrf_enable_dynamic_payload(1);
	hal_nrf_enable_continious_wave (0);

	RF = 1;      													/* Enable RF interrupt 								*/
	EA = 1;	     													/* Enable all interrupts 							*/
	
	CE_HIGH();  													/* Enable Rx 										*/
}

void RF_SendData(void)
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
}

void putstr(char *ch)                                              
{
	while (*ch != '\0') {
		hal_uart_putchar((uint8_t) *ch++);
	}
}
 /*print*/
int32_t debugs(const char *fmt, ...)                             
{
	int32_t printed;
	char printf_buf[300];
	va_list args;

	va_start(args, fmt);
	printed = vsprintf(printf_buf, fmt, args);
	va_end(args);

	putstr(printf_buf);

	return printed;
}

hal_nrf_datarate_t hal_nrf_get_datarate(void) 
{
	hal_nrf_datarate_t result;
	uint8_t rf_setup = my_nrf_read_reg(RF_SETUP) & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

    if (rf_setup == _BV(RF_DR_LOW)) {
        // '10' = 250KBPS
        result = HAL_NRF_250KBPS;
    } else if (rf_setup == _BV(RF_DR_HIGH)) {
        // '01' = 2MBPS
        result = HAL_NRF_2MBPS;
    } else {
        // '00' = 1MBPS
        result = HAL_NRF_1MBPS;
    }
    return result;
}

uint8_t hal_nrf_get_pa_power(void)
{
	return (my_nrf_read_reg(RF_SETUP) & (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH))) >> 1;
}

hal_nrf_crc_mode_t hal_nrf_get_crc_length(void)
{
	hal_nrf_crc_mode_t result;
	uint8_t config = my_nrf_read_reg(CONFIG) & (_BV(CRCO) | _BV(EN_CRC));

	if (config == _BV(CRCO)) {
        // '10' = 8BIT
        result = HAL_NRF_CRC_8BIT;
    } else if (config == (_BV(CRCO) | _BV(EN_CRC))) {
        // '11' = 16BIT
        result = HAL_NRF_CRC_16BIT;
    } else {
        // '00' = OFF
        result = HAL_NRF_CRC_OFF;
    }
	return result;
}

static void print_status(void)
{
	uint8_t status = my_nrf_read_reg(STATUS);

	debugs("STATUS\t\t = 0x%b02x RX_DR=%bu TX_DS=%bu MAX_RT=%bu RX_P_NO=%bu TX_FULL=%bu\r\n",
			 status,
			(status & _BV(RX_DR)) ? 1 : 0,
			(status & _BV(TX_DS)) ? 1 : 0,
			(status & _BV(MAX_RT)) ? 1 : 0,
			(status >> RX_P_NO) & 0x07,
			(status & _BV(TX_FULL)) ? 1 : 0);
}

static void print_byte_register(const char *name, uint8_t reg, uint8_t counts)
{
	debugs("%s\t = ", name);
	while (counts--) {
		debugs("0x%b02x ", my_nrf_read_reg(reg++));
	}
	debugs("\r\n");
}

static void print_address_register(const char *name, uint8_t reg, uint8_t counts)
{
	uint8_t buf[5], *bufptr;

	debugs("%s\t =", name);
	while (counts--) {
		debugs(" 0x");
		hal_nrf_get_address(reg++, buf);

		bufptr = buf + 5;
		while (--bufptr >= buf) {
			debugs("%b02x", *bufptr);
		}
	}
	debugs("\r\n");
}

void print_details(void)
{
	const char data_rate[][8] = {"1Mbps", "2Mbps", "250Kbps"};
	const char crc_len[][8] = {"OFF", "8BIT", "16BIT"};
	const char pa_power[][8] = {"PA_MIN", "PA_LOW", "PA_HIGH", "PA_MAX"};

	debugs("\r\n================ NRF Configuration ===============\r\n");
	print_status();

	print_address_register("RX_ADDR_P0-1", HAL_NRF_PIPE0, 2);
	print_byte_register("RX_ADDR_P2-5", RX_ADDR_P2, 4);
	print_address_register("TX_ADDR\t", HAL_NRF_TX, 1);

	print_byte_register("RX_PW_P0-P5", RX_PW_P0, 6);
	print_byte_register("EN_AA\t\t", EN_AA, 1);
	print_byte_register("EN_RXADDR\t", EN_RXADDR, 1);
	print_byte_register("RF_CH\t\t", RF_CH, 1);
	print_byte_register("RF_SETUP\t", RF_SETUP, 1);
	print_byte_register("CONFIG\t", CONFIG, 1);
	print_byte_register("DYNPD/FEATURE", DYNPD, 2);

	debugs("Data Rate\t\t = %s\r\n", data_rate[hal_nrf_get_datarate()]);
	debugs("Model\t\t\t = nRF24LE1\r\n");
	debugs("CRC Length\t\t = %s\r\n", crc_len[hal_nrf_get_crc_length()]);
	debugs("PA Power\t\t = %s\r\n", pa_power[hal_nrf_get_pa_power()]);
}