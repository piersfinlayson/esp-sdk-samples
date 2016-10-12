/*
 * Copyright (C) 2016 Piers Finlayson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * A simple test program designed to erase and write a sector at 1MB intervals on the 
 * installed flash chip, up to the detected flash size.  (The maximum ESP8266 claims to
 * support is 16MB, but the SDK spi_flash functions as they stand don't support >4MB
 * hence the wrapper functions starting _spi_*.
 */


#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

extern SpiFlashChip  flashchip;

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

SpiFlashChip *_flashchip;
uint32_t flash_size_sdk;
uint32_t flash_size_actual;
uint8_t byte_to_write = 0x80;
uint32_t address;
#define ADDRESS_INITIAL    0x100000
#define ADDRESS_INCREMENT  0x100000
#define ADDRESS_FINAL      0xFFFFFF
#define SECTOR_SIZE        0x1000

uint32_t write_buf[SECTOR_SIZE/(sizeof(uint32_t))];
uint32_t read_buf[SECTOR_SIZE/(sizeof(uint32_t))];

SpiFlashOpResult _spi_flash_erase_sector(uint16 sector)
{
  int8 status=0;

  _flashchip->chip_size = flash_size_actual;

  status = spi_flash_erase_sector(sector);

  _flashchip->chip_size = flash_size_sdk; // restore chip size

  return status;
}

SpiFlashOpResult _spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size)
{
  int8 status=0;

  _flashchip->chip_size = flash_size_actual;

  status = spi_flash_write(des_addr, src_addr, size);

  _flashchip->chip_size = flash_size_sdk; // restore chip size

  return status;
}

SpiFlashOpResult _spi_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size)
{
  int8 status=0;
  
  _flashchip->chip_size = flash_size_actual;

  status = spi_flash_read(src_addr, des_addr, size);

  _flashchip->chip_size = flash_size_sdk; // restore chip size

  return status;
}


void some_timerfunc(void *arg)
{
  uint8 rc;

  if (address > ADDRESS_FINAL)
  {
    ets_printf("Done\r\n");
    
    //Disarm timer
    os_timer_disarm(&some_timer);

    goto EXIT_LABEL;
  }

  byte_to_write++;

  // Erase a sector and check it's erased
  ets_printf("Trying to erase: 0x%x ... ", address);
  rc = _spi_flash_erase_sector(address/SECTOR_SIZE);
  if (rc)
  {
    ets_printf("failed to erase sector: 0x%x", address/SECTOR_SIZE);
    if (address > flash_size_actual)
    {
      ets_printf(" ... as expected");
    }
  }
  else
  {
    os_memset(write_buf, 0xff, SECTOR_SIZE);
    rc = _spi_flash_read(address, read_buf, SECTOR_SIZE);
    if (os_memcmp(write_buf, read_buf, SECTOR_SIZE))
    {
      ets_printf("failed - sector has not been erased correctly\r\n");
      ets_printf("First four bytes of sector: 0x%08x", read_buf[0]);
    }
    else
    {
      ets_printf("success");
    }
  }
  ets_printf("\r\n");
  
  // Write a sector and check it's written
  ets_printf("Trying to write: 0x%x ... ", address);
  os_memset(write_buf, byte_to_write, SECTOR_SIZE);
  rc = _spi_flash_write(address, write_buf, SECTOR_SIZE);
  if (rc)
  {
    ets_printf("failed to write data\r\n");
    if (address > flash_size_actual)
    {
      ets_printf(" ... as expected");
    }
  }
  else
  {
    rc = _spi_flash_read(address, read_buf, SECTOR_SIZE);
    if (os_memcmp(write_buf, read_buf, SECTOR_SIZE))
    {
      ets_printf("failed - sector has not been written correctly\r\n");
      ets_printf("First four bytes of sector: 0x%08x", read_buf[0]);
    }
    else
    {
      ets_printf("success");
    }
  }
  ets_printf("\r\n");

EXIT_LABEL:
  
  address += ADDRESS_INCREMENT;

  return;
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
  os_delay_us(10);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
  uint32_t flash_id;
  uint8_t flash_man;
  uint8_t flash_size;
  int ii;

  // Disable SDK logging
  system_set_os_print(0);

  //Disarm timer
  os_timer_disarm(&some_timer);

  //Setup timer
  os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

  //Arm the timer
  //&some_timer is the pointer
  //1000 is the fire time in ms
  //0 for once and 1 for repeating
  os_timer_arm(&some_timer, 1000, 1);
    
  //Start os task
  system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

  // Get flash ID
  flash_id = spi_flash_get_id();
  ets_printf("Flash ID: 0x%x\r\n", flash_id);
  flash_man = flash_id & 0xff;
  flash_size = (flash_id >> 16) & 0xff;
  ets_printf("Flash manufacturer: ");
  switch (flash_man)
  {
    case 0xef:
      ets_printf("winbond");
      break;
   
    default:
      ets_printf("unknown");
      break;
  }
  ets_printf("\r\n");
  ets_printf("ESP8266 flashchip struct:  0x%x\r\n", (uint32_t)flashchip.deviceId);
  _flashchip = (SpiFlashChip *)flashchip.deviceId;
  ets_printf("ESP8266 flash deviceId:    0x%x\r\n", _flashchip->deviceId);
  ets_printf("ESP8266 flash chip_size:   0x%x\r\n", _flashchip->chip_size);
  ets_printf("ESP8266 flash block_size:  0x%x\r\n", _flashchip->block_size);
  ets_printf("ESP8266 flash sector_size: 0x%x\r\n", _flashchip->sector_size);
  ets_printf("ESP8266 flash page_size:   0x%x\r\n", _flashchip->page_size);
  ets_printf("ESP8266 flash status_mask: 0x%x\r\n", _flashchip->status_mask);

  // For some reason everything is offset by 4 bytes - so use block size instead!
  flash_size_sdk = _flashchip->chip_size;
  ets_printf("SDK flash size:    %d bytes\r\n", flash_size_sdk);
  
  flash_size_actual = 1;
  for (ii = 0; ii < flash_size; ii++)
  {
    flash_size_actual *= 2;
  }
  ets_printf("Actual flash size: %d bytes\r\n", flash_size_actual);

  address = ADDRESS_INITIAL;
}
