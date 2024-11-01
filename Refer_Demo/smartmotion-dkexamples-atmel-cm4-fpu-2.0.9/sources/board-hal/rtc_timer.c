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

#include "rtc_timer.h"

#include <stddef.h>

static uint64_t rtc_timer_get_timestamp_us(void);
static void get_time_and_milliseconds_synced(Rtc *p_rtc, 
	uint32_t *pul_hour, uint32_t *pul_minute, uint32_t *pul_second, uint32_t *pul_millisecond);

/*
 * Callback for RTC IRQ handler
 */
static void (*sRtc_irq_handler_cb)(void);


/* Public functions  -------------------------------------------------*/

void rtc_timer_init(void (*rtc_irq_handler)(void))
{
	sRtc_irq_handler_cb = rtc_irq_handler;

	pmc_switch_sclk_to_32kxtal(PMC_OSC_XTAL);
	while (!pmc_osc_is_ready_32kxtal());

	/* Default RTC configuration, 24-hour mode */	
	rtc_set_hour_mode(RTC, 0);
	rtc_set_time(RTC,0,0,0);
}


void rtc_timer_update_irq_callback(void (*rtc_irq_handler)(void))
{
	sRtc_irq_handler_cb = rtc_irq_handler;
}

uint64_t rtc_timer_get_time_us(void)
{
	static uint64_t timestamp_monotonic_us = 0;

	uint64_t ts = rtc_timer_get_timestamp_us();

	while (ts < timestamp_monotonic_us) {
		ts += 24ULL * 3600ULL * 1000000ULL;
	}

	timestamp_monotonic_us = ts;

	return timestamp_monotonic_us;
}

/* Interrupt management ------------------------------------------------------*/

/**
 * \brief Interrupt handler for the RTC.
 */
void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);

	/* Time event interrupt */
	if ((ul_status & RTC_SR_TIMEV) == RTC_SR_TIMEV) {
		/* Disable RTC interrupt */
		rtc_disable_interrupt(RTC, RTC_IDR_TIMDIS);

		rtc_clear_status(RTC, RTC_SCCR_TIMCLR);

		rtc_enable_interrupt(RTC, RTC_IER_TIMEN);
	
		if (sRtc_irq_handler_cb != NULL)
			sRtc_irq_handler_cb();
	}
}

/* Private function  -----------------------------------------------*/

/**
  * @brief  Get timestamps from RTC calendar counters in microsecond
  * @retval timestamps in microsecond on 24 hours
  */
static uint64_t rtc_timer_get_timestamp_us(void)
{
	uint32_t counter_subsec;
	uint32_t rtc_hour;
	uint32_t rtc_minute;
	uint32_t rtc_second;
	uint64_t subsecond_us;
	uint64_t base_us;

	//counter_subsec = rtc_get_milliseconds(RTC);
	get_time_and_milliseconds_synced(RTC, &rtc_hour, &rtc_minute, &rtc_second, &counter_subsec);

	base_us = (uint64_t)(counter_subsec * 1000000ULL) / 1024ULL;

	//rtc_get_time(RTC, &rtc_hour, &rtc_minute, &rtc_second);

	subsecond_us = (uint64_t)rtc_hour * 3600ULL * 1000000ULL
				  + (uint64_t)rtc_minute * 60ULL * 1000000ULL
				  + (uint64_t)rtc_second * 1000000ULL
				  + base_us;
	
	return subsecond_us;
}


/* Defined in rtc.c, necessary for the workaround*/
#define BCD_SHIFT      4    /* The BCD code shift value */
#define BCD_MASK       0xfu /* The BCD code mask value */
#define BCD_FACTOR     10   /* The BCD mul/div factor value */
/**
  * @brief  Get timestamps from RTC calendar
  * Required to read RTC_TIMR and RTC_MSR simultaneously to prevent out of sync issue
  * A while loop is required to ensure the data is stable
  */
static void get_time_and_milliseconds_synced(Rtc *p_rtc, 
	uint32_t *pul_hour, uint32_t *pul_minute, uint32_t *pul_second, uint32_t *pul_millisecond)
{
	uint32_t ul_time;
	uint32_t ul_time_milli;
	uint32_t ul_temp;

	/* Get the current RTC time (multiple reads are necessary to insure a stable value). */
	ul_time = p_rtc->RTC_TIMR;
	ul_time_milli = p_rtc->RTC_MSR;
	while (ul_time != p_rtc->RTC_TIMR || ul_time_milli != p_rtc->RTC_MSR) {
		ul_time = p_rtc->RTC_TIMR;
		ul_time_milli = p_rtc->RTC_MSR;
	}

	/* Hour */
	if (pul_hour) {
		ul_temp = (ul_time & RTC_TIMR_HOUR_Msk) >> RTC_TIMR_HOUR_Pos;
		*pul_hour = (ul_temp >> BCD_SHIFT) * BCD_FACTOR + (ul_temp & BCD_MASK);

		if ((ul_time & RTC_TIMR_AMPM) == RTC_TIMR_AMPM) {
			*pul_hour += 12;
		}
	}

	/* Minute */
	if (pul_minute) {
		ul_temp = (ul_time & RTC_TIMR_MIN_Msk) >> RTC_TIMR_MIN_Pos;
		*pul_minute = (ul_temp >> BCD_SHIFT) * BCD_FACTOR +  (ul_temp & BCD_MASK);
	}

	/* Second */
	if (pul_second) {
		ul_temp = (ul_time & RTC_TIMR_SEC_Msk) >> RTC_TIMR_SEC_Pos;
		*pul_second = (ul_temp >> BCD_SHIFT) * BCD_FACTOR + (ul_temp & BCD_MASK);
	}

	/* Millisecond */
	if (pul_millisecond) {
		*pul_millisecond = ul_time_milli & RTC_MSR_MS_Msk;
	}
}
