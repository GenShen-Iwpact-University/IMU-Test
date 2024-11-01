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

#include "pio.h"
#include "ioport.h"
#include "pmc.h"
#include "pio_handler.h"

#include "gpio.h"

struct gpio_mapping {
	Pio * p_pio;
	uint32_t pio_id;
	uint32_t pio_index;
	uint32_t pio_mask;
	pio_type_t pio_type;
	uint32_t pio_attributes;
	IRQn_Type pio_irqn;
	void (*callback) (void *context, unsigned pin_num);
	void *context;
};

// Set which gpio configuration to apply according to board revision
#ifdef INV_SENSOR_DB
static unsigned invn_gpio_board_rev = INV_GPIO_BOARD_REVB_DB;
#else
static unsigned invn_gpio_board_rev = INV_GPIO_BOARD_REVB;
#endif

static struct gpio_mapping gm[INV_GPIO_BOARD_MAX][INV_GPIO_MAX] = {
	/* 
	 * INV_GPIO_BOARD_REVG = INV_GPIO_BOARD_REVI
	 */
	{
		{ // GPIO_INV_SENSOR_INT1
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA30_IDX,
			.pio_mask = PIO_PA30,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_INT2
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA17_IDX, 
			.pio_mask = PIO_PA17,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_FSYNC
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA18,
			.pio_index = PIO_PA18_IDX,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_3RD_PARTY_SENSOR_INT1
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA20,
			.pio_index = PIO_PA20_IDX,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_SW0_BUTTON
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA2,
			.pio_index = PIO_PA2_IDX, 
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_FALL_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // INV_GPIO_CLKIN
		  // /!\ Duplicates GPIO_INV_SENSOR_INT2
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA17_IDX, 
			.pio_mask = PIO_PA17,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // INV_GPIO_AD0
			.p_pio = PIOA,
			.pio_id = ID_PIOA,
			.pio_mask = PIO_PA12,
			.pio_index = PIO_PA12_IDX,
			.pio_type = PIO_OUTPUT_0,
			.pio_attributes = (PIO_DEFAULT),
			.pio_irqn = PIOA_IRQn,
			.callback = 0,
			.context = 	0
		},
		{ // INV_GPIO_CSN
			0
		}
	},
	/* 
	 * INV_GPIO_BOARD_REVB
	 */
	{
		{ // GPIO_INV_SENSOR_INT1
			.p_pio = PIOB, 
			.pio_id = ID_PIOB,
			.pio_index = PIO_PB3_IDX, 
			.pio_mask = PIO_PB3,
			.pio_irqn = PIOB_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_INT2
			.p_pio = PIOB, 
			.pio_id = ID_PIOB,
			.pio_index = PIO_PB15_IDX,
			.pio_mask = PIO_PB15,
			.pio_irqn = PIOB_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_FSYNC
			0
		},
		{ // GPIO_3RD_PARTY_SENSOR_INT1
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA15,
			.pio_index = PIO_PA15_IDX,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_SW0_BUTTON
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA2,
			.pio_index = PIO_PA2_IDX, 
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_FALL_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // INV_GPIO_CLKIN
			0
		},
		{ // INV_GPIO_AD0
			0
		},
		{ // INV_GPIO_CSN
			0
		}
	},
	/* 
	 * INV_GPIO_BOARD_REVB_DB
	 */
	{
		{ // GPIO_INV_SENSOR_INT1
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA30_IDX, 
			.pio_mask = PIO_PA30,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_INT2
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA20_IDX,
			.pio_mask = PIO_PA20,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_FSYNC
			.p_pio = PIOA,
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA29_IDX,
			.pio_mask = PIO_PA29,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT,
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE),
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_3RD_PARTY_SENSOR_INT1
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA15,
			.pio_index = PIO_PA15_IDX,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_SW0_BUTTON
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA2,
			.pio_index = PIO_PA2_IDX, 
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_FALL_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // INV_GPIO_CLKIN
			.p_pio = PIOA,
			.pio_id = ID_PIOA,
			.pio_mask = PIO_PA17,
			.pio_index = PIO_PA17_IDX,
			.pio_type = PIO_OUTPUT_0,
			.pio_attributes = (PIO_DEFAULT),
			.pio_irqn = PIOA_IRQn,
			.callback = 0,
			.context = 0
		},
		{ // INV_GPIO_AD0
			0
		},
		{ // INV_GPIO_CSN
			0
		}
	},
	/* 
	 * INV_GPIO_BOARD_REVDPLUS
	 */
	{
		{ // GPIO_INV_SENSOR_INT1
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA30_IDX,
			.pio_mask = PIO_PA30,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_INT2
			.p_pio = PIOA, 
			.pio_id = ID_PIOA,
			.pio_index = PIO_PA20_IDX, 
			.pio_mask = PIO_PA20,
			.pio_irqn = PIOA_IRQn,
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_OPENDRAIN | PIO_IT_RISE_EDGE), 
			.callback = 0, 
			.context = 0
		},
		{ // GPIO_INV_SENSOR_FSYNC
			0
		},
		{ // GPIO_3RD_PARTY_SENSOR_INT1
			0
		},
		{ // GPIO_SW0_BUTTON
			.p_pio = PIOA, 
			.pio_id = ID_PIOA, 
			.pio_mask = PIO_PA2,
			.pio_index = PIO_PA2_IDX, 
			.pio_type = PIO_INPUT, 
			.pio_attributes = (PIO_DEFAULT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_FALL_EDGE), 
			.pio_irqn = PIOA_IRQn, 
			.callback = 0, 
			.context = 0
		},
		{ // INV_GPIO_CLKIN
			0
		},
		{ // INV_GPIO_AD0
			.p_pio = PIOA,
			.pio_id = ID_PIOA,
			.pio_mask = PIO_PA12,
			.pio_index = PIO_PA12_IDX,
			.pio_type = PIO_OUTPUT_0,
			.pio_attributes = (PIO_DEFAULT),
			.pio_irqn = PIOA_IRQn,
			.callback = 0,
			.context = 	0
		},
		{ // INV_GPIO_CSN
			.p_pio = PIOA,
			.pio_id = ID_PIOA,
			.pio_mask = PIO_PA11,
			.pio_index = PIO_PA11_IDX,
			.pio_type = PIO_OUTPUT_0,
			.pio_attributes = (PIO_DEFAULT),
			.pio_irqn = PIOA_IRQn,
			.callback = 0,
			.context = 	0
		}
	}
};

