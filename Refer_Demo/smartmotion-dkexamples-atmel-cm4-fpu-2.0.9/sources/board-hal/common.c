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

#include "sysclk.h"
#include "wdt.h"
#include "ioport.h"

#include "common.h"

/* This variable contains the number of nested calls to disable_irq */
static uint32_t sDisableIntCount = 0;

void inv_board_hal_init(void)
{
	/* Initialize the SAM system */
	sysclk_init();
	
	ioport_init();
	
	/* Initialize LED0, turned off */
	ioport_set_pin_dir(PIO_PA6_IDX, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(PIO_PA6_IDX, IOPORT_PIN_LEVEL_HIGH);

	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;
}

void inv_disable_irq(void)
{
	if(sDisableIntCount == 0) {
		__disable_irq();
	}
	sDisableIntCount ++;
}

void inv_enable_irq(void)
{
	sDisableIntCount --;
	if(sDisableIntCount == 0) {
		__enable_irq();
	}
}
