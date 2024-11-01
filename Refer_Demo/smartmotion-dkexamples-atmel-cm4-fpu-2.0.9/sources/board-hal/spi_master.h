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

/** @defgroup SPI_master SPI_master
	@ingroup  Low_Level_Driver
	@{
*/
#ifndef __INV_SPI_MASTER_H__
#define __INV_SPI_MASTER_H__

#include <stdint.h>

typedef void (*spi_master_async_cb_t)(void * context);

/** @brief Enumeration of the SPIs connected to the INV sensor:
           - INV_SPI_REVG represents the SPI between AP and ICM on rev G
           - INV_SPI_REVI represents the SPI between AP and ICM on rev I
           - INV_SPI_ONBOARD_REVB represents the SPI between AP and on-board ICM on rev B
           - INV_SPI_REVDPLUS represents the SPI between AP and ICM on rev D+
*/
typedef enum spi_num {
	INV_SPI_REVG         = 0,
	INV_SPI_REVI         = INV_SPI_REVG,
	INV_SPI_REVDPLUS     = INV_SPI_REVG,
	INV_SPI_ONBOARD_REVB = 1,
	INV_SPI_MAX
}spi_num_t;

/* For retro-compatibility */
#define INV_SPI_DB      INV_SPI_REVG
#define INV_SPI_ONBOARD INV_SPI_ONBOARD_REVB

/** @brief Configures SPI master peripheral to act as a synchronous peripheral
* i.e. each read operation polls for end of transfer
* @param[in] spinum, required spi line number
* @param[in] speed, required spi speed
*/
void inv_spi_master_init(unsigned spi_num, uint32_t speed_hz);

/** @brief Configures SPI master peripheral to act as an asynchronous peripheral
* i.e. each read operation is ended only once associated IRQ is triggered
* @param[in] spinum, required spi line number
* @param[in] speed, required spi speed
* @param[in] transfer_done_cb, callback to be called upon IRQ trigger
*/
void inv_spi_master_init_async(unsigned spi_num, uint32_t speed_hz, 
		spi_master_async_cb_t transfer_done_cb);

/** @brief Deactivates the SPI master peripheral
* @param[in] spinum, required spi line number
*/
void inv_spi_master_deinit(unsigned spi_num);

/** @brief Read a register through the control interface SPI 
* @param[in] spinum, required spi line number
* @param[in] register_addr, register address (location) to access
* @param[in] register_len, length value to read
* @param[in] register_value, pointer on byte value to read
* @retval 0 if correct communication, else wrong communication
*/
unsigned long inv_spi_master_read_register(unsigned spi_num, unsigned char register_addr, 
		unsigned short register_len, unsigned char *register_value);

/** @brief Write a register through the control interface SPI  
* @param[in] spinum, required spi line number
* @param[in] register_addr, register address (location) to access
* @param[in] register_len, length value to write
* @param[in] register_value, pointer on byte value to write
* @retval 0 if correct communication, else wrong communication
*/
unsigned long inv_spi_master_write_register(unsigned spi_num, unsigned char register_addr, 
		unsigned short register_len, const unsigned char *register_value);

#endif /* __INV_SPI_MASTER_H__ */

/** @} */
