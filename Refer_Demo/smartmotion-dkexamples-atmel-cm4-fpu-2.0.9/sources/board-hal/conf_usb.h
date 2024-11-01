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

#ifndef _CONF_USB_H_
#define _CONF_USB_H_

#include "compiler.h"

/** USB Device Configuration */

//! Device definition (mandatory)
#define  USB_DEVICE_VENDOR_ID             USB_VID_ATMEL
#define  USB_DEVICE_PRODUCT_ID            USB_PID_ATMEL_ASF_HIDMOUSE
#define  USB_DEVICE_MAJOR_VERSION         1
#define  USB_DEVICE_MINOR_VERSION         0
#define  USB_DEVICE_POWER                 100 // Consumption on Vbus line (mA)

#define  USB_DEVICE_ATTR                  (USB_CONFIG_ATTR_SELF_POWERED)

//! USB Device string definitions (Optional)
#define  USB_DEVICE_MANUFACTURE_NAME      "ATMEL ASF"

/** Device speeds support */

/** USB Device Callbacks definitions (Optional) */
#define  UDC_SUSPEND_EVENT()              user_callback_suspend_action()
extern void user_callback_suspend_action(void);
#define  UDC_RESUME_EVENT()               user_callback_resume_action()
extern void user_callback_resume_action(void);

/** USB Interface Configuration */

/** Configuration of HID Mouse interface */
//! Interface callback definition
#define  UDI_HID_MOUSE_ENABLE_EXT()          true
#define  UDI_HID_MOUSE_DISABLE_EXT()

/** HID Mouse interval polling interval value (in ms - default value: 10) */
#define UDI_HID_MOUSE_EP_BINTERVAL          1

/** USB Device Driver Configuration */

//! The includes of classes and other headers must be done at the end of this file to avoid compile error
#include "udi_hid_mouse_conf.h"

#endif // _CONF_USB_H_
