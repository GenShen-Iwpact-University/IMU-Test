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

#include "tc.h"
#include "sysclk.h"

#include "timer.h"

#define DIV_ROUND_UINT(n,d) ((((n) + (d)/2)/(d)))

struct timer_mapping {
	Tc * timer_periph;
	uint32_t timer_channel;
	uint32_t timer_id;
	uint32_t timer_mode;
	uint32_t ra;
	uint32_t rc;
	void (*callback) (void *context);
	void *context;
	IRQn_Type irq;
	void (*irq_handler) (void);
};

static uint32_t timer_prescaler;
static volatile uint32_t timer1_hsb_counter;

static struct timer_mapping tm[6] = {
	{ // TIMER1. Chained to the timer mapped to TIMER_MAX
	.timer_periph = TC0, 
	.timer_channel = 1, 
	.timer_id = ID_TC1, 
	.timer_mode = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVE | TC_CMR_ACPA_CLEAR | 
	              TC_CMR_ACPC_SET /* trigger event to chained timer when .rc is reached == when rollover happens */ | TC_CMR_ASWTRG_SET,
	.ra = UINT16_MAX / 2,
	.rc = 0, 
	.callback = 0, 
	.context = 0, 
	.irq = TC1_IRQn,
	.irq_handler = TC1_Handler
	},
	{ // TIMER2
	.timer_periph = TC0, 
	.timer_channel = 2, 
	.timer_id = ID_TC2, 
	.timer_mode = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVE,
	.ra = 0,
	.rc = 0, 
	.callback = 0, 
	.context = 0, 
	.irq = TC2_IRQn,
	.irq_handler = TC2_Handler
	},
	{ // TIMER3
	.timer_periph = TC1, 
	.timer_channel = 0, 
	.timer_id = ID_TC3, 
	.timer_mode = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVE,
	.ra = 0,
	.rc = 0, 
	.callback = 0, 
	.context = 0, 
	.irq = TC3_IRQn,
	.irq_handler = TC3_Handler
	},
	{ // TIMER4
	.timer_periph = TC1, 
	.timer_channel = 1, 
	.timer_id = ID_TC4, 
	.timer_mode = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVE,
	.ra = 0,
	.rc = 0, 
	.callback = 0, 
	.context = 0, 
	.irq = TC4_IRQn,
	.irq_handler = TC4_Handler
	},
	{ // TIMER5
	.timer_periph = TC1, 
	.timer_channel = 2, 
	.timer_id = ID_TC5, 
	.timer_mode = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_WAVE,
	.ra = 0,
	.rc = 0, 
	.callback = 0, 
	.context = 0, 
	.irq = TC5_IRQn,
	.irq_handler = TC5_Handler
	},
	{ // TIMER_MAX used to count TIMER1's rollovers. Should always be mapped to TIMER_MAX
	.timer_periph = TC0, 
	.timer_channel = 0, 
	.timer_id = ID_TC0, 
	.timer_mode = TC_CMR_TCCLKS_XC0 | TC_CMR_WAVE,
	.ra = 0,
	.rc = 0, 
	.callback = 0, 
	.context = 0, 
	.irq = TC0_IRQn,
	.irq_handler = TC0_Handler
	}
};

/* @brief Fusions three 16 bit and 32 bit timers into one 64 bit timer.
 * @param lsb_1: first timer value obtained during the first read
 * @param msb_1: second timer value obtained during the first read
 * @param hsb_1: third timer value obtained during the first read
 * @param lsb_2: first timer value obtained during the second read
 * @param msb_2: second timer value obtained during the second read
 * @param hsb_2: third timer value obtained during the second read
 * @return Concatenated LSB and MSB and HSB
 * @details
 * This function supposes that the three timers that are fed as input are chained, meaning that the
 * second timer counts the overflows of the first timer, and the third timer counts the overflow of the second
 * timer. It also supposes that the three reads of the three timers are done as close as possible in time.
 * It needs three timer reads in order to detect if there was an overflow between the read of the first timer
 * and the read of the second timer and third timer.
 * @note:
 * The two timers have to be read lsb then msb then hsb. Reading them in another order might prevent this
 * function from detecting an overflow that took place between three timer reads.
 */
