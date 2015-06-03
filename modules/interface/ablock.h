/**
 *    ||          ____  _ __                           
 * +------+      / __ )(_) /_______________ _____  ___ 
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * ablock.h - Used to send ablock data to the client
 */

#ifndef ABLOCK_H_
#define ABLOCK_H_

#include <stdint.h>
#include <stdbool.h>
#include <uart_extgps.h>

#define ABLOCK_WDT_TIMEOUT  M2T(4000)

/**
 * Initialize the ablock
 */
void ablockInit(void);

bool ablockTest(void);

void ablockWatchdog(void);
uint32_t ablockGetInactivityTime(void);

/**
 * Put a character to the ablock buffer
 *
 * @param ch character that shall be sent
 * @return The character casted to unsigned int or EOF in case of error
 */
int ablockPutchar(int ch);

/**
 * Put a null-terminated string on the ablock buffer
 *
 * @param str Null terminated string
 * @return a nonnegative number on success, or EOF on error. 
 */
int ablockPuts(char *str);

/**
 * Flush the ablock buffer
 */
void ablockFlush(void);


#endif /*ABLOCK_H_*/
