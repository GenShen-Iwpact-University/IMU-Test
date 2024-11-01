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

#include "delay.h"
#include "timer.h"

#include <stdint.h>

static unsigned timer_num;

static void start_timer(void) 
{
	inv_timer_enable(timer_num);
}

static void stop_timer(void) 
{
	inv_timer_channel_stop(timer_num, 0);
}

static void internal_delay(uint16_t us)
{
	const uint32_t start = (uint32_t)inv_timer_get_counter(timer_num);

	uint32_t now, prev = 0;
	do{
		now = (uint32_t)inv_timer_get_counter(timer_num);

		/* handle rollover */
		if(now < prev)
			now = UINT16_MAX + now;
		prev = now;

	}while((now - start) <= us);
}

int inv_delay_init(unsigned tim_num)
{
	timer_num = tim_num;
	
	/* Timer configuration at 1MHz frequency */
	return inv_timer_configure_timebase(1000000);
}

void inv_delay_us(uint32_t us)
{
	start_timer();

	/* in case the delay is up to UINT16_MAX */
	if(us >= UINT16_MAX) {
		/* go to the loop as the internal_delay function only support uint16_t argument type */
		for(uint32_t i = 0; i < (uint32_t)(us / UINT16_MAX); i++) {
			internal_delay(UINT16_MAX);
		}
		internal_delay(us % UINT16_MAX);
	}
	else
		internal_delay(us);

	stop_timer();
}

void inv_delay_ms(uint32_t ms)
{
	inv_delay_us(ms * 1000);
}

