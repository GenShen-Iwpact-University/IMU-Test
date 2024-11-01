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

/** @defgroup RTC_Timer RTC_Timer
	@ingroup Low_Level_Driver
	@{
*/

#ifndef _RTC_TIMER_H_
#define _RTC_TIMER_H_

#include <stdint.h>
#include "rtc.h"
#include "pmc.h"

/**
  * @brief  Init system timer based on RTC clock (24-hour mode)  
  * @Note   Initialize SLCK to external crystal.
  * @param  rtc_irq_handler RTC clock IRQ handler, set to NULL in case no wake up is required from RTC
  */
void rtc_timer_init(void (*rtc_irq_handler)(void));

/**
  * @brief  Reconfigure RTC clock IRQ handler
  * @param  rtc_irq_handler RTC clock IRQ handler
  */
void rtc_timer_update_irq_callback(void (*rtc_irq_handler)(void));

/**
  * @brief  Get timestamps from RTC calendar counters in microsecond and prevent rollover
  * @retval timestamps in microsecond
  */
uint64_t rtc_timer_get_time_us(void);

#endif /* _RTC_TIMER_H_ */

/** @} */