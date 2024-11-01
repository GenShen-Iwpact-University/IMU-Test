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

/** @defgroup I2C_Master I2C_Master
	@ingroup Low_Level_Driver
	@{
*/
#ifndef __INV_I2C_MASTER_H__
#define __INV_I2C_MASTER_H__

#include <stdint.h>

/** @brief Configures I2C master peripheral
*/
void inv_i2c_master_init(void);

/** @brief Deactivates the I2C master peripheral
*/
void inv_i2c_master_deinit(void);

/** @brief Read a register through the control interface I2C
* @param[in] address, I2c 7bit-address
* @param[in] register_addr, register address (location) to access
* @param[in] register_len, length value to read
* @param[in] register_value, pointer on byte value to read
* @retval 0 if correct communication, else wrong communication
*/
unsigned long inv_i2c_master_read_register(unsigned char address, unsigned char register_addr,
                                          unsigned short register_len, unsigned char *register_value);

/** @brief Write a register through the control interface I2C
* @param[in] address, I2c 7bit-address
* @param[in] register_addr, register address (location) to access
* @param[in] register_len, length value to write
* @param[in] register_value, pointer on byte value to write
* @retval 0 if correct communication, else wrong communication
*/
unsigned long inv_i2c_master_write_register(unsigned char address, unsigned char register_addr,
                                           unsigned short register_len, const unsigned char *register_value);

/** @brief Read a byte buffer through the control interface I2C
* @param[in] address, I2c 7bit-address
* @param[in] len, length value to read
* @param[in] value, pointer on byte value to read
* @retval 0 if correct communication, else wrong communication
*/
unsigned long inv_i2c_master_read_raw(unsigned char address, unsigned short len, unsigned char *value);

/** @brief Write a byte buffer through the control interface I2C
* @param[in] address, I2c 7bit-address
* @param[in] len, length value to write
* @param[in] value, pointer on byte value to write
* @retval 0 if correct communication, else wrong communication
*/
unsigned long inv_i2c_master_write_raw(unsigned char address, unsigned short len, const unsigned char *value);

#endif /* __INV_I2C_MASTER_H__ */

/** @} */