static void gpio_common_callback(const uint32_t id, const uint32_t index);

void inv_gpio_set_board_rev(gpio_num_t board_rev)
{
	invn_gpio_board_rev = board_rev;
}

void inv_gpio_init_pin_out(unsigned pin_num)
{
	pmc_enable_periph_clk(gm[invn_gpio_board_rev][pin_num].pio_id);
	pio_set_output(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_mask, LOW, DISABLE, DISABLE);
}

void inv_gpio_init_pin_input(unsigned pin_num)
{
	pmc_enable_periph_clk(gm[invn_gpio_board_rev][pin_num].pio_id);
	pio_set_input(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_mask, PIO_DEFAULT);
}

void inv_gpio_toggle_pin(unsigned pin_num)
{
	pio_toggle_pin_group(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_mask);
}

void inv_gpio_set_pin_high(unsigned pin_num)
{
	pio_set_pin_group_high(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_mask);
}

void inv_gpio_set_pin_low(unsigned pin_num)
{
	pio_set_pin_group_low(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_mask);
}

int inv_gpio_get_status(unsigned pin_num)
{
	if(pin_num >= INV_GPIO_MAX)
		return -1;

	return (int)pio_get(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_type, gm[invn_gpio_board_rev][pin_num].pio_mask);
}

int inv_gpio_output_clk_on_pin(unsigned pin_num)
{
	// Only works for PA17
	if (gm[invn_gpio_board_rev][pin_num].pio_index == PIO_PA17_IDX) {
		
		pmc_switch_sclk_to_32kxtal(0);
		
		ioport_set_pin_mode(gm[invn_gpio_board_rev][pin_num].pio_index, IOPORT_MODE_MUX_B);
		ioport_disable_pin(gm[invn_gpio_board_rev][pin_num].pio_index);
		
		pmc_switch_pck_to_sclk(1, 0);
		pmc_enable_pck(1);
		
	} else {
		return -1;
	}
	
	return 0;
}

int inv_gpio_disable_clk_on_pin(unsigned pin_num)
{
	// Only works for PA17
	if (gm[invn_gpio_board_rev][pin_num].pio_index == PIO_PA17_IDX) {

		pmc_disable_pck(1);
		inv_gpio_init_pin_input(pin_num);

	} else {
		return -1;
	}

	return 0;
}

void inv_gpio_sensor_irq_init(unsigned pin_num,
		void (*interrupt_cb)(void * context, unsigned pin_num), void * context)
{
	if(pin_num >= INV_GPIO_MAX)
		return;
	
	gm[invn_gpio_board_rev][pin_num].callback = interrupt_cb;
	gm[invn_gpio_board_rev][pin_num].context = context;
	
	pmc_enable_periph_clk(gm[invn_gpio_board_rev][pin_num].pio_id);
	
	pio_configure(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_type, gm[invn_gpio_board_rev][pin_num].pio_mask, 
			 gm[invn_gpio_board_rev][pin_num].pio_attributes);
	
	pio_handler_set(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_id, gm[invn_gpio_board_rev][pin_num].pio_mask, 
			gm[invn_gpio_board_rev][pin_num].pio_attributes, gpio_common_callback);
	
	pio_enable_interrupt(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_mask);
	
	pio_handler_set_priority(gm[invn_gpio_board_rev][pin_num].p_pio, gm[invn_gpio_board_rev][pin_num].pio_irqn, 0);// TODO: make the priority configurable
}

static void gpio_common_callback(const uint32_t id, const uint32_t index)
{
	for(unsigned i=0; i<sizeof(gm)/sizeof(gm[invn_gpio_board_rev][0]); i++) {
		if((gm[invn_gpio_board_rev][i].pio_id == id) && (gm[invn_gpio_board_rev][i].pio_mask == index)) {
			if(gm[invn_gpio_board_rev][i].callback)
				gm[invn_gpio_board_rev][i].callback(gm[invn_gpio_board_rev][i].context, i);
			break;
		}
	}
}