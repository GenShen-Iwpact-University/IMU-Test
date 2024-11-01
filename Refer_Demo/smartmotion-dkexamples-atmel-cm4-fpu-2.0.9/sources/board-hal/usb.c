/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2018-2019 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively "Software") is subject
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

#include "common.h"
#include "usb.h"
#include "udc.h"
#include "conf_usb.h"

/********************************* Globals ************************************/
/** External callback called when the host suspend the USB line */
void user_callback_suspend_action(void);
/** External callback called when the host or device resume the USB line */
void user_callback_resume_action(void);

static inv_usb_init_struct_t usb_mapping = {
	.usb_num = INV_USB_TARGET,
	
	.suspend_action_cb = NULL,
	.resume_action_cb = NULL,
};

/****************************** Public Functions ******************************/

void inv_usb_init(inv_usb_init_struct_t * usb_init)
{
	usb_mapping.suspend_action_cb = usb_init->suspend_action_cb;
	usb_mapping.resume_action_cb  = usb_init->resume_action_cb;

	/* Start USB stack to authorize VBus monitoring */
	udc_start();
}

int inv_usb_hid_mouse_move(int8_t moveX, int8_t moveY)
{
	if (udi_hid_mouse_moveXY(moveX, moveY))
		return 0;
	else
		return -1;
}

int inv_usb_hid_mouse_button_click(bool click_left, bool click_right)
{
	if (udi_hid_mouse_btnleft(click_left) && udi_hid_mouse_btnright(click_right))
		return 0;
	else
		return -1;
}

/****************************** Callback Functions ******************************/

void user_callback_suspend_action(void)
{
	if (usb_mapping.suspend_action_cb != NULL)
		usb_mapping.suspend_action_cb();
}

void user_callback_resume_action(void)
{
	if (usb_mapping.resume_action_cb != NULL)
		usb_mapping.resume_action_cb();
}
