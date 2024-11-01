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

#ifndef _FLASH_MANAGER_H_
#define _FLASH_MANAGER_H_

#include <stdint.h>

/**
  * @brief  Initialize the FLASH
  * @return 0 if init succeeds, flash_status otherwise
  */
int inv_flash_manager_init(void);

/**
  * @brief  Write data to the FLASH sector
  * @param  pData, pointer on 84 bytes buffer of data
  * @return 0 if write succeeds, flash_status otherwise
  */
int inv_flash_manager_writeData(const uint8_t* pData);

/**
  * @brief  Erase a FLASH sector
  * @return 0 if read succeeds, flash_status otherwise
  */
int inv_flash_manager_eraseData(void);

/**
  * @brief  Read data from the FLASH sector
  * @param  pData, pointer on 84 bytes buffer of data
  * @return 0 if read succeeds, flash_status otherwise
  */
int inv_flash_manager_readData(uint8_t* pData);

#endif /* _FLASH_MANAGER_H_ */
