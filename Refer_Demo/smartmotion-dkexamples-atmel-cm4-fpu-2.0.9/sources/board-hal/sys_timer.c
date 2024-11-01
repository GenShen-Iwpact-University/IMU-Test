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

#include "sys_timer.h"

/*
 * Callback for Systick handler
 */
static void (*sSystick_irq_handler_cb)(void);

void inv_sys_timer_init(void (*systick_handler)(void), uint32_t wake_period_ms)
{
	sSystick_irq_handler_cb = systick_handler;

	// Configure the Systick
	// To adjust the SysTick time base, use the following formula:
	// Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)

	// - Reload Value is the parameter to be passed for SysTick_Config() function
	// - Reload Value should not exceed 0xFFFFFF
	
	if(wake_period_ms < 1000)
		SysTick_Config(sysclk_get_cpu_hz() / (1000 / wake_period_ms));
	else 
		SysTick_Config(sysclk_get_cpu_hz() * (wake_period_ms / 1000));

}

/* Interrupt management ------------------------------------------------------*/
  
/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	if(sSystick_irq_handler_cb != 0)
		sSystick_irq_handler_cb();
}
