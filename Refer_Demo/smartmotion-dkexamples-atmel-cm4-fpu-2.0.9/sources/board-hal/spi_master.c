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

#include "spi.h"
#include "pdc.h"
#include "flexcom.h"
#include "sysclk.h"
#include "ioport.h"
#include "common.h"

#include <string.h>

#include "spi_master.h"

/********************************* Defines ************************************/
#ifndef SPI_BUFFER_SIZE
	#define SPI_BUFFER_SIZE 8220
#endif

/********************************* Globals ************************************/
static void spi_master_irq_handler(unsigned spi_num);
static void spi_master_spi_abort_async(unsigned spi_num);

/* keep track of spi used according to board revision, set during init */
static spi_num_t invn_spi_num = INV_SPI_REVG;

struct spi_mapping {
	Spi                   * p_spi;
	Pdc                   * p_pdc;
	Flexcom               * p_flexcom;
	uint32_t              chip_sel;
	uint32_t              clk_pol;
	uint32_t              clk_ph;
	uint8_t               dlybs; // delay before SPCK. See SPI_CSR register in the device datasheet for more info
	uint8_t               dlybct; // delay between consecutive transfers. See SPI_CSR register in the device datasheet for more info
	ioport_pin_t          clk_pin;
	ioport_mode_t         clk_pin_mode;
	ioport_pin_t          mosi_pin;
	ioport_mode_t         mosi_pin_mode;
	ioport_pin_t          miso_pin;
	ioport_mode_t         miso_pin_mode;
	ioport_pin_t          cs_pin;
	ioport_mode_t         cs_pin_mode;
	uint8_t               tx_buffer[SPI_BUFFER_SIZE];
	uint8_t               rx_buffer[SPI_BUFFER_SIZE];
	enum IRQn             irqn;
	spi_master_async_cb_t transfer_done_cb;
	uint8_t               * rx_dest_addr;
	uint16_t              rx_len;
};

struct spi_mapping sm[INV_SPI_MAX] = {
	/*
	 * INV_SPI_REVG = INV_SPI_REVI = INV_SPI_REVDPLUS
	 */
	{
		.p_spi     = SPI5, 
		.p_pdc     = 0,
		.p_flexcom = FLEXCOM5, 
		.chip_sel  = 0,
		.clk_pol   = 1, 
		.clk_ph    = 0, 
		.dlybs     = 0x40, 
		.dlybct    = 0x01, 
		// pin mapping
		.clk_pin       = PIO_PA14_IDX, 
		.clk_pin_mode  = IOPORT_MODE_MUX_A, 
		.mosi_pin      = PIO_PA13_IDX, 
		.mosi_pin_mode = IOPORT_MODE_MUX_A, 
		.miso_pin      = PIO_PA12_IDX,
		.miso_pin_mode = IOPORT_MODE_MUX_A,
		.cs_pin        = PIO_PA11_IDX,
		.cs_pin_mode   = IOPORT_MODE_MUX_A,
		.tx_buffer     = {0}, 
		.rx_buffer     = {0}, 
		// Asynchronous driver related fields
		.irqn             = FLEXCOM5_IRQn,
		.transfer_done_cb = 0,
		.rx_dest_addr     = 0,
		.rx_len           = 0
	},
	/*
	 * INV_SPI_ONBOARD_REVB
	 */
	{
		.p_spi     = SPI5, 
		.p_pdc     = 0, 
		.p_flexcom = FLEXCOM5,
		.chip_sel  = 1,
		.clk_pol   = 1, 
		.clk_ph    = 0, 
		.dlybs     = 0x40, 
		.dlybct    = 0x01, 
		// pin mapping
		.clk_pin       = PIO_PA14_IDX, 
		.clk_pin_mode  = IOPORT_MODE_MUX_A, 
		.mosi_pin      = PIO_PA13_IDX, 
		.mosi_pin_mode = IOPORT_MODE_MUX_A, 
		.miso_pin      = PIO_PA12_IDX,
		.miso_pin_mode = IOPORT_MODE_MUX_A,
		.cs_pin        = PIO_PA5_IDX,
		.cs_pin_mode   = IOPORT_MODE_MUX_B,
		.tx_buffer     = {0}, 
		.rx_buffer     = {0}, 
		// Asynchronous driver related fields
		.irqn             = FLEXCOM5_IRQn,
		.transfer_done_cb = 0,
		.rx_dest_addr     = 0,
		.rx_len           = 0
	}
};

/********************************* Private Prototypes **************************/

/**
 * \brief Interrupt handler for the SPI master.
 */
