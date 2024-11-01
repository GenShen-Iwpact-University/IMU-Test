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
#include "uart_mngr.h"

#include <stdio.h>
#include <string.h>

/********************************* Defines ************************************/
/*
 * For each UART, we define 2 buffers (one for RX, the other for TX).
 *
 * Each buffer may be split between uart manager (the present module abstracting 
  * UART) and UART driver (uart.c controlling IP and handling effective data 
 * transfers). First part of the buffer is reserved for UART driver. It is 
 * passed at init time (see inv_uart_init() for the details). It is then no more 
 * used by uart manager.
 * The rest part of the buffer is used by uart manager as a circular buffer.
 * 
 * Buffers sizes are defined with the 2 macros (XX being RX or TX):
 * - <UART_ID>_XX_BUFFER_SIZE defines total size of buffer in bytes
 * - <UART_ID>_XX_MAX_TXFER_SIZE defines size in bytes reserved for uart driver.
 *   It corresponds to the maximum size uart driver can transfer in a single 
 *   transfer.
 * 
 * In current implementation, TX buffer only uses a circular buffer. For RX
 * buffers, the whole size is reserved for driver:
 *
 * <uart_id>_tx_buffer[0]
 * |
 * |             <uart_id>_tx_buffer[<UART_ID>_TX_MAX_TXFER_SIZE]
 * |                        |
 * |                        |              <uart_id>_tx_buffer[<UART_ID>_TX_BUFFER_SIZE]
 * |                        |                                                  |
 * *---------------------------------------------------------------------------*
 * |  uart driver reserved  |           uart manager circular buffer           |
 * *---------------------------------------------------------------------------*
 *
 *
 * <uart_id>_rx_buffer[0]
 * |                                   <uart_id>_rx_buffer[<UART_ID>_RX_BUFFER_SIZE]
 * |                                   (= <uart_id>_rx_buffer[<UART_ID>_TX_MAX_TXFER_SIZE])
 * |                                                                           |
 * *---------------------------------------------------------------------------*
 * |                           uart driver reserved                            |
 * *---------------------------------------------------------------------------*
 *
 */
/** INV_UART_SENSOR_CTRL **/
#ifndef INV_UART_SENSOR_CTRL_TX_BUFFER_SIZE
  #define INV_UART_SENSOR_CTRL_TX_BUFFER_SIZE      128
#endif
#ifndef INV_UART_SENSOR_CTRL_RX_BUFFER_SIZE
  #define INV_UART_SENSOR_CTRL_RX_BUFFER_SIZE       64
#endif

/** < Defines the maximum number of characters that the uart driver will be able to transmit in a single transfer */
#define INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE      64
#if INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE > INV_UART_SENSOR_CTRL_TX_BUFFER_SIZE
  #error INV_UART_SENSOR_CTRL_TX_BUFFER_SIZE can not be smaller then INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE
#endif

/** UART2 **/
#ifndef INV_UART_LOG_TX_BUFFER_SIZE
  #define INV_UART_LOG_TX_BUFFER_SIZE      128
#endif
#ifndef INV_UART_LOG_RX_BUFFER_SIZE
  #define INV_UART_LOG_RX_BUFFER_SIZE       64
#endif

/** < Defines the maximum number of characters that the uart driver will be able to transmit in a single transfer */
#define INV_UART_LOG_TX_MAX_TXFER_SIZE      64
#if INV_UART_LOG_TX_MAX_TXFER_SIZE > INV_UART_LOG_TX_BUFFER_SIZE
  #error INV_UART_LOG_TX_BUFFER_SIZE can not be smaller then INV_UART_LOG_TX_MAX_TXFER_SIZE
#endif


#define UART_MNGR_TX_ABSOLUTE_MAX_TRANSFER_SIZE  ((INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE > INV_UART_LOG_TX_MAX_TXFER_SIZE) ? INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE : INV_UART_LOG_TX_MAX_TXFER_SIZE)

/********************************* Globals ************************************/

/** @brief UART manager buffer object definition.
 */
