/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2016-2016 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively “Software”) is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */

#include "common.h"
#include "uart.h"

#include "ioport.h"
#include "uart_serial.h"
#include "pdc.h"

#include <string.h>


/********************************* Defines ************************************/

/*
 * A couple of macros giving UART int handlers a fancier name to 
 * make this module more readable
 */
#define USART0_IRQ_HANDLER          FLEXCOM0_Handler
#define USART7_IRQ_HANDLER          FLEXCOM7_Handler


/********************************* Globals ************************************/
/** @brief UART object definition.
 *
 * Contains all the fields needed by the present API to handle an UART device.
 */
struct uart_gpio {
	uint32_t port;                                         /** < gpio port. 0xffffffff means not applicable */ 
	uint32_t pin_id;                                       /** < gpio pin number */ 
	uint32_t mode_mux;                                     /** < gpio mux function */
};

struct uart_mapping {
	Usart *                      uart_ip;                       /** < UART peripheral */
	Pdc *                        uart_pdc;                      /** < Peripheral DMA controller for targeted UART */
	uint32_t                     uart_it_nb;                    /** < Number of interrupt line for targeted UART */
	uint32_t                     uart_periph_id;                /** < Peripheral ID for targeted UART as defined by ATMEL specification */
	struct uart_gpio             uart_gpio[4];                  /** < GPIO configuration for RXD/TXD/RTS/CTS.
	                                                                               Gpio whose configuration port is 0xFFFFFFFF are ignored */
	volatile inv_uart_state_t   uart_tx_state;
	inv_uart_state_t            uart_rx_state;
	volatile uint8_t *           uart_rx_buffer;                /** < Pointer to the memory region used to store Rx bytes */
	volatile uint8_t *           uart_tx_buffer;
	uint16_t                     uart_rx_buffer_size;           /** < The size of the memory region used to store Rx bytes */
	uint16_t                     uart_tx_buffer_size;
	volatile uint16_t            uart_rx_buffer_tail;           /** < Index of the last read byte from the uart_rx_buffer */
	void                         (*tx_done_cb)(void * context); /** < Callback executed by UART DMA tx IRQ handler */
	void                         *tx_context;                   /** < Context passed to tx_interrupt_cb */
	
};

static struct uart_mapping um[2] = {

	{
		.uart_ip           = USART0,
		.uart_pdc          = NULL,
		.uart_it_nb        = FLEXCOM0_IRQn,
		.uart_periph_id    = ID_FLEXCOM0,
		.uart_gpio = {
			{ IOPORT_PIOA, PIO_PA9A_RXD0,  IOPORT_MODE_MUX_A },
			{ IOPORT_PIOA, PIO_PA10A_TXD0, IOPORT_MODE_MUX_A },
			{ IOPORT_PIOA, PIO_PA26A_RTS0, IOPORT_MODE_MUX_A },
			{ IOPORT_PIOA, PIO_PA25A_CTS0, IOPORT_MODE_MUX_A }
		},
		.uart_tx_state = INV_UART_STATE_RESET,
		.uart_rx_state = INV_UART_STATE_RESET,
		.uart_rx_buffer = NULL,
		.uart_tx_buffer = NULL,
		.uart_rx_buffer_size = 0,
		.uart_tx_buffer_size = 0,
		.uart_rx_buffer_tail = 0,
		.tx_done_cb = NULL,
		.tx_context = NULL
	},

	{
		.uart_ip = USART7,
		.uart_pdc = NULL,
		.uart_it_nb = FLEXCOM7_IRQn,
		.uart_periph_id = ID_FLEXCOM7,
		.uart_gpio = {
			{ IOPORT_PIOA, PIO_PA27B_RXD7, IOPORT_MODE_MUX_B },
			{ IOPORT_PIOA, PIO_PA28B_TXD7, IOPORT_MODE_MUX_B },
			{ 0xffffffff, 0, 0 }, /* no RTS on UART7 */
			{ 0xffffffff, 0, 0 }  /* no CTS on UART7 */
		},
		.uart_tx_state = INV_UART_STATE_RESET,
		.uart_rx_state = INV_UART_STATE_RESET,
		.uart_rx_buffer = NULL,
		.uart_tx_buffer = NULL,
		.uart_rx_buffer_size = 0,
		.uart_tx_buffer_size = 0,
		.uart_rx_buffer_tail = 0,
		.tx_done_cb = NULL,
		.tx_context = NULL
	}
};

