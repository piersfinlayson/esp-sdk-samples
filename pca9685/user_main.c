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

bool pca9685_inited = FALSE;
bool pca9685_on = FALSE;
#define PCA9685_ADDR 0x40

uint8_t init_i2c(void)
{
  ets_printf("I2C stack init ... ");
  brzo_i2c_setup(100);
  ets_printf("success\r\n");
  return 0;
}

uint8_t reset_i2c(void)
{
  uint8_t rc;
  uint8_t bytes[2];

  ets_printf("I2C Software Reset Call ... ");
  bytes[0] = 0;  // general call address
  bytes[1] = 0b00000110; // SWRST data
  brzo_i2c_start_transaction(PCA9685_ADDR, 100);
  brzo_i2c_write(bytes, 2, FALSE);
  rc = brzo_i2c_end_transaction();
  if (!rc)
  {
    ets_printf("success\r\n");
  }
  else
  {
    ets_printf("failed %d\r\n", rc);
  }

  return rc;
}

uint8_t init_pca9685(uint8_t addr)
{
  uint8_t rc;
  uint8_t bytes[2];

  ets_printf("PCA9685 init ... ");
  // Write MODE1 register (key bits are to set auto-increment and turn off sleep mode
  bytes[0] = 0x00; // MODE1 register
  bytes[1] = 0b00100001; // reset = 1, AI = 1, sleep = 0, allcall = 1
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(bytes, 2, FALSE);
  rc = brzo_i2c_end_transaction();
  if (rc)
  {
    ets_printf("failed write: %d\r\n", rc);
    goto EXIT_LABEL;
  }

  // Now read back the register to check it took
  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(bytes, 1, FALSE);
  brzo_i2c_read(bytes, 1, FALSE);
  rc = brzo_i2c_end_transaction();
  if (!rc)
  {
    if (bytes[0] == bytes[1])
    {
      ets_printf("success\r\n");
      goto EXIT_LABEL;
    }
    else
    {
      ets_printf("failed read mismatch: 0x%02x\r\n", bytes[0]);
    }
  }
  else
  {
    ets_printf("failed read: %d\r\n", rc);
  }

EXIT_LABEL:

  return rc;
}

void toggle_pca9685_leds(uint8_t addr, bool desired_state)
{
  uint8_t rc;
  bool will_be_on;
  uint8_t bytes[8];

  // Turn all LEDs on or off
  // To turn specific LEDs on or off set byte 0 (register) to 0x06 for LED 0,
  // 0x0A for LED 1, 0x0E for LED2, ...
  bytes[0] = 0xfa; // All LEDs
  // bytes[0] = 0x06; // LED 0 only
  ets_printf("Turn LED(s) ");
  if (!desired_state)
  {
    // Turn off
    ets_printf("off ... ");
    bytes[1] = 0;
    bytes[2] = 0;
    bytes[3] = 0;
    bytes[4] = 0b00010000;
    will_be_on = FALSE;
  }
  else
  {
    // Turn on
    ets_printf("on  ... ");
    bytes[1] = 0;
    bytes[2] = 0b00010000;
    bytes[3] = 0;
    bytes[4] = 0;
    will_be_on = TRUE;
  }

  brzo_i2c_start_transaction(addr, 100);
  brzo_i2c_write(bytes, 5, FALSE);
  rc = brzo_i2c_end_transaction();

  if (rc) 
  {
    ets_printf("failed: %d\r\n", rc);
    goto EXIT_LABEL;
  }

  if (!rc)
  {
    ets_printf("success\r\n");
  }

  pca9685_on = will_be_on;

EXIT_LABEL:

  return;
}

void some_timerfunc(void *arg)
{
  uint8_t rc;

  // Initialise everything if not yet successfully initalised
  if (!pca9685_inited)
  {
    rc = init_i2c();
    if (rc)
    {
      goto EXIT_LABEL;
    }

    rc = reset_i2c();
    if (rc)
    {
      goto EXIT_LABEL;
    }

    rc = init_pca9685(PCA9685_ADDR);
    if (!rc)
    {
      pca9685_inited = TRUE;
    }

    toggle_pca9685_leds(PCA9685_ADDR, FALSE);

    goto EXIT_LABEL;
  }

  toggle_pca9685_leds(PCA9685_ADDR, pca9685_on ? FALSE : TRUE);

EXIT_LABEL:

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
    //Disarm timer
    os_timer_disarm(&some_timer);

    //Setup timer
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

    //Arm the timer
    //&some_timer is the pointer
    //1000 is the fire time in ms
    //0 for once and 1 for repeating
    os_timer_arm(&some_timer, 2000, 1);
    
    //Start os task
    system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
}