typedef struct {
	uint8_t *    buffer;                       /** < Pointer to the start of the memory area allocated to the module.
                                                     Note: A part of this buffer will be yielded to the UART driver 
                                                           while the rest will be used as a ring byte buffer by the 
                                                           current module */
	uint16_t     driver_max_single_txfer_size; /** < Size of the buffer yielded to the UART driver */
	uint16_t     circ_buffer_size;             /** < Size of the UART manager ring byte buffer
                                                     Note: total buffer size = driver_max_single_txfer_size +
                                                                                     circ_buffer_size */
	volatile uint16_t circ_buffer_head;        /** < Pointer to the head of the UART manager ring byte buffer */
	volatile uint16_t circ_buffer_tail;        /** < Pointer to the tail of the UART manager ring byte buffer */
} uart_mngr_buffer_struct_t;

/** @brief UART manager object definition.
 *
 * Contains all the fields needed by the current module in order to manage an UART peripheral
 */
typedef struct {
	inv_uart_num_t   uart_mngr_uart_num;                  /** < Managed UART peripheral */
	uart_mngr_buffer_struct_t uart_rx_buf;            /** < Managed UART RX buffer */
	uart_mngr_buffer_struct_t uart_tx_buf;            /** < Managed UART TX buffer */
}uart_mngr_struct_t;

/* Uart manager internal buffers */
static uint8_t sensor_ctrl_uart_tx_buffer[INV_UART_SENSOR_CTRL_TX_BUFFER_SIZE];
static uint8_t sensor_ctrl_uart_rx_buffer[INV_UART_SENSOR_CTRL_RX_BUFFER_SIZE];
static uint8_t log_uart_tx_buffer[INV_UART_LOG_TX_BUFFER_SIZE];
static uint8_t log_uart_rx_buffer[INV_UART_LOG_RX_BUFFER_SIZE];

/* Uart manager objects */
static uart_mngr_struct_t um[2] = {
	
	{
		.uart_mngr_uart_num               = INV_UART_SENSOR_CTRL,
		.uart_rx_buf = {
			.buffer                       = &sensor_ctrl_uart_rx_buffer[0],
			.driver_max_single_txfer_size = INV_UART_SENSOR_CTRL_RX_BUFFER_SIZE,
			.circ_buffer_size             = 0, /* whole RX buffer is reserved for uart driver */
			.circ_buffer_head             = 0,
			.circ_buffer_tail             = 0
		},
		.uart_tx_buf = {
			.buffer                       = &sensor_ctrl_uart_tx_buffer[0],
			.driver_max_single_txfer_size = INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE,
			.circ_buffer_size             = INV_UART_SENSOR_CTRL_TX_BUFFER_SIZE - INV_UART_SENSOR_CTRL_TX_MAX_TXFER_SIZE,
			.circ_buffer_head             = 0,
			.circ_buffer_tail             = 0,
		},
    },

	{
		.uart_mngr_uart_num                 = INV_UART_LOG,
		.uart_rx_buf = {
			.buffer                       = &log_uart_rx_buffer[0],
			.driver_max_single_txfer_size = INV_UART_LOG_RX_BUFFER_SIZE,
			.circ_buffer_size             = 0, /* whole RX buffer is reserved for uart driver */
			.circ_buffer_head             = 0,
			.circ_buffer_tail             = 0,
		},
		.uart_tx_buf = {
			.buffer                       = &log_uart_tx_buffer[0],
			.driver_max_single_txfer_size = INV_UART_LOG_TX_MAX_TXFER_SIZE,
			.circ_buffer_size             = INV_UART_LOG_TX_BUFFER_SIZE - INV_UART_LOG_TX_MAX_TXFER_SIZE,
			.circ_buffer_head             = 0,
			.circ_buffer_tail             = 0,
		},
	}
};


/********************************* Prototypes *********************************/

/** @brief      Pops data form the ring byte buffer
 *  @param[in]  uart_mngr Pointer to the UART manager buffer object
 *  @param[out] data Pointer to the memory area where the popped data will be stored
 *  @param[in]  len Number of bytes to be popped
 *  @note:      This function does not check if there are any bytes available (waiting to be popped)
 *              in the ring byte buffer prior to popping them
 */