/***************************** Private Prototypes *****************************/
static int uart_dma_rx(inv_uart_num_t uart);


/****************************** Public Functions ******************************/
int inv_uart_init(inv_uart_init_struct_t * uart_init)
{
	uint32_t i;
	usart_serial_options_t USART_InitStructure;
	inv_uart_num_t uart = uart_init->uart_num;
	
	/* Don't execute this function if UART is not under reset state */
	if( (um[uart].uart_tx_state != INV_UART_STATE_RESET) ||
		(um[uart].uart_rx_state != INV_UART_STATE_RESET))
		return INV_UART_ERROR_SUCCES;
	
	/* Check memory is passed in parameter for tx and rx  */
	if ((uart_init->tx_size == 0) && (uart_init->rx_size == 0))
			return INV_UART_ERROR_BAD_ARG;

	/* init structure */
	um[uart].uart_pdc = usart_get_pdc_base(um[uart].uart_ip);
	um[uart].uart_tx_state = INV_UART_STATE_IDLE;	
	um[uart].uart_rx_state = INV_UART_STATE_IDLE;
	um[uart].uart_rx_buffer = uart_init->rx_buffer;
	um[uart].uart_tx_buffer = uart_init->tx_buffer;
	um[uart].uart_rx_buffer_size = uart_init->rx_size;
	um[uart].uart_tx_buffer_size = uart_init->tx_size;
	um[uart].tx_done_cb = uart_init->tx_done_cb;
	um[uart].tx_context = uart_init->tx_context;
	
	/* Configure GPIO pins */
	for(i=0 ; i < (uint32_t) (sizeof(um[uart].uart_gpio)/sizeof(um[uart].uart_gpio[0])) ; i++) {
		/* gpio port )= 0xffffffff means ignore this pin */
		if(um[uart].uart_gpio[i].port != 0xffffffff) {
			ioport_set_port_mode(um[uart].uart_gpio[i].port,
								 um[uart].uart_gpio[i].pin_id,
								 um[uart].uart_gpio[i].mode_mux);
			ioport_disable_port(um[uart].uart_gpio[i].port,
								um[uart].uart_gpio[i].pin_id);
		}
	}

	/* Configure USART TX/RX:
	 *  - 8bits
	 *  - 1Stop
	 *  - No parity
	 *  - baudrate from input parameter
	 */
	USART_InitStructure.baudrate = uart_init->baudrate;
	USART_InitStructure.charlength = US_MR_CHRL_8_BIT;
	USART_InitStructure.stopbits = US_MR_NBSTOP_1_BIT;
	USART_InitStructure.paritytype = US_MR_PAR_NO;

	
	sysclk_enable_peripheral_clock(um[uart].uart_periph_id);
	
	/* initialize and enable UART */
	usart_serial_init(um[uart].uart_ip, &USART_InitStructure);
	
	/*
	 * Enable hw handshake if required.
	 * Note: Flow control is only supported on INV_UART_SENSOR_CTRL
	 */
	if(uart_init->flow_ctrl != INV_UART_FLOW_CONTROL_NONE) {
		if(uart == INV_UART_SENSOR_CTRL)
			um[INV_UART_SENSOR_CTRL].uart_ip->US_MR = (um[INV_UART_SENSOR_CTRL].uart_ip->US_MR & ~US_MR_USART_MODE_Msk) |
										US_MR_USART_MODE_HW_HANDSHAKING;
		else
			return INV_UART_ERROR_BAD_ARG;
	}
	
	/* 
	 * UART TX specific configuration:
	 *  - Enable UART interrupt at NVIC level
	 */
	NVIC_EnableIRQ((IRQn_Type)um[uart].uart_it_nb);

	/* 
	 * UART RX specific configuration:
	 *  - Set circular buffer mode for RX
	 *  - No interrupt
	 */
	um[uart].uart_pdc->PERIPH_PTCR = PERIPH_PTCR_RXCBEN;
	
	/* Trigger a DMA RX. 
	 * Note: DMA is in circular mode. refer to the device datasheet for further details.
	 */
	return uart_dma_rx(uart);
}

