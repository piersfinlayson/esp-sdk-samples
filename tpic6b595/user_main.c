#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

static int next = 0;

#define ENABLE_MASK BIT4|BIT5|BIT12
#define SLEEP 1 

void some_timerfunc(void *arg)
{
  uint32 set_mask = 0;
  uint32 clear_mask = 0;
  int ii;

  ets_printf("Setting bits to %d\r\n", next);

  for (ii = 0; ii < 24; ii++)
  {
    // Send bit
    if (next)
    {
      set_mask &= ~BIT4;
      clear_mask |= BIT4; 
    }
    else
    {
      set_mask |= BIT4;
      clear_mask &= ~BIT4; 
    }
    //ets_printf("  output: 0x%8.8x 0x%8.8x\r\n", set_mask, clear_mask);
    gpio_output_set(set_mask, clear_mask, ENABLE_MASK, 0);
    os_delay_us(SLEEP);

    // Send clock
    set_mask &= ~BIT5;
    clear_mask |= BIT5; 
    //ets_printf("  output: 0x%8.8x 0x%8.8x\r\n", set_mask, clear_mask);
    gpio_output_set(set_mask, clear_mask, ENABLE_MASK, 0);
    os_delay_us(SLEEP);

    // Clear clock
    set_mask |= BIT5;
    clear_mask &= ~BIT5; 
    //ets_printf("  output: 0x%8.8x 0x%8.8x\r\n", set_mask, clear_mask);
    gpio_output_set(set_mask, clear_mask, ENABLE_MASK, 0);
    os_delay_us(SLEEP);

    if ((ii == 7) || (ii == 15))
    {
      next = next ? 0 : 1;
    }
  }
   
  set_mask = 0;
  clear_mask = 0;
  if (next)
  {
    // Clear last bit
    set_mask |= BIT4;
    clear_mask &= ~BIT4; 
    //ets_printf("  output: 0x%8.8x 0x%8.8x\r\n", set_mask, clear_mask);
    gpio_output_set(set_mask, clear_mask, ENABLE_MASK, 0);
    os_delay_us(SLEEP);
  }

  // Register lock
  set_mask = 0;
  clear_mask = BIT12;
  //ets_printf("  output: 0x%8.8x 0x%8.8x\r\n", set_mask, clear_mask);
  gpio_output_set(set_mask, clear_mask, ENABLE_MASK, 0);
  os_delay_us(SLEEP);
  set_mask = BIT12;
  clear_mask = 0;
  //ets_printf("  output: 0x%8.8x 0x%8.8x\r\n", set_mask, clear_mask);
  gpio_output_set(set_mask, clear_mask, ENABLE_MASK, 0);
  os_delay_us(SLEEP);

  // Set next to different value for next time
  next = next ? 0 : 1;
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
    // Initialize the GPIO subsystem.
    gpio_init();

    //Set GPIO4, 5 and 12 to output mode
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,  FUNC_GPIO12);

    //Set GPIO4, 5 and 12 low
    gpio_output_set(BIT4|BIT5|BIT12, 0, BIT4|BIT5|BIT12, 0);

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
}
