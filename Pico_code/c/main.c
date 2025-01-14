﻿#include "DEV_Config.h"
#include "DEV_config.h"
#include "GUI_Paint.h"
#include "LCD_1in8.h"
#include "anime/photo.c"
#include "cJSON.c"
#include "cJSON.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "lib/Fonts/font12.c"
#include "lwip/apps/http_client.h"
#include "lwipopts.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include <cyw43_country.h>
#include <hardware/rtc.h>
#include <lwip/tcp.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
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

int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth,
          const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);

char myBuff[2000];
char timeBuff[2000];
bool timerCB(repeating_timer_t *rt) {
  int r = 2 * 7;
  return true;
}
void result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len,
            u32_t server_res, err_t err) {
  printf("%s\n", (char *)arg);
  printf("transfer complete!\n");
  printf("local result = %d\n", httpc_result);
  printf("http result = %d\n", server_res);
}

err_t headers(httpc_state_t *connection, void *arg, struct pbuf *hdr,
              u16_t hdr_len, u32_t content_len) {
  printf("Headers received\n");
  printf("Content length: %u\n", content_len);
  printf("Header length: %u\n", hdr_len);

  size_t copy_len =
      hdr->tot_len < sizeof(myBuff) - 1 ? hdr->tot_len : sizeof(myBuff) - 1;
  pbuf_copy_partial(hdr, myBuff, copy_len, 0);
  myBuff[copy_len] = '\0'; // Null-terminate the buffer
  printf("Headers:\n%s\n", myBuff);
  printf("Buffer end\n");

  return ERR_OK;
}

err_t body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
  printf("body\n");
  pbuf_copy_partial(p, myBuff, p->tot_len, 0);
  printf("%s\n", myBuff);
  // actual contens of a website
  // data that is begin fetched
  printf("Contents of pbuf %s\n", (char *)p->payload);
  if (p->tot_len > 0) {
    pbuf_free(p);
  }
  return ERR_OK;
}
void get_time_from_json(const char *json, char *time) {
  cJSON *json_obj = cJSON_Parse(json);
  if (json_obj == NULL) {
    printf("Error parsing JSON\n");
    return;
  }
  cJSON *time_obj = cJSON_GetObjectItem(json_obj, "time_24");
  if (time_obj == NULL) {
    printf("Error finding 'time_24' object in JSON\n");
    cJSON_Delete(json_obj);
    return;
  }
  strcpy(time, time_obj->valuestring);
  cJSON_Delete(json_obj);
}
int get_temperature_from_json(const char *json) {
  // Parse the JSON data
  cJSON *json_obj = cJSON_Parse(json);
  if (json_obj == NULL) {
    printf("Error parsing JSON just at the start, temp\n");
    return 1;
  }

  // Navigate through the JSON structure to get the "temp" field under "main"
  cJSON *main = cJSON_GetObjectItem(json_obj, "main");
  if (main == NULL) {
    printf("Error finding 'main' object in JSON\n");
    cJSON_Delete(json_obj);
    return 1;
  }

  cJSON *temp = cJSON_GetObjectItem(main, "temp");
  if (temp == NULL) {
    printf("Error finding 'temp' field in 'main' object\n");
    cJSON_Delete(json_obj);
    return 1;
  }

  // Print the temperature
  printf("Current temp: %f\n", temp->valuedouble);
  printf("in int %d\n", (int)temp->valuedouble);
  int res = temp->valuedouble;
  // Clean up the JSON object
  cJSON_Delete(json_obj);
  return res;
}
void get_pass_name_wifi(char *name, char *pass) {
  bool pass_entered = false, name_entered = false;
  char wifi[100] = "wifi:";
  char key[100] = "pass:";

  int wifi_len = strlen(wifi);
  int key_len = strlen(key);
  while (!pass_entered || !name_entered) {
    if (stdio_usb_connected()) {
      char buffer[128];
      int len = scanf("%127s", buffer); // Read user input
      if (len > 0) {
        printf("You sent: %s\n", buffer);

        // Check if buffer starts with "wifi:"
        if (!name_entered && strncmp(buffer, wifi, wifi_len) == 0) {
          memcpy(name, buffer + wifi_len,
                 strlen(buffer) - wifi_len);      // Copy the part after "wifi:"
          name[strlen(buffer) - wifi_len] = '\0'; // Null-terminate the name
          name_entered = true;
        }
        // Check if buffer starts with "pass:"
        else if (!pass_entered && strncmp(buffer, key, key_len) == 0) {
          memcpy(pass, buffer + key_len,
                 strlen(buffer) - key_len);      // Copy the part after "pass:"
          pass[strlen(buffer) - key_len] = '\0'; // Null-terminate the pass
          pass_entered = true;
        }
      }
    }
  }
}