int inv_uart_putc(inv_uart_num_t uart, int ch)
{
	uint8_t lch;
	inv_uart_tx_transfer_t txfer;
	
	lch = ch;
	txfer.data = (uint8_t *)&lch;
	txfer.len = 1;
	
	return inv_uart_tx_txfer(uart, &txfer);
}

int inv_uart_puts(inv_uart_num_t uart, const char * s, unsigned short l)
{
	inv_uart_tx_transfer_t txfer;
	
	txfer.data = (uint8_t *)s;
	txfer.len = (uint16_t)l;
	
	return inv_uart_tx_txfer(uart, &txfer);
}

int inv_uart_tx_txfer(inv_uart_num_t uart, inv_uart_tx_transfer_t * txfer)
{
	int rc = INV_UART_ERROR_SUCCES;
	inv_uart_state_t uart_tx_state;
	int timeout = 1000;
	
	uart_tx_state = inv_uart_tx_get_state(uart);
	
	if(uart_tx_state == INV_UART_STATE_IDLE) {
		if (txfer->len > um[uart].uart_tx_buffer_size) {
			/* Requested transfer size does fit in the internal buffers */
			rc = INV_UART_ERROR_MEMORY;
		} else {
			pdc_packet_t pdc_usart_packet;
			uint32_t pdc_status;

			inv_disable_irq();
			
			/* Double-check if there is already an on-going transfer in TX */
			pdc_status = pdc_read_status(um[uart].uart_pdc);
			if( (pdc_status & PERIPH_PTCR_TXTEN) != 0) {
				inv_enable_irq();
				return INV_UART_ERROR_BUSY;
			}

			/* Copy the data to be transfered into the internal buffers */
			memcpy((void*)um[uart].uart_tx_buffer, txfer->data, txfer->len);
			
			/* Set address and size of data to be transfered  and configure PDC */
			pdc_usart_packet.ul_addr = (uint32_t)um[uart].uart_tx_buffer;
			pdc_usart_packet.ul_size = txfer->len;
			
			pdc_tx_init(um[uart].uart_pdc, &pdc_usart_packet, NULL);

			/* Enable DMA transfer in TX */
			pdc_enable_transfer(um[uart].uart_pdc, PERIPH_PTCR_TXTEN);

			/* Enable UART TX buffer empty interrupt. Corresponding NVIC interrupt 
			 * was already enabled by uart_init(). */
			usart_enable_interrupt(um[uart].uart_ip, US_IER_TXBUFE);

			um[uart].uart_tx_state = INV_UART_STATE_BUSY_TX;
			
			/* Wait for the stream to be enabled */
			while (((pdc_read_status(um[uart].uart_pdc) & PERIPH_PTCR_TXTEN) == 0) && (timeout-- > 0));
			if (timeout == 0) {
				/* For some reason, the transfer did not start */
				um[uart].uart_tx_state = INV_UART_STATE_IDLE;
				rc = INV_UART_ERROR;
			}
			
			inv_enable_irq();
		}
	} else if(uart_tx_state == INV_UART_STATE_BUSY_TX) {
		/* A transfer is already on-going */
		rc = INV_UART_ERROR_BUSY;
	}

	return rc;
}

int inv_uart_getc(inv_uart_num_t uart)
{
	int data = EOF;

	/* Check if there is something in the RX FIFO */
	if(inv_uart_available(uart) > 0) {
		inv_disable_irq();
		/* Pop the data from the RX FIFO and increment the tail */
		data = um[uart].uart_rx_buffer[um[uart].uart_rx_buffer_tail++];
		/* Tail rollover */
		um[uart].uart_rx_buffer_tail %= um[uart].uart_rx_buffer_size;
		inv_enable_irq();
	}

	return data;
}

