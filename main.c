#include "main.h"

xdata bool slp_flag;
xdata bool wk_flag;

void main()
{	
	IO_Init();
	hal_uart_init(UART_BAUD_9K6);  
	hal_clk_set_16m_source(HAL_CLK_XOSC16M);   				/* Use external 16MHz crystal*/

	while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M);
	
	RfCofig();
	hal_nrf_set_address(HAL_NRF_PIPE0, PIPE_NO);                    /* set pipe0 address				  				  */  
	hal_nrf_set_address(HAL_NRF_TX, PIPE_NO);                       /* set TX address									*/

	printDetails();

	sprintf(tx_payload, "L%sR",NODE_ID);
	RF_SendDat();

	slp_flag = false;
	wk_flag = false;

	while(1)
	{
		if(slp_flag==true)
		{
			sprintf(tx_payload,"L%sS",NODE_ID);
			RF_SendDat();
			nrf_sleep();
		}
		if(wk_flag==true){
			hal_nrf_set_address(HAL_NRF_PIPE0, PIPE_NO);                    /* set pipe0 address				  				  */  
			hal_nrf_set_address(HAL_NRF_TX, PIPE_NO);                       /* set TX address									*/
			sprintf(tx_payload,"L%sW",NODE_ID);
			RF_SendDat();
			wk_flag=false;
		}
		sprintf(tx_payload, "L%sA",NODE_ID);
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