#include "main.h"

xdata bool slp_flag;
xdata bool wk_flag;
char nodeid[5];
char nodepipe[5];
char pipe_no[6]="0pipe";

void main()
{	
	IO_Init();
	hal_uart_init(UART_BAUD_9K6);  
	hal_clk_set_16m_source(HAL_CLK_XOSC16M);   				/* Use external 16MHz crystal*/

	while(hal_clk_get_16m_source() != HAL_CLK_XOSC16M);
	
	RfCofig();
	hal_nrf_set_address(HAL_NRF_PIPE0, "gaway");                    /* set pipe0 address				  				  */  
	hal_nrf_set_address(HAL_NRF_TX, pipe_no);                       /* set TX address									*/

	print_details();

	tx_payload[0] = 'R';
	tx_payload[1] = 'G';
	tx_payload[2] = 'S';
	tx_payload[3] = 0;
	RF_SendDat();

	while (RF_Recv_Flag != 1);
	RF_Recv_Flag = 0;
	strncpy(nodeid,rx_payload,3);
	strncpy(nodepipe,rx_payload+4,1);
	nodeid[3]='\0';
	nodepipe[1]='\0';
	sprintf(pipe_no,"%spipe",nodepipe);

	hal_nrf_set_address(HAL_NRF_TX, pipe_no);                       /* set TX address									*/
	debugs("%s\r\n",rx_payload);

	slp_flag = false;
	wk_flag = false;

	while(1)
	{
		if(slp_flag==true)
		{
			sprintf(tx_payload,"L%sS",nodeid);
			RF_SendDat();
			nrf_sleep();
		}
		if(wk_flag==true){
			hal_nrf_set_address(HAL_NRF_PIPE0, pipe_no);                    /* set pipe0 address				  				  */  
			hal_nrf_set_address(HAL_NRF_TX, pipe_no);                       /* set TX address									*/
			sprintf(tx_payload,"L%sW",nodeid);
			RF_SendDat();
			wk_flag=false;
		}
		sprintf(tx_payload, "L%sA",nodeid);
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