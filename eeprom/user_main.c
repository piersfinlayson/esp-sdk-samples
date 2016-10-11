#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

uint32_t address;
#define ADDRESS_INITIAL    0x100000
#define ADDRESS_INCREMENT  0x100000
#define ADDRESS_FINAL      0xFFFFFF
#define SECTOR_SIZE        0x1000

uint32_t write_buf[SECTOR_SIZE/(sizeof(uint32_t))] = {0};

void some_timerfunc(void *arg)
{
  uint8 rc;

  if (address > ADDRESS_FINAL)
  {
    ets_printf("Done\r\n");
    
    //Disarm timer
    os_timer_disarm(&some_timer);
    return;
  }
  else
  {
    ets_printf("Trying to write: 0x%x ... ", address);
    rc = spi_flash_erase_sector(address/SECTOR_SIZE);
    if (rc)
    {
      ets_printf("failed to erase sector: 0x%x\r\n", address/SECTOR_SIZE);
    }
    else
    {
      rc = spi_flash_write(address, write_buf, SECTOR_SIZE);
      if (rc)
      {
        ets_printf("failed to write data\r\n");
      }
      else
      {
        ets_printf("success\r\n");
      }

    }
    
    address += ADDRESS_INCREMENT;
  }
  
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
  uint32_t flash_size_byte;
  int ii;

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
  flash_size_byte = 1;
  for (ii = 0; ii < flash_size; ii++)
  {
    flash_size_byte *= 2;
  }
  ets_printf("Flash size: %d bytes\r\n", flash_size_byte);
  
  address = ADDRESS_INITIAL;
}
