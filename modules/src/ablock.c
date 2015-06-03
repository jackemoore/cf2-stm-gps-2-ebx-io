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
 * ablock.c - Used to send ablock data to client
 */
#define DEBUG_MODULE "AB"

#include <stdbool.h>

/*FreeRtos includes*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "ablock.h"

#include "crtp.h"

#include "debug.h"

#define max_retries 10
#define max_Retries 10

CRTPPacket messageToSend;
xSemaphoreHandle synch2 = NULL;

static void ablockCrtpCB(CRTPPacket* pk);
//int ablockPuts(char *str);

static const char fullMsg[] = "<F>\n";
static bool isInit;

static uint32_t lastUpdate;
static bool isInactive = true;

static void ablockWatchdogReset(void);

/**
 * Send the data to the client
 * returns TRUE if successful otherwise FALSE
 */
static bool ablockSendMessage(void)
{

  if (crtpSendPacket(&messageToSend) == pdTRUE)
  {
    messageToSend.size = 0;
  }
  else
  {
    return false;
  }

  return true;
}

void ablockInit()
{
  if (isInit)
    return;

  messageToSend.size = 0;
  messageToSend.header = CRTP_HEADER(CRTP_PORT_ABLOCK, 0);
  vSemaphoreCreateBinary(synch2);
  
  crtpRegisterPortCB(CRTP_PORT_ABLOCK, ablockCrtpCB);

  lastUpdate = xTaskGetTickCount();
  isInactive = true;

  isInit = true;
}

bool ablockTest(void)
{
  return isInit;
}

static void ablockWatchdogReset(void)
{
  lastUpdate = xTaskGetTickCount();
  isInactive = false;
}

uint32_t ablockGetInactivityTime(void)
{
  return xTaskGetTickCount() - lastUpdate;
}

int i, indx = 0, len = 0, retries = 0, Retries = 0, lastlen = 0;
bool assistNow = false,  newline = false, endblock = false, assistReq = false;
bool newReq = false, expecting = false, samemsg = true;
char message[100], lastmsg[31];


