#include "GUI_Paint.h"
#include "LCD_1in8.h"
#include "anime/photo.c"
#include "font12.c"
#include "font16.c"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "helper_functions.c"
#include "icons.c"
#include "lwip/apps/http_client.h"
#include "lwipopts.h"
#include "ntp_time.c"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#include "pico_clock.c"
#include "wifi_setup.c"
#include "wifi_setup.h"
#include <cyw43_country.h>
#include <hardware/rtc.h>
#include <lwip/err.h>
#include <lwip/tcp.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#define PIXEL_COUNT 61440 // number of pixels on the display
#define time_api                                                               \
  "https://api.ipgeolocation.io/"                                              \
  "timezone?apiKey=479c261c6f4a4cc5b022e3351058718b&tz=Europe/Amsterdam"
#define weather_api_key                                                        \
  "http://api.openweathermap.org/data/2.5/"                                    \
  "weather?q=Enschede&appid=1abe25c0ccddb9a35b2931cf9090a3b3&units=metric"
#define PAGE_SIZE 256
#define NVS_SIZE 4096
#define FLASH_WRITE_START (PICO_FLASH_SIZE_BYTES - NVS_SIZE)
#define FLASH_READ_START (FLASH_WRITE_START + XIP_BASE)

bool timerCB(repeating_timer_t *rt) {
  int r = 2 * 7;
  return true;
}

char *arrs[] = {arr_1,  arr_2,  arr_3,  arr_4,  arr_5,  arr_6,  arr_7,
                arr_8,  arr_9,  arr_10, arr_11, arr_12, arr_13, arr_14,
                arr_15, arr_16, arr_17, arr_18, arr_19, arr_20, arr_21,
                arr_22, arr_23, arr_24, arr_25, arr_26, arr_27};
char icon[100];

char temperature[10] = {0};
char curr_time[10] = {0};
int main(void) {
  stdio_init_all();
  // setting up wifi_name and password
  set_name_pass(ssid, pass);
  // setting up the wifi and connecting to it
  // configuring the wifi
  if (setup(country, ssid, pass, auth, "MyPicoW", NULL, NULL, NULL) == 3) {
    printf("Connected to wifi, good to go\n");
  }

  httpc_connection_t settings;
  uint16_t port = 80;
  settings.result_fn = result;
  settings.headers_done_fn = headers;

  // Initializing the display
  DEV_Delay_ms(100);
  if (DEV_Module_Init() != 0) {
    printf("Display could not be initialized\n");
    return -1;
  }
  LCD_1IN8_Init(HORIZONTAL);
  LCD_1IN8_Clear(WHITE);

  UDOUBLE Imagesize = 40 * 40 * 2;
  UWORD *BlackImage;
  if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
    printf("Failed to apply for black memory...\r\n");
    return 1;
  }
  Paint_NewImage((UBYTE *)BlackImage, LCD_1IN8.WIDTH, LCD_1IN8.HEIGHT, 0,
                 WHITE);
  Paint_SetScale(65);
  Paint_Clear(WHITE);
  Paint_SetRotate(ROTATE_0);

  int photoIndx = 0;

  // variable for cheching wheather new http request should be made
  // to fetch the data about the weather
  uint32_t prevWeather = to_ms_since_boot(get_absolute_time()),
           prevTime = to_ms_since_boot(get_absolute_time());
  // sync time every hour
  // fetch weather every 30 minutes
  uint32_t delayWeather = 1800000, delayTime = 3600000;
  // to check the state of the request
  err_t errWeather, errTime;
  datetime_t datetime;
  rtc_init();
  bool firstTime = true, firstWeather = true;
  while (true) {

    // check whether the delay time has passed
    // so that new request should be sent
    uint32_t nextWeather = prevWeather + delayWeather;
    if (firstWeather || to_ms_since_boot(get_absolute_time()) >= nextWeather) {

      errWeather =
          httpc_get_file_dns("api.openweathermap.org", 80,
                             "/data/2.5/"
                             "weather?q=Enschede&appid="
                             "1abe25c0ccddb9a35b2931cf9090a3b3&units=metric",
                             &settings, bodyWeather, NULL, NULL);
      prevWeather = to_ms_since_boot(get_absolute_time());
      firstWeather = false;
    }
    if (errWeather == ERR_OK && strlen(weatherBuffer) > 1) {
      get_temp_from_json(myBuff, temperature);
      errWeather = ERR_VAL;
    }
    // TODO: logic for time
    uint32_t nextTime = prevTime + delayTime;

    if (firstTime || to_ms_since_boot(get_absolute_time()) >= nextTime) {

      errTime = httpc_get_file_dns(
          "api.ipgeolocation.io", 80,
          "/timezone?apiKey=790488a8a8fa4fc18ca63bd7064f1d41&"
          "tz=Europe/Amsterdam",
          &settings, bodyTime, NULL, NULL);
      prevTime = to_ms_since_boot(get_absolute_time());
      firstTime = false;
    }
    if (errTime == ERR_OK && strlen(timeBuffer) > 1) {
      get_time_from_json(timeBuffer);
      errTime = ERR_VAL;
    }
    struct tm time_info;
    time_t current_time;

    // Get current time
    time(&current_time);
    localtime_r(&current_time, &time_info);
    printf("Current time: %d-%d-%d %d:%d:%d\n", time_info.tm_year + 1900,
           time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour,
           time_info.tm_min, time_info.tm_sec);
    sprintf(curr_time, "%d:%d", time_info.tm_hour, time_info.tm_min);
    // TODO: logic for icons
    /*  get_curr_icon(myBuff, icon);*/
    // TODO:
    // convert all images to hex and store them in a file
    // based on the curr icon fetched from the weather api
    // display the image on the screen
    // UPD:
    // almost done, need to convert to diff resolutions
    // for diff formats of displaying data

    Paint_DrawImage(arrs[photoIndx++], 0, 1, 160, 128);
    if (photoIndx == 26)
      photoIndx = 0;

    if (temperature != NULL && strlen(temperature) > 0) {
      Paint_DrawString_EN(0, 1, temperature, &Font16, WHITE, BLACK);
    }
    if (curr_time != NULL && strlen(curr_time) > 0) {
      // for bigger font(16)
      Paint_DrawString_EN(105, 1, curr_time, &Font16, WHITE, BLACK);
    }
    Paint_DrawImage(d01, 12, 1, 14, 14);
    LCD_1IN8_Display(BlackImage);
    /*DEV_Delay_ms(10);*/
  }
  free(BlackImage);
  BlackImage = NULL;
  DEV_Module_Exit();
  return 0;
}