static uint64_t fusion_16bit_into_64bit(uint16_t lsb_1, uint16_t msb_1, uint32_t hsb_1,
		uint16_t lsb_2, uint16_t msb_2, uint32_t hsb_2)
{
	uint64_t ret_val;
	
	/* Keep consistency between lsb_1, msb_1 and hsb_1 to be concatenated */
	if (lsb_2 < lsb_1) {
		/* Detect if the second counter rolled-over between the moments when the 
		* msb_1 and hsb_1 were read
		*/
		if (msb_2 < msb_1)
			if (hsb_2 == hsb_1)
				hsb_1--;
		/* Detect if the first counter rolled-over between the moments when the 
		* lsb_1 and msb_1 were read
		*/
		if (msb_2 == msb_1)
			msb_1--;
	}
	
	ret_val  = (uint64_t)lsb_1;
	ret_val |= (uint64_t)msb_1 << 16;
	ret_val |= (uint64_t)hsb_1 << 32;
	
	return ret_val;
}

void inv_timer_enable(unsigned timer_num)
{
	if(timer_num >= INV_TIMER_MAX)
		return;
	
	/* If timer is not already running, start it */
	if((tc_get_status(tm[timer_num].timer_periph, tm[timer_num].timer_channel) & TC_SR_CLKSTA) == 0) {
		tc_start(tm[timer_num].timer_periph, tm[timer_num].timer_channel);
		while((tc_get_status(tm[timer_num].timer_periph, tm[timer_num].timer_channel) & TC_SR_CLKSTA) == 0);
	}
}

int inv_timer_configure_timebase(uint32_t frequency)
{
	/* 
	 * Compute the prescaler value for the requested frequency,
	 * knowing that TC0 and TC1 are fed by the same clock.
	 */
	timer_prescaler = DIV_ROUND_UINT(sysclk_get_peripheral_bus_hz(TC0), frequency) - 1;
	if(timer_prescaler > UINT8_MAX)
		return -1;
	
	/* Configure the PMC to enable the TC modules. */
	for(uint32_t i=0; i< (uint32_t)(sizeof(tm)/sizeof(tm[0])); i++) {
		sysclk_enable_peripheral_clock(tm[i].timer_id);
	}
	/* Enable PCK output and configure prescaler */
	pmc_disable_pck(PMC_PCK_3);
	if(pmc_switch_pck_to_mck(PMC_PCK_3, PMC_PCK_PRES(timer_prescaler)))
		return -1;
	pmc_enable_pck(PMC_PCK_3);
	
	/* Configure all channels as per tm structure request */
	for(uint32_t i=0; i< (uint32_t)(sizeof(tm)/sizeof(tm[0])); i++) {
		tc_init(tm[i].timer_periph, tm[i].timer_channel, tm[i].timer_mode);
		
		tc_write_ra(tm[i].timer_periph, tm[i].timer_channel, tm[i].ra);
		tc_write_rc(tm[i].timer_periph, tm[i].timer_channel, tm[i].rc);
		
		/* Clear status register and pending interrupts */
		tc_get_status(tm[i].timer_periph, tm[i].timer_channel);
		NVIC_DisableIRQ(tm[i].irq);
		NVIC_ClearPendingIRQ(tm[i].irq);
		NVIC_SetPriority(tm[i].irq, 0); // TODO: make the priority configurable
		NVIC_EnableIRQ(tm[i].irq);
	}
		
	timer1_hsb_counter = 0;

	/* Chain TIOA channel 1 to XC0 of channel 0 for INV_TIMER_MAX to count INV_TIMER1 overflow */
	tc_set_block_mode(tm[INV_TIMER1].timer_periph, TC_BMR_TC0XC0S_TIOA1);
	/* Enable interrupt to increment timer1_hsb_counter when INV_TIMER_MAX overflows, i.e. when RC is compared */
	tc_enable_interrupt(tm[INV_TIMER_MAX].timer_periph, tm[INV_TIMER_MAX].timer_channel, TC_SR_CPCS);
	/* Can be started since it will not count until INV_TIMER1 (TC0 channel 1) is started */
	tc_start(tm[INV_TIMER_MAX].timer_periph, tm[INV_TIMER_MAX].timer_channel);

	return 0;
}