static void ablockCrtpCB(CRTPPacket* pk)
{
// Integer values for control characters:
//  "S" 83 (begin AssistNow); "E" 69 (end AssistNow); "X" 88 (abort AssistNow);
//  "G" 71 (ok to proceed); "n" 110 (request newline);
//  "r" 114 (repeat last line); "R" 82 (repeat last block);
//  "<" 60 & ">" 62 (at beginning of newline enclosing control character or
//   when together at end of last line signifying end of block)

	char payload[31], ca, cb, cka, ckb;

	expecting = false;
	ablockWatchdogReset();

	len = pk->size;
//	DEBUG_PRINT("insize %d\n", len);

	newline = false;

	if ((len > 0) & (len <= 30))
	{
		for (i=0; i<len; i++)
		{
		    payload[i] = pk->data[i];
		}
		payload[i] = 0;
		DEBUG_PRINT("indata %s", payload); //payload has "\n"
		newline = true;
	}

	if ((len >= 3) & (payload[0] == 60) & (payload[2] == 62))
	{
		if (payload[1] == 83 )
		{
		    assistReq = true;
		    indx = 0;
		    endblock = false;
		    retries = 0;
		    Retries = 0;
	        ablockPuts(payload);
	        expecting = true;
	        lastlen = 0;
		}
	    else
	    {

	    	if (assistReq & (payload[1] == 71 ))
	        {
	        	assistNow = true;
	    		payload[1] = 110;
	            ablockPuts(payload);
	            expecting = true;
	        }
	        if (assistNow & (payload[1] == 69 ))
	        {
	        	assistNow = false;
	        	assistReq = false;
	        }
	        if (assistNow & (payload[1] == 88 ))
	        {
	        	assistNow = false;
	        	assistReq = false;
	        }
	    }
    }
	else
	{
        if (assistNow & newline)
        {
        	samemsg = true;
        	len--;
        	if ((payload[len-2] == 60) & (payload[len-1] == 62))
        	{
        	    len -= 2;
        	    endblock = true;
        	}
        	if (len != lastlen)
        	{
        	    lastlen = len;
        	    samemsg = false;
        	}
        	if (len > 0)
        	{
        		for (i=0; i<len; i += 2)
        	    {
        	        ca = payload[i] - 48;
        	        if (ca > 9)
        	        {
        	        	ca = payload[i] - 55;
        	        }
        	        if (ca != lastmsg[i])
        	        {
        	        	lastmsg[i] = ca;
        	        	samemsg = false;
        	        }
        	        cb = payload[i+1] - 48;
        	        if (cb > 9)
        	        {
        	            cb = payload[i+1] - 55;
        	        }
        	        if (cb != lastmsg[i+1])
        	        {
        	            lastmsg[i+1] = cb;
        	            samemsg = false;
        	        }
        	        if (!samemsg)
        	        {
        	        	message[indx] = (ca * 16) + (cb & 15);
        	        	indx++;
        	        }
        	    }
        	}
        	if (samemsg)
        	{
        		endblock = false;
        		newReq = false;
        	}

//        	DEBUG_PRINT("len %d indx %d M0 %d M1 %d\n",len,indx,message[0],message[1]);
        	newReq = true;
        	if (endblock & (indx > 0) & !samemsg)
        	{
        	    message[indx] = 0;

           	    cka = 0;
        	    ckb = 0;
        	    for (i=2; i<indx-2; i++)
        	    {
        	    	cka += message[i];
        	    	ckb += cka;
        	    }
        	    if ((cka == message[indx-2]) && (ckb == message[indx-1]))
        	    {
        	    	uartExtgpsSendData(indx, (uint8_t*)message);
                    retries = 0;
                    Retries = 0;
        	        DEBUG_PRINT("Sent to Gps\n");
        	    }
        	    else
				{
//        	    	DEBUG_PRINT("cka %d ckb %d A %d B %d\n",cka,ckb,message[indx-2],message[indx-1]);
         	    	Retries++;
         	    	if (Retries > max_Retries)
         	    	{
         	    		assistNow = false;
         	    		assistReq = false;
         	    		newReq = false;
         	    		ablockPuts("<X>\n");
         	    	}
         	    	else
         	    	{
         	    		newReq = false;
         	    		ablockPuts("<R>\n");
         	    		expecting = true;
         	    	}
				}
        	    endblock = false;
        	    indx = 0;
        	}
        	if (newReq)
        	{
        		ablockPuts("<n>\n");
        		expecting = true;
        	}
        }
	}
//	DEBUG_PRINT("outdata %s", payload); //payload has "\n"
}

void ablockWatchdog(void)
{
  uint32_t ticktimeSinceUpdate;

  ticktimeSinceUpdate = xTaskGetTickCount() - lastUpdate;

  if ((ticktimeSinceUpdate > ABLOCK_WDT_TIMEOUT) & assistNow & expecting)
  {
	  if (!isInactive)
	  {
	      isInactive = true;
	      retries++;
	      if (retries > max_retries)
	      {
	          assistNow = false;
	          assistReq = false;
	          ablockPuts("<X>\n");
	      }
	      else
	      {
	          ablockPuts("<r>\n");
	      }
	      ablockWatchdogReset();
	  }
  }
  else
  {
    isInactive = false;
  }
}

int ablockPutchar(int ch)
{
  int i;

  if (xSemaphoreTake(synch2, portMAX_DELAY) == pdTRUE)
  {
    if (messageToSend.size < CRTP_MAX_DATA_SIZE)
    {
      messageToSend.data[messageToSend.size] = (unsigned char)ch;
      messageToSend.size++;
    }
    if (ch == '\n' || messageToSend.size >= CRTP_MAX_DATA_SIZE)
    {
      if (crtpGetFreeTxQueuePackets() == 1)
      {
        for (i = 0; i < sizeof(fullMsg) && (messageToSend.size - i) > 0; i++)
        {
          messageToSend.data[messageToSend.size - i] =
              (uint8_t)fullMsg[sizeof(fullMsg) - i - 1];
        }
      }
      ablockSendMessage();
    }
    xSemaphoreGive(synch2);
  }
  
  return (unsigned char)ch;
}

int ablockPuts(char *str)
{
  int ret = 0;
  
  while(*str)
    ret |= ablockPutchar(*str++);
  
  return ret;
}

void ablockFlush(void)
{
  if (xSemaphoreTake(synch2, portMAX_DELAY) == pdTRUE)
  {
    ablockSendMessage();
    xSemaphoreGive(synch2);
  }
}