int inv_uart_available(inv_uart_num_t uart)
{
	uint16_t head = um[uart].uart_rx_buffer_size - pdc_read_rx_counter(um[uart].uart_pdc);
	uint16_t tail = um[uart].uart_rx_buffer_tail;

	if(head >= tail)
		return (int)(head - tail);
	else
		return (int)(um[uart].uart_rx_buffer_size - (tail - head));
}

inv_uart_flow_control_t inv_uart_get_flow_control_configuration(inv_uart_num_t uart)
{	
	if( (um[uart].uart_ip->US_MR & US_MR_USART_MODE_Msk) == US_MR_USART_MODE_HW_HANDSHAKING)
		return INV_UART_FLOW_CONTROL_RTS_CTS;
	else
		return INV_UART_FLOW_CONTROL_NONE;
}

inv_uart_state_t inv_uart_tx_get_state(inv_uart_num_t uart)
{	
	return um[uart].uart_tx_state;
}

inv_uart_state_t inv_uart_rx_get_state(inv_uart_num_t uart)
{
	return um[uart].uart_rx_state;
}



/***************************** Private Functions ******************************/

static int uart_dma_rx(inv_uart_num_t uart)
{
	int rc = 0;
	int timeout = 1000;
	
	inv_disable_irq();
	if(um[uart].uart_rx_state == INV_UART_STATE_IDLE) {

		pdc_packet_t pdc_usart_packet;

		/* Read PDC current status */
		uint32_t pdc_status = pdc_read_status(um[uart].uart_pdc);
		
		/* Double-check if there is already an on-going transfer in RX */
		if( (pdc_status & PERIPH_PTCR_RXTEN) != 0) {
			inv_enable_irq();
			return INV_UART_ERROR_BUSY;
		}

		/* Initialize PDC (DMA) transfer address and size. Set same values for next transfer 
		 * as RX is used in circular mode (cf atmel SAM55G datasheet §21.5.3).
		 */
		pdc_usart_packet.ul_addr = (uint32_t)um[uart].uart_rx_buffer;
		pdc_usart_packet.ul_size = um[uart].uart_rx_buffer_size;

		/* Configure PDC for data receive */
		pdc_rx_init(um[uart].uart_pdc, &pdc_usart_packet, &pdc_usart_packet);

		um[uart].uart_rx_state = INV_UART_STATE_BUSY_RX;
		
		/* Enable DMA transfer in RX */
		pdc_enable_transfer(um[uart].uart_pdc, PERIPH_PTCR_RXTEN);

		/* Wait for the stream to be actually enabled  */
		timeout = 1000;
		while (((pdc_read_status(um[uart].uart_pdc) & PERIPH_PTCR_RXTEN) == 0) && (timeout-- > 0));
		if (timeout == 0) {
			um[uart].uart_rx_state = INV_UART_STATE_IDLE;
			rc = INV_UART_ERROR;
		}
	} else {
		rc = INV_UART_ERROR_BUSY;
	}
	
	inv_enable_irq();

	return rc;
}



/* Interrupt management ------------------------------------------------------*/

static void commonUSART_IRQ_HANDLER(inv_uart_num_t uart)
{
	uint32_t uart_status;
	
	uart_status = usart_get_status(um[uart].uart_ip);


	if((uart_status & US_CSR_TXBUFE) == US_CSR_TXBUFE) {
		pdc_disable_transfer(um[uart].uart_pdc, PERIPH_PTCR_TXTDIS);

		/* Mask TXBUFE interrupt. This will move IRQ state from active to inactive
		 * at NVIC level when exiting this handler.  */
		usart_disable_interrupt(um[uart].uart_ip, US_IER_TXBUFE);

		um[uart].uart_tx_state = INV_UART_STATE_IDLE;

		if(um[uart].tx_done_cb)
			um[uart].tx_done_cb(um[uart].tx_context);
	}

	/* TODO : Handle transmission error here */	
}


void USART0_IRQ_HANDLER(void)
{
	commonUSART_IRQ_HANDLER(INV_UART_SENSOR_CTRL);
}

void USART7_IRQ_HANDLER(void)
{
	commonUSART_IRQ_HANDLER(INV_UART_LOG);	
}
