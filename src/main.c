/*
 * Copyright (c) 2022 Lukasz Majewski, DENX Software Engineering GmbH
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample which uses the filesystem API with littlefs */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include "filefs.h"
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(main);
#define SIZE_BUFF 256
#define NUMBER 256
#define WRITE_SIZE 64

/* size of stack area used by each thread */
#define STACKSIZE 4096

/* scheduling priority used by each thread */
#define PRIORITY 7


typedef struct log
{
	uint8_t data[SIZE_BUFF];
} log;

uint8_t write[SIZE_BUFF];
uint8_t read[SIZE_BUFF];
struct fs_file_t filelog;

void file_task(void)
{
	 file_system_init();

	memset(write, 9, SIZE_BUFF);
	write[0] = 10;
	write[1] = 20;
	LOG_INF("Write file");
	uint32_t system_tick = sys_clock_tick_get_32();
	 fs_write_file_index("",  USER_INFOR_FULL_PATH, write, SIZE_BUFF, 0);
	uint32_t elapsed_ms = k_cyc_to_ms_near32((sys_clock_tick_get_32() - system_tick));
	LOG_INF("Write file take %dms", elapsed_ms);
	
	// test read file
	memset(read, 0, SIZE_BUFF);
	LOG_INF("READ file");
	system_tick = sys_clock_tick_get_32();
	fs_read_file_index("",  USER_INFOR_FULL_PATH, read, SIZE_BUFF, 0);
	elapsed_ms = k_cyc_to_ms_near32((sys_clock_tick_get_32() - system_tick));
	LOG_INF("READ file take %dms", elapsed_ms);
    
	if(memcmp(read,write, SIZE_BUFF) ==0 ){
 	LOG_INF("Test successfull");
	}else{
	LOG_INF("Test Fail");
	}

	while (1)
	{

		k_msleep(500);
	}
}


K_THREAD_DEFINE(file_id, STACKSIZE, file_task, NULL, NULL, NULL,
								PRIORITY, 0, 0);