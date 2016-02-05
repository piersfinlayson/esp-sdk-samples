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

void some_timerfunc(void *arg)
{
  os_printf("Resetting ...\n"); 

  // Pause to allow test to output before resetting
  os_delay_us(1000);

  // Reset by pulling reset GPIO (connected to reset) low
  // XXX Only works for GPIO 16
  WRITE_PERI_REG(RTC_GPIO_OUT,
    (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(0));
  
  // No need to reschedule in case pin wasn't connected as we 
  // set up a timer to run forever
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
  // Set up serial logging
  uart_div_modify(0, UART_CLK_FREQ / 115200);

  // Set GPIO 16 (reset GPIO) to high 
  gpio_init();
  WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
    (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);      // mux configuration for XPD_DCDC to output rtc_gpio0

  WRITE_PERI_REG(RTC_GPIO_CONF,
    (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);  //mux configuration for out enable

  WRITE_PERI_REG(RTC_GPIO_ENABLE,
    (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);        //out enable

  WRITE_PERI_REG(RTC_GPIO_OUT,
    (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(1));

  //Disarm timer
  os_timer_disarm(&some_timer);

  //Setup timer
  os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

  // Set the timer for 10s
  os_timer_arm(&some_timer, 10000, 1);
  os_printf("Resetting in 10s ...\n");
    
  //Start os task
  system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}