static void spi_master_irq_handler(unsigned spi_num)
{
	/* Disable the RX and TX PDC transfer requests */
	pdc_disable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTDIS |
			PERIPH_PTCR_TXTDIS);
			
	NVIC_ClearPendingIRQ(sm[spi_num].irqn);

	if(sm[spi_num].p_spi->SPI_SR & SPI_SR_RXBUFF) {
		memcpy(sm[spi_num].rx_dest_addr, &sm[spi_num].rx_buffer[1], sm[spi_num].rx_len);

		/* Disable SPI IRQ */
		spi_disable_interrupt(sm[spi_num].p_spi, SPI_IDR_RXBUFF);

		if(sm[spi_num].transfer_done_cb)
			sm[spi_num].transfer_done_cb(0);
	}
}


/********************************* Public Prototypes **************************/

void inv_spi_master_init(unsigned spi_num, uint32_t speed_hz)
{

	invn_spi_num = (spi_num_t)spi_num;

	ioport_set_pin_mode(sm[spi_num].clk_pin,  sm[spi_num].clk_pin_mode);
	ioport_set_pin_mode(sm[spi_num].mosi_pin, sm[spi_num].mosi_pin_mode);
	ioport_set_pin_mode(sm[spi_num].miso_pin, sm[spi_num].miso_pin_mode);
	ioport_set_pin_mode(sm[spi_num].cs_pin,   sm[spi_num].cs_pin_mode);
	ioport_disable_pin(sm[spi_num].clk_pin);
	ioport_disable_pin(sm[spi_num].mosi_pin);
	ioport_disable_pin(sm[spi_num].miso_pin);
	ioport_disable_pin(sm[spi_num].cs_pin);
	
	/* Get pointer to SPI master PDC register base */
	sm[spi_num].p_pdc = spi_get_pdc_base(sm[spi_num].p_spi);

	/* Enable the peripheral and set SPI mode. */
	flexcom_enable(sm[spi_num].p_flexcom);
	flexcom_set_opmode(sm[spi_num].p_flexcom, FLEXCOM_SPI);

	spi_disable(sm[spi_num].p_spi);
	spi_reset(sm[spi_num].p_spi);
	spi_set_lastxfer(sm[spi_num].p_spi);
	spi_set_master_mode(sm[spi_num].p_spi);
	spi_disable_mode_fault_detect(sm[spi_num].p_spi);
	
	spi_configure_cs_behavior(sm[spi_num].p_spi, sm[spi_num].chip_sel, SPI_CS_RISE_NO_TX);
	
	spi_set_peripheral_chip_select_value(sm[spi_num].p_spi, sm[spi_num].chip_sel);
	
	spi_set_clock_polarity(sm[spi_num].p_spi, sm[spi_num].chip_sel, sm[spi_num].clk_pol);
	spi_set_clock_phase(sm[spi_num].p_spi, sm[spi_num].chip_sel, sm[spi_num].clk_ph);
	spi_set_bits_per_transfer(sm[spi_num].p_spi, sm[spi_num].chip_sel, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(sm[spi_num].p_spi, sm[spi_num].chip_sel, 
			(sysclk_get_peripheral_hz() / speed_hz));
	spi_set_transfer_delay(sm[spi_num].p_spi, sm[spi_num].chip_sel, sm[spi_num].dlybs, 
			sm[spi_num].dlybct);

	spi_enable(sm[spi_num].p_spi);

	pdc_disable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
	
	sm[spi_num].transfer_done_cb = 0;
}

void inv_spi_master_init_async(unsigned spi_num, uint32_t speed_hz, spi_master_async_cb_t transfer_done_cb)
{
	inv_spi_master_init(spi_num, speed_hz);
	
	sm[spi_num].transfer_done_cb = transfer_done_cb;
	
	if(sm[spi_num].transfer_done_cb) {
		/* Configure SPI interrupts */
		NVIC_DisableIRQ(sm[spi_num].irqn);
		NVIC_ClearPendingIRQ(sm[spi_num].irqn);
		NVIC_EnableIRQ(sm[spi_num].irqn);
	}
}

void inv_spi_master_deinit(unsigned spi_num)
{
	if (sm[spi_num].transfer_done_cb)
		spi_master_spi_abort_async(spi_num);
	ioport_reset_pin_mode(sm[spi_num].clk_pin);
	ioport_reset_pin_mode(sm[spi_num].mosi_pin);
	ioport_reset_pin_mode(sm[spi_num].miso_pin);
	ioport_reset_pin_mode(sm[spi_num].cs_pin);

	flexcom_disable(sm[spi_num].p_flexcom);

	spi_disable(sm[spi_num].p_spi);
	spi_reset(sm[spi_num].p_spi);

	pdc_disable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
	
	sm[spi_num].transfer_done_cb = 0;
	/* Configure SPI interrupts */
	spi_disable_interrupt(sm[spi_num].p_spi, SPI_IDR_RXBUFF) ;
	NVIC_DisableIRQ(sm[spi_num].irqn);
	NVIC_ClearPendingIRQ(sm[spi_num].irqn);
}

unsigned long inv_spi_master_write_register(unsigned spi_num, unsigned char register_addr,
		unsigned short len, const unsigned char *value)
{
	if(len+1 > SPI_BUFFER_SIZE)
		return 1;
	
	pdc_packet_t pdc_spi_packet;
	
	/* Disable Irq during buffer write*/
	inv_disable_irq();
	pdc_spi_packet.ul_addr = (uint32_t)&sm[spi_num].rx_buffer[0];
	pdc_spi_packet.ul_size = len + 1;
	pdc_rx_init(sm[spi_num].p_pdc, &pdc_spi_packet, NULL);

	sm[spi_num].tx_buffer[0] = (uint8_t) register_addr;
	memcpy(&sm[spi_num].tx_buffer[1], (uint8_t *)value, len);

	pdc_spi_packet.ul_addr = (uint32_t)&sm[spi_num].tx_buffer[0];
	pdc_spi_packet.ul_size = len + 1;
	pdc_tx_init(sm[spi_num].p_pdc, &pdc_spi_packet, NULL);
	
	/* Enable the RX and TX PDC transfer requests */
	pdc_enable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
	/* Re activate Irq */
	inv_enable_irq();
	/* Waiting transfer done*/
	while((spi_read_status(sm[spi_num].p_spi) & SPI_SR_TXEMPTY) == 0);
		
	/* Disable the RX and TX PDC transfer requests */
	pdc_disable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTDIS |
			PERIPH_PTCR_TXTDIS);
	
	return 0;
}

