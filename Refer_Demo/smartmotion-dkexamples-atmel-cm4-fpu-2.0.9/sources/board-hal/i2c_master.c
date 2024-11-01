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

#include <string.h>

#include "twi.h"
#include "flexcom.h"
#include "sysclk.h"
#include "ioport.h"
#include "pio.h"

#include "i2c_master.h"

/********************************* Globals ************************************/
struct i2c_master_mapping {
	uint8_t is_inited;
	Twi * p_i2c;
	Flexcom * p_flexcom;
	ioport_pin_t clk_pin;
	ioport_mode_t clk_pin_mode;
	ioport_pin_t sda_pin;
	ioport_mode_t sda_pin_mode;
	uint32_t clk_speed;
};

struct i2c_master_mapping i2cm = {
	.is_inited = 0,
	.p_i2c = TWI4, 
	.p_flexcom = FLEXCOM4, 
	.clk_pin = PIO_PB9_IDX, 
	.clk_pin_mode = IOPORT_MODE_MUX_A, 
	.sda_pin = PIO_PB8_IDX, 
	.sda_pin_mode = IOPORT_MODE_MUX_A, 
	.clk_speed = 400000
};

/********************************* Prototypes *********************************/

void inv_i2c_master_init(void)
{
	twi_options_t opt;
	if (i2cm.is_inited != 0)
		return;

	memset(&opt, 0, sizeof(twi_options_t));
	
	ioport_set_pin_mode(i2cm.sda_pin, i2cm.sda_pin_mode);
	ioport_disable_pin(i2cm.sda_pin);
	ioport_set_pin_mode(i2cm.clk_pin, i2cm.clk_pin_mode);
	ioport_disable_pin(i2cm.clk_pin);

	/* Enable the peripheral and set TWI mode. */
	flexcom_enable(i2cm.p_flexcom);
	flexcom_set_opmode(i2cm.p_flexcom, FLEXCOM_TWI);

	/* Configure the options of TWI driver */
	opt.master_clk = sysclk_get_peripheral_hz();
	opt.speed      = i2cm.clk_speed;

	if (twi_master_init(i2cm.p_i2c, &opt) != TWI_SUCCESS)
		return;
	
	i2cm.is_inited = 1;
}

unsigned long inv_i2c_master_write_register(unsigned char Address, unsigned char RegisterAddr, unsigned short RegisterLen, const unsigned char *RegisterValue)
{
	uint8_t data = 0;
	twi_packet_t packet_tx;
	
	/* Configure the data packet to be transmitted */
	packet_tx.chip        = Address;
	packet_tx.addr[0]     = RegisterAddr;
	packet_tx.addr_length = 1;

	/* I2C Semi-Write is basically not supported
	 * Force packet length to 1 and 0 in packet buffer in case of I2C Semi-Write needed */
	if (RegisterLen == 0) {
		packet_tx.buffer  = &data;
		packet_tx.length  = 1;
	} else {
		packet_tx.buffer  = (uint8_t *) RegisterValue;
		packet_tx.length  = RegisterLen;
	}

	/* Send data to attached I2C slave */
	if (twi_master_write(i2cm.p_i2c, &packet_tx) != TWI_SUCCESS)
		return -1;
	else
		return 0;
}

unsigned long inv_i2c_master_write_raw(unsigned char Address, unsigned short RegisterLen, const unsigned char *RegisterValue)
{
	twi_packet_t packet_tx;
	
	/* Configure the data packet to be transmitted */
	packet_tx.chip        = Address;
	packet_tx.addr[0]     = 0;
	packet_tx.addr_length = 0;
	packet_tx.buffer      = (uint8_t *) RegisterValue;
	packet_tx.length      = RegisterLen;

	/* Send data to attached I2C slave */
	if (twi_master_write(i2cm.p_i2c, &packet_tx) != TWI_SUCCESS)
		return -1;
	else
		return 0;    
}

unsigned long inv_i2c_master_read_register(unsigned char Address, unsigned char RegisterAddr, unsigned short RegisterLen, unsigned char *RegisterValue)
{
	twi_packet_t packet_rx;
	
	/* Configure the data packet to be received */
	packet_rx.chip        = Address;
	packet_rx.addr[0]     = RegisterAddr;
	packet_rx.addr_length = 1;
	packet_rx.buffer      = RegisterValue;
	packet_rx.length      = RegisterLen;

	/* Get data out of attached I2C slave */
	if (twi_master_read(i2cm.p_i2c, &packet_rx) != TWI_SUCCESS)
		return -1;
	else
		return 0;
}

unsigned long inv_i2c_master_read_raw(unsigned char Address, unsigned short RegisterLen, unsigned char *RegisterValue)
{
 	twi_packet_t packet_rx;
	
	/* Configure the data packet to be received */
	packet_rx.chip        = Address;
	packet_rx.addr[0]     = 0;
	packet_rx.addr_length = 0;
	packet_rx.buffer      = RegisterValue;
	packet_rx.length      = RegisterLen;

	/* Get data out of attached I2C slave */
	if (twi_master_read(i2cm.p_i2c, &packet_rx) != TWI_SUCCESS)
		return -1;
	else
		return 0;   
}

void inv_i2c_master_deinit(void)
{
	/* Disable the I2C peripheral */
	ioport_reset_pin_mode(i2cm.sda_pin);
	ioport_reset_pin_mode(i2cm.clk_pin);
	
	flexcom_disable(i2cm.p_flexcom);
	twi_disable_master_mode(i2cm.p_i2c);
	twi_reset(i2cm.p_i2c);
	
	i2cm.is_inited = 0;
}