void parse_wifi_credentials(const char *input, char *wifi, char *pass) {
  // Helps to parse the input string to the right format
  // wifi_name:password
  const char *colon = strchr(input, ':');
  if (colon == NULL) {
    printf("Invalid format. Expected 'wifi:pass'\n");
    wifi[0] = '\0';
    pass[0] = '\0';
    return;
  }
  size_t wifi_len = colon - input;
  strncpy(wifi, input, wifi_len);
  wifi[wifi_len] = '\0';
  strcpy(pass, colon + 1);
}
void set_name_pass(char *ssid, char *pass) {
  while (true) {
    if (stdio_usb_connected()) {
      char buffer[128];
      if (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        parse_wifi_credentials(buffer, ssid, pass);
        break;
      }
    }
  }
}
void get_curr_time(char *curr_time) {
  /*datetime_t t;*/
  /*rtc_get_datetime(&t);*/
  /*printf("current time: %d:%d:%d\n", t.hour, t.min, t.sec);*/
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  int hour = timeinfo->tm_hour;
  int minutes = timeinfo->tm_min;
  snprintf(curr_time, 6, "%02d:%02d", hour, minutes);
}
char ssid[64] = {0};
char pass[64] = {0};
char curr_time[100];
const uint32_t country = CYW43_COUNTRY_WORLDWIDE;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;
char *arrs[] = {arr_1,  arr_2,  arr_3,  arr_4,  arr_5,  arr_6,  arr_7,
                arr_8,  arr_9,  arr_10, arr_11, arr_12, arr_13, arr_14,
                arr_15, arr_16, arr_17, arr_18, arr_19, arr_20, arr_21,
                arr_22, arr_23, arr_24, arr_25, arr_26, arr_27};

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

  bool flag = true;
  char temperature[100];
  int i = 0;
  // http request
  /*err_t err =*/
  /*    httpc_get_file_dns("api.openweathermap.org", 80,*/
  /*                       "/data/2.5/"*/
  /*                       "weather?q=Enschede&appid="*/
  /*                       "1abe25c0ccddb9a35b2931cf9090a3b3&units=metric",*/
  /*                       &settings, body, NULL, NULL);*/
  /*if (err != ERR_OK) {*/
  /*  printf("HTTP request failed with error: %d\n", err);*/
  /*  return -1;*/
  /*}*/

  uint32_t currTime = to_ms_since_boot(get_absolute_time());
  uint32_t delay = 10000;
  while (true) {

    uint32_t nextTime = currTime + delay;
    if (to_ms_since_boot(get_absolute_time()) >= nextTime) {

      httpc_get_file_dns("api.openweathermap.org", 80,
                         "/data/2.5/"
                         "weather?q=Enschede&appid="
                         "1abe25c0ccddb9a35b2931cf9090a3b3&units=metric",
                         &settings, body, NULL, NULL);
      currTime = to_ms_since_boot(get_absolute_time());
    }
    // TODO: send request every 10 minutes
    // or whatever interval of time can be considered wise
    // to fetch data about the weather

    if (strlen(myBuff) > 1 && flag) {
      int curr_temp = get_temperature_from_json(myBuff);
      itoa(curr_temp, temperature, 10);
      flag = false;
    }
    /*get_time_from_json(myBuff, curr_time);*/
    get_curr_time(curr_time);
    Paint_DrawImage(arrs[i++], 0, 1, 160, 128);
    if (i == 26)
      i = 0;

    if (strlen(temperature) > 0) {
      Paint_DrawString_EN(0, 1, temperature, &Font12, RED, GREEN);
    }
    Paint_DrawString_EN(123, 1, curr_time, &Font12, RED, GREEN);
    LCD_1IN8_Display(BlackImage);
    /*DEV_Delay_ms(10);*/
  }
  free(BlackImage);
  BlackImage = NULL;
  DEV_Module_Exit();
  return 0;
}

int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth,
          const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw) {
  if (cyw43_arch_init() != 0) {
    printf("Cannot initialize\n");
    return 1;
  }
  cyw43_arch_poll();
  cyw43_arch_enable_sta_mode();
  if (hostname != NULL) {
    netif_set_hostname(netif_default, hostname);
  }

  if (cyw43_arch_wifi_connect_async(ssid, pass, auth)) {
    return 2;
  }

  int flashrate = 1000;
  int status = CYW43_LINK_UP + 1;
  while (status >= 0 && status != CYW43_LINK_UP) {
    int new_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if (new_status != status) {
      status = new_status;
      flashrate = flashrate / (status + 1);
      printf("connect status: %d %d\n", status, flashrate);
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(flashrate);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(flashrate);
  }
  if (status < 0) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  } else {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    if (ip != NULL) {
      netif_set_ipaddr(netif_default, ip);
    }
    if (mask != NULL) {
      netif_set_netmask(netif_default, mask);
    }
    if (gw != NULL) {
      netif_set_gw(netif_default, gw);
    }
    printf("IP: %s\n", ip4addr_ntoa(netif_ip_addr4(netif_default)));
    printf("Mask: %s\n", ip4addr_ntoa(netif_ip_netmask4(netif_default)));
    printf("Gateway: %s\n", ip4addr_ntoa(netif_ip_gw4(netif_default)));
    printf("Host Name: %s\n", netif_get_hostname(netif_default));
  }
  return status;
}
