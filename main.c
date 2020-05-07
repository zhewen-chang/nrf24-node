#include <reg24le1.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal_nrf.h"
#include "hal_nrf_hw.h"
#include "hal_clk.h"
#include "hal_delay.h"
#include "hal_uart.h"
#include "user_config.h"
xdata bool slp_flag;
xdata bool wk_flag;
void main()
{	
	IO_Init();
	hal_uart_init(UART_BAUD_9K6);  
	hal_clk_set_16m_source(HAL_CLK_XOSC16M);   				/* Use external 16MHz crystal*/

	while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M);
	
	RfCofig();

	printDetails();

	sprintf(tx_payload, "L002R");
	RF_SendDat();

	slp_flag = false;
	wk_flag = false;

	while(1)
	{
		if(slp_flag==true)
		{
			sprintf(tx_payload,"L002S");
			RF_SendDat();
			nrf_sleep();
		}
		if(wk_flag==true){
			sprintf(tx_payload,"L002W");
			RF_SendDat();
		}
		sprintf(tx_payload, "L002A");
		RF_SendDat();
		D2=0;
		delay_ms(500);
		D2=1;
		delay_ms(500);
	}                                           
} 

void rf_irq() interrupt INTERRUPT_RFIRQ
{
	uint8_t irq_flags;
	
	irq_flags = hal_nrf_get_clear_irq_flags();					/* Read and clear RF interrupt flag */
	
	if(irq_flags & ((1<<HAL_NRF_TX_DS)))				 			/* Transimmter finish 				*/
	{
		radio_busy = false;			
	}

	if(irq_flags & ((1<<HAL_NRF_MAX_RT)))				 			/* Re-transimmter 					*/
	{
		radio_busy = false;
		hal_nrf_flush_tx();
	}
		
		if(irq_flags & (1<<HAL_NRF_RX_DR)) 							/* Rx interrupt 					*/
	{
		while(!hal_nrf_rx_fifo_empty()) 							/* Read data 						*/
		{
			PipeAndLen = hal_nrf_read_rx_payload(rx_payload);
		}
				hal_nrf_flush_rx();
		RF_Recv_Flag = 1;  										/* Set Receive OK flag 				*/
   	}
}															

void wuop_irq() interrupt INTERRUPT_WUOPIRQ
{
	slp_flag = !slp_flag;
	
	if (!slp_flag) {
		nrf_wakeup();
		wk_flag=true;

	}
}