uint64_t inv_timer_get_counter(unsigned timer_num)
{
	uint64_t ret_value;
	uint16_t lsb_1, msb_1, lsb_2, msb_2;
	uint32_t hsb_1, hsb_2;
	
	/* Sanity check */
	if(timer_num >= INV_TIMER_MAX)
		return 0;
	
	if(timer_num == INV_TIMER1) {
		lsb_1 = tc_read_cv(tm[INV_TIMER1].timer_periph, tm[INV_TIMER1].timer_channel);
		msb_1 = tc_read_cv(tm[INV_TIMER_MAX].timer_periph, tm[INV_TIMER_MAX].timer_channel);
		hsb_1 = timer1_hsb_counter;
		lsb_2 = tc_read_cv(tm[INV_TIMER1].timer_periph, tm[INV_TIMER1].timer_channel);
		msb_2 = tc_read_cv(tm[INV_TIMER_MAX].timer_periph, tm[INV_TIMER_MAX].timer_channel);
		hsb_2 = timer1_hsb_counter;
		ret_value = fusion_16bit_into_64bit(lsb_1, msb_1, hsb_1, lsb_2, msb_2, hsb_2);
	} else {
		/* convert 16 bits to 64 bits */
		ret_value = (uint64_t)tc_read_cv(tm[timer_num].timer_periph, tm[timer_num].timer_channel);
	}
	
	return ret_value;
}

static int timer_verify_freq(uint32_t freq)
{
	uint32_t timer_counting_freq, timer_period, min_freq, max_freq;

	/*
		Prescaler is configured by <timer_configure_timebase> as: (MCK / frequency) - 1.
		Timer frequency can be obtained using the following:
			frequency = MCK / (timer_prescaler + 1)
		For example, a timer with 1Mhz resolution on 16bit can do a minimum of:
		1,000,000 / 2^16 ~= 15.26Hz
	*/

	timer_period = UINT16_MAX;

	timer_counting_freq = sysclk_get_peripheral_bus_hz(TC0) / (timer_prescaler + 1);

	min_freq = timer_counting_freq / timer_period;
	if(min_freq == 0)
		/* above line will return 0 if timer_period>timer_counting_freq,
		setting 1 here also protects from <freq> being 0 */
		min_freq = 1;
	max_freq = timer_counting_freq; /*maximum one interrupt every tick*/

	if(freq >= min_freq && freq <= max_freq) {
		return 0;
	}

	return -1;
}

int inv_timer_configure_callback(unsigned timer_num, uint32_t freq,
	void * context, void (*callback) (void *context))
{
	uint32_t sr, rc;
	
	
	/* Sanity checks */
	if(timer_verify_freq(freq) != 0)
		return -1;
	if((timer_num >= INV_TIMER_MAX) || (timer_num == INV_TIMER1))
		return -1;
	
	tm[timer_num].rc = sysclk_get_peripheral_bus_hz(TC0) / (timer_prescaler + 1) / freq /*Hz*/;
	tm[timer_num].callback = callback;
	tm[timer_num].context  = context;
	
	/* Clear status register and pending interrupts */
	sr = tc_get_status(tm[timer_num].timer_periph, tm[timer_num].timer_channel);
	NVIC_ClearPendingIRQ(tm[timer_num].irq);
	
	if((sr & TC_SR_CLKSTA) == 0) {
		/* If the timer is stopped, the call to 'tc_start' will reset the counter value*/
		rc  = 0;
	} else {
		rc  = tc_read_cv(tm[timer_num].timer_periph, tm[timer_num].timer_channel);
	}
	rc += tm[timer_num].rc;
	tc_write_rc(tm[timer_num].timer_periph, tm[timer_num].timer_channel, rc);
	
	tc_enable_interrupt(tm[timer_num].timer_periph, tm[timer_num].timer_channel, TC_SR_CPCS);
	
	/* If timer is not already running, start it */
	if((sr & TC_SR_CLKSTA) == 0)
		tc_start(tm[timer_num].timer_periph, tm[timer_num].timer_channel);

	return 0;
}

