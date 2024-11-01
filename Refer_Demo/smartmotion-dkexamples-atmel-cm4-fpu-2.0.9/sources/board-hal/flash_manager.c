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

#include "flash_manager.h"

#include <string.h>
#include "flash_efc.h"

#define FLASH_PAGE_ADDRESS_START (IFLASH_ADDR + IFLASH_PAGE_SIZE)
#define FLASH_DATA_SIZE          84
#define FLASH_HEADER_SIZE        4
#define FLASH_HEADER             0x0A0B0B0A
#define FLASH_HEADER_MISSING_RC  1

uint32_t ul_test_page_addr = FLASH_PAGE_ADDRESS_START;

/** Public functions **/

int inv_flash_manager_init(void)
{
	uint32_t rc;

	/* Lock the flash in default case */
	if (flash_is_locked(ul_test_page_addr, ul_test_page_addr+IFLASH_PAGE_SIZE-1) == 0) {
		flash_lock(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);
	}

	/* Initialize flash: 6 wait states for flash writing. */
	rc = flash_init(FLASH_ACCESS_MODE_128, 6);

	return rc;
}

int inv_flash_manager_eraseData(void)
{
	uint32_t rc;

	rc = flash_unlock(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);
	if (rc != FLASH_RC_OK) {
		goto LOCK_FLASH;
	}

	rc |= flash_erase_sector(ul_test_page_addr);
	if (rc != FLASH_RC_OK) {
		goto LOCK_FLASH;
	}

LOCK_FLASH :

	rc |= flash_lock(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);

	return rc;
}

int inv_flash_manager_readData(uint8_t* pData)
{
	uint32_t *pul_test_page = (uint32_t *) ul_test_page_addr;
	uint32_t FlashHeader = 0;
	int rc = 0;

	/* check Header */
	for (int i = 0; i < FLASH_HEADER_SIZE; i++)
		FlashHeader |= pul_test_page[i] << i * 8;
		
	if (FlashHeader == FLASH_HEADER) {
		for (int i = 0; i < FLASH_DATA_SIZE; i++)
			pData[i] = pul_test_page[i + FLASH_HEADER_SIZE];
	}
	else {
		memset(pData, 0, FLASH_DATA_SIZE);
		rc |= FLASH_HEADER_MISSING_RC;
	}

	return rc;
}


int inv_flash_manager_writeData(const uint8_t* pData)
{
	uint32_t rc;
	uint32_t ul_idx;
	uint32_t ul_page_buffer[IFLASH_PAGE_SIZE / sizeof(uint32_t)] = {0};

	/* Unlock page */
	rc = flash_unlock(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);
	if (rc != FLASH_RC_OK) {
		goto LOCK_FLASH;
	}
		
	/* Write Header */
	for (ul_idx = 0; ul_idx < FLASH_HEADER_SIZE; ul_idx++)
		ul_page_buffer[ul_idx] = (FLASH_HEADER & (0xFF << ul_idx * 8)) >> ul_idx * 8;

	/* Write page */
	for (ul_idx = 0; ul_idx < FLASH_DATA_SIZE; ul_idx++) {
		ul_page_buffer[ul_idx + FLASH_HEADER_SIZE] = *pData;
		pData ++;
	}

	rc |= flash_write(ul_test_page_addr, ul_page_buffer,
		IFLASH_PAGE_SIZE, 0);

	if (rc != FLASH_RC_OK) {
		goto LOCK_FLASH;
	}

LOCK_FLASH :
	rc |= flash_lock(ul_test_page_addr,
			ul_test_page_addr + IFLASH_PAGE_SIZE - 1, 0, 0);

	return rc;
}