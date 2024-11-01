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

#include "ioport.h"

#include "dbg_gpio.h"

static const uint16_t mapping[INV_DBG_GPIO_MAX] = { PIO_PA0_IDX, PIO_PA17_IDX, PIO_PA18_IDX };

void inv_dbg_gpio_init_out(int gpio_num)
{
	if(gpio_num < INV_DBG_GPIO_MAX) {
		ioport_init();
		ioport_set_pin_dir(mapping[gpio_num], IOPORT_DIR_OUTPUT);
		ioport_set_pin_mode(mapping[gpio_num], IOPORT_MODE_PULLDOWN);
	}
}

void inv_dbg_gpio_set(int gpio_num)
{
	if(gpio_num < INV_DBG_GPIO_MAX) {
		ioport_set_pin_level(mapping[gpio_num], true);
	}
}

void inv_dbg_gpio_clear(int gpio_num)
{
	if(gpio_num < INV_DBG_GPIO_MAX) {
		ioport_set_pin_level(mapping[gpio_num], false);
	}
}

void inv_dbg_gpio_toggle(int gpio_num)
{
	if(gpio_num < INV_DBG_GPIO_MAX) {
		ioport_toggle_pin_level(mapping[gpio_num]);
	}
}