int inv_timer_channel_suspend(unsigned timer_num, uint8_t channel)
{
	(void)channel;

	if(timer_num >= INV_TIMER_MAX)
		return -1;

	tc_disable_interrupt(tm[timer_num].timer_periph, tm[timer_num].timer_channel, TC_SR_CPCS);

	return 0;
}

int inv_timer_channel_stop(unsigned timer_num, uint8_t channel)
{
	(void) channel;
	
	if(timer_num >= INV_TIMER_MAX)
		return -1;

	tc_disable_interrupt(tm[timer_num].timer_periph, tm[timer_num].timer_channel, TC_SR_CPCS);
	
	tc_stop(tm[timer_num].timer_periph, tm[timer_num].timer_channel);
	while(((tc_get_status(tm[timer_num].timer_periph, tm[timer_num].timer_channel) & TC_SR_CLKSTA) == 1));
	
	tm[timer_num].callback = 0;
	tm[timer_num].context  = 0;

	return 0;
}

int inv_timer_channel_reconfigure_freq(unsigned timer_num, uint8_t channel, uint32_t new_freq)
{
	(void)channel;

	/* Sanity checks */
	if((timer_num >= INV_TIMER_MAX) || (timer_num == INV_TIMER1))
		return -1;
	if (timer_verify_freq(new_freq) != 0)
		return -1;

	tc_disable_interrupt(tm[timer_num].timer_periph, tm[timer_num].timer_channel, TC_SR_CPCS);
	
	tm[timer_num].rc = sysclk_get_peripheral_bus_hz(TC0) / (timer_prescaler + 1) / new_freq /*Hz*/;
	
	tc_write_rc(tm[timer_num].timer_periph, tm[timer_num].timer_channel, 
			tc_read_cv(tm[timer_num].timer_periph, tm[timer_num].timer_channel) + tm[timer_num].rc);
	
	tc_enable_interrupt(tm[timer_num].timer_periph, tm[timer_num].timer_channel, TC_SR_CPCS);

	return 0;
}

static void timer_irq_handler(void (*caller)(void))
{
	for(int i=INV_TIMER1; i<INV_TIMER_MAX; i++) {
		if(caller == tm[i].irq_handler) {
			if((tc_get_status(tm[i].timer_periph, tm[i].timer_channel) & TC_SR_CPCS) == TC_SR_CPCS) {
				tc_write_rc(tm[i].timer_periph, tm[i].timer_channel, 
						tc_read_rc(tm[i].timer_periph, tm[i].timer_channel) + tm[i].rc);
				
				if(tm[i].callback)
					tm[i].callback(tm[i].context);
			}
		}
	}
}

void TC0_Handler(void)
{
	uint32_t status = tc_get_status(tm[INV_TIMER_MAX].timer_periph, tm[INV_TIMER_MAX].timer_channel);
	if (status & TC_SR_CPCS)
		timer1_hsb_counter++;
}

void TC1_Handler(void)
{
	timer_irq_handler(TC1_Handler);
}

void TC2_Handler(void)
{
	timer_irq_handler(TC2_Handler);
}

void TC3_Handler(void)
{
	timer_irq_handler(TC3_Handler);
}

void TC4_Handler(void)
{
	timer_irq_handler(TC4_Handler);
}

void TC5_Handler(void)
{
	timer_irq_handler(TC5_Handler);
}