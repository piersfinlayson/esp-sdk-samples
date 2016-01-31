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

bool connected;
bool connecting;
bool dhcpc_started;
struct station_config wifi_conf;

void some_timerfunc(void *arg)
{
  int wifi_status;
 
  if ((!connected) && (!connecting))
  {
    strcpy((char *)wifi_conf.ssid, "geginfedw");
    strcpy((char *)wifi_conf.password, "geginfedw");
    wifi_conf.bssid_set = FALSE;
    ETS_UART_INTR_DISABLE();
    wifi_station_set_config_current(&wifi_conf);
    ETS_UART_INTR_ENABLE();
    connected = wifi_station_connect();
  }

  if (connecting)
  {
    wifi_status = wifi_station_get_connect_status();
    switch (wifi_status)
    {
      case STATION_GOT_IP:
      connecting = FALSE;
      connected = TRUE;
      break;

      case STATION_IDLE:
      case STATION_CONNECTING:
        // Still going
        break;

      case STATION_WRONG_PASSWORD:
      case STATION_NO_AP_FOUND:
      case STATION_CONNECT_FAIL:
      default:
        connecting = FALSE;
        connected = FALSE;
        wifi_station_disconnect();
        break;
    }
  }

  if (connected && !dhcpc_started)
  {
    dhcpc_started = wifi_station_dhcpc_start();
  }
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
  // Set Wifi mode to station but don't save to flash
  wifi_status_led_install(4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
  wifi_set_opmode_current(STATION_MODE);
  connected = FALSE;
  connecting = FALSE;
  dhcpc_started = FALSE;

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