static void buffer_pop(uart_mngr_buffer_struct_t * uart_mngr, uint8_t * data, uint16_t len)
{
  /* start index of the circular buffer (first bytes are reserved for driver) */
		uint16_t start = uart_mngr->driver_max_single_txfer_size;
	uint16_t tail = uart_mngr->circ_buffer_tail;

	uart_mngr->circ_buffer_tail += len;
	uart_mngr->circ_buffer_tail %= uart_mngr->circ_buffer_size;

	if((uart_mngr->circ_buffer_size - tail) >= len) {
		memcpy(data, &uart_mngr->buffer[start+tail], len);
	} else {
		memcpy(data, &uart_mngr->buffer[start+tail],
				uart_mngr->circ_buffer_size - tail);
		memcpy(&data[uart_mngr->circ_buffer_size - tail],
				&uart_mngr->buffer[start],
				len - uart_mngr->circ_buffer_size + tail);
	}
}

/** @brief      Pushes data in the ring byte buffer
 *  @param[in]  uart_mngr Pointer to the UART manager buffer object
 *  @param[in]  data Pointer to the first element to the pushed
 *  @param[in]  len Number of bytes to be pushed
 *  @note:      This function does not check if there is any space available 
 *              in the ring byte buffer prior to pushing the data
 */
static void buffer_push(uart_mngr_buffer_struct_t * uart_mngr, uint8_t * data, uint16_t len)
{
  /* start index of the circular buffer (first bytes are reserved for driver) */
		uint16_t start = uart_mngr->driver_max_single_txfer_size;

	uint16_t head = uart_mngr->circ_buffer_head;

	if((uart_mngr->circ_buffer_size - head) >= len) {
		memcpy(&uart_mngr->buffer[start+head], data, len);
	} else {
		
		memcpy(&uart_mngr->buffer[start+head], data, 
				uart_mngr->circ_buffer_size - head);
		memcpy(&uart_mngr->buffer[start],
				&data[uart_mngr->circ_buffer_size - head],
				len - uart_mngr->circ_buffer_size + head);
	}

	uart_mngr->circ_buffer_head += len;
	uart_mngr->circ_buffer_head %= uart_mngr->circ_buffer_size;
}

/** @brief      Returns the bytes available in the ring byte buffer
 *  @param[in]  uart_mngr Pointer to the UART manager buffer object
 *  @return     The number of bytes available (waiting to be popped) in the ring byte buffer
 */
static uint16_t buffer_get_size(uart_mngr_buffer_struct_t * uart_mngr)
{
	uint16_t head = uart_mngr->circ_buffer_head;
	uint16_t tail = uart_mngr->circ_buffer_tail;

	if(head >= tail)
		return (head - tail);
	else
		return (uart_mngr->circ_buffer_size - tail + head);
}

/** @brief      Returns the free space that is available in the ring byte buffer
 *  @param[in]  uart_mngr Pointer to the UART manager buffer object
 *  @return     The space, in bytes, available in the ring byte buffer
 */
static uint16_t buffer_get_available(uart_mngr_buffer_struct_t * uart_mngr)
{
	return (uart_mngr->circ_buffer_size - buffer_get_size(uart_mngr));
}

/** @brief      Callback called when a TX transfer finishes
 *  @param[in]  context Context passed at init
 */
static void uart_tx_done_cb(void * context)
{
	inv_uart_tx_transfer_t uart_txfer_struct;
	static uint8_t temp_buf[UART_MNGR_TX_ABSOLUTE_MAX_TRANSFER_SIZE];
	uart_mngr_struct_t * uart_mngr = (uart_mngr_struct_t *)context;
	
	/* Check if there is data waiting to be transfered in the ring byte buffer*/
	uint16_t bytes_num = buffer_get_size(&uart_mngr->uart_tx_buf);
	
	if(bytes_num) {
		if(bytes_num >= uart_mngr->uart_tx_buf.driver_max_single_txfer_size) {
			/* Clamp the number of bytes to be transfered to the maximum size supported by the UART driver */
			bytes_num = uart_mngr->uart_tx_buf.driver_max_single_txfer_size;
		}
		buffer_pop(&uart_mngr->uart_tx_buf, temp_buf, bytes_num);
		uart_txfer_struct.data = temp_buf;
		uart_txfer_struct.len  = bytes_num;
		inv_uart_tx_txfer(uart_mngr->uart_mngr_uart_num, &uart_txfer_struct);
	}
}