unsigned long inv_spi_master_read_register(unsigned spi_num, unsigned char register_addr,
		unsigned short len, unsigned char *value)
{
	if(len+1 > SPI_BUFFER_SIZE)
		return 1;
	
	pdc_packet_t pdc_spi_packet;
	
	if(sm[spi_num].transfer_done_cb)
		pdc_rx_clear_cnt(sm[spi_num].p_pdc);
	/* Desactivate Irq during buffer write*/
	inv_disable_irq();
	pdc_spi_packet.ul_addr = (uint32_t)&sm[spi_num].rx_buffer[0];
	pdc_spi_packet.ul_size = len + 1;
	pdc_rx_init(sm[spi_num].p_pdc, &pdc_spi_packet, NULL);
	
	sm[spi_num].tx_buffer[0] = (uint8_t) register_addr | 0x80;
	memset(&sm[spi_num].tx_buffer[1], 0x00, len);

	pdc_spi_packet.ul_addr = (uint32_t)&sm[spi_num].tx_buffer[0];
	pdc_spi_packet.ul_size = len + 1;
	pdc_tx_init(sm[spi_num].p_pdc, &pdc_spi_packet, NULL);
	/* Re activate Irq */
	inv_enable_irq();

	if (sm[spi_num].transfer_done_cb == 0) {
		/* Enable the RX and TX PDC transfer requests */
		pdc_enable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
		
		/* Waiting transfer done*/
		while((spi_read_status(sm[spi_num].p_spi) & SPI_SR_ENDRX) == 0);
		
		/* Disable the RX and TX PDC transfer requests */
		pdc_disable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTDIS |
				PERIPH_PTCR_TXTDIS);
				
		memcpy(value, &sm[spi_num].rx_buffer[1], len);
	} else {
		sm[spi_num].rx_dest_addr = value;
		sm[spi_num].rx_len = len;

		/* Transfer done handler is in ISR */
		spi_enable_interrupt(sm[spi_num].p_spi, SPI_IER_RXBUFF) ;

		/* Enable the RX and TX PDC transfer requests */
		pdc_enable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
	}

	return 0;
}

/** @brief Abort and reset any on going SPI transfer
* @param[in] spinum, required spi line number
*/
static void spi_master_spi_abort_async(unsigned spi_num)
{
	/* Disable SPI IRQ */
	spi_disable_interrupt(sm[spi_num].p_spi, SPI_IDR_RXBUFF);

	if(pdc_read_rx_counter(sm[spi_num].p_pdc) || pdc_read_rx_next_counter(sm[spi_num].p_pdc)) {
		/* Disable PDC transfers */
		pdc_disable_transfer(sm[spi_num].p_pdc, PERIPH_PTCR_RXTDIS | PERIPH_PTCR_TXTDIS);
			
		/* Clear PDC buffer receive counter */
		pdc_rx_clear_cnt(sm[spi_num].p_pdc);
		
		/* Clear status register in case subsequent transfer is synchronous */
		while(spi_is_rx_full(sm[spi_num].p_spi)) {
			spi_get(sm[spi_num].p_spi);
		};

		/* Disable SPI interrupt */
		NVIC_DisableIRQ(sm[spi_num].irqn);

		/* Waiting for abort to be fully processed, checking for CSn rising edge */
		while(((spi_read_status(sm[spi_num].p_spi) & SPI_SR_TXEMPTY) == 0) && 
			  ((spi_read_status(sm[spi_num].p_spi) & SPI_SR_ENDRX) == 0));
	}
}


void FLEXCOM5_Handler(void)
{
	spi_master_irq_handler(invn_spi_num);
}