int inv_uart_mngr_init(inv_uart_mngr_init_struct_t * uart_mngr_init_struct)
{
	inv_uart_init_struct_t uart_init_struct;
	inv_uart_num_t uart_nb = uart_mngr_init_struct->uart_num;

	/* Populate the driver init structure with data common for all the UART peripherals */
	uart_init_struct.uart_num = uart_mngr_init_struct->uart_num;
	uart_init_struct.baudrate = uart_mngr_init_struct->baudrate;
	uart_init_struct.flow_ctrl = uart_mngr_init_struct->flow_ctrl;
	uart_init_struct.tx_done_cb = uart_tx_done_cb;

	/* Populate the driver init structure with data specific to each UART peripheral */
	uart_init_struct.tx_buffer  = um[uart_nb].uart_tx_buf.buffer;
	uart_init_struct.rx_buffer  = um[uart_nb].uart_rx_buf.buffer;
	uart_init_struct.tx_size    = um[uart_nb].uart_tx_buf.driver_max_single_txfer_size;
	uart_init_struct.rx_size    = um[uart_nb].uart_rx_buf.driver_max_single_txfer_size;
	uart_init_struct.tx_context = &um[uart_nb];
	
	/* Initialize the UART driver */
	return inv_uart_init(&uart_init_struct);
}

int inv_uart_mngr_puts(inv_uart_num_t uart, const char * s, unsigned short l)
{
	int rc = INV_UART_ERROR_SUCCES;
	inv_uart_state_t uart_tx_state;
	uart_mngr_struct_t * uart_mngr = &um[uart];
	
	/* Check if the data to be transfered fits in the internal buffer
	 * Total size of the buffer being driver reserved size + circular buffer size
	 */
	if(l > (uart_mngr->uart_tx_buf.driver_max_single_txfer_size + 
		     uart_mngr->uart_tx_buf.circ_buffer_size) )
		return INV_UART_ERROR_MEMORY;
	
	inv_disable_irq();
	uart_tx_state = inv_uart_tx_get_state(uart_mngr->uart_mngr_uart_num);
	
	if(uart_tx_state == INV_UART_STATE_IDLE) {
		if(l <= uart_mngr->uart_tx_buf.driver_max_single_txfer_size) {
			/* If the data fits into the UART driver's buffers, make a single transfer */
			rc = inv_uart_puts(uart_mngr->uart_mngr_uart_num, s, l);
		} else {
			/* If the data does not fit in the UART driver's internal buffers, 
			 * make an UART transfer and push the rest of the data in the UART manager's ring byte buffer 
			 */
			buffer_push(&uart_mngr->uart_tx_buf, (uint8_t *)&s[uart_mngr->uart_tx_buf.driver_max_single_txfer_size], 
					l - uart_mngr->uart_tx_buf.driver_max_single_txfer_size);
			rc = inv_uart_puts(uart_mngr->uart_mngr_uart_num, s, uart_mngr->uart_tx_buf.driver_max_single_txfer_size);
		}
	} else if(uart_tx_state == INV_UART_STATE_BUSY_TX) {
		if(buffer_get_available(&uart_mngr->uart_tx_buf) < l) {
			/* Not enough room in the ring byte buffer */
			rc = INV_UART_ERROR_MEMORY;
		} else {
			/* Push the data in the ring byte buffer so that it can be transfered once the current transfer finishes */
			buffer_push(&uart_mngr->uart_tx_buf, (uint8_t *)s, l);
		}
	} else if(uart_tx_state == INV_UART_STATE_RESET) {
		/* UART driver non-initialized */
		rc = INV_UART_ERROR;
	}
	inv_enable_irq();
	
	return rc;
}

int inv_uart_mngr_getc(inv_uart_num_t uart)
{
	return inv_uart_getc(uart);
}

int inv_uart_mngr_available(inv_uart_num_t uart)
{
	return inv_uart_available(uart);
}