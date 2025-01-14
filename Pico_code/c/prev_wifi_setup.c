#include "hardware/uart.h"
#include "lwip/apps/http_client.h"
#include "lwipopts.h"
#include "pico/cyw43_arch.h"
#include <cyw43_country.h>
#include <hardware/rtc.h>
#include <lwip/pbuf.h>
#include <lwip/tcp.h>
int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth,
          const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);
char myBuff[2000];
char weatherBuffer[2000];
char timeBuffer[10000];
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

    // Just to debug
    /*printf("IP: %s\n", ip4addr_ntoa(netif_ip_addr4(netif_default)));*/
    /*printf("Mask: %s\n", ip4addr_ntoa(netif_ip_netmask4(netif_default)));*/
    /*printf("Gateway: %s\n", ip4addr_ntoa(netif_ip_gw4(netif_default)));*/
    /*printf("Host Name: %s\n", netif_get_hostname(netif_default));*/
  }
  return status;
}
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
  printf("headers recieved\n");
  printf("content length=%d\n", content_len);
  printf("header length %d\n", hdr_len);

  if (hdr->tot_len > sizeof(myBuff) - 1) {
    printf("Received data exceeds buffer size, myBuff\n");
    return ERR_BUF; // Indicate a buffer error
  }
  pbuf_copy_partial(hdr, myBuff, hdr->tot_len, 0);
  printf("headers \n");
  printf("%s", myBuff);
  pbuf_free(hdr); // Free the pbuf
  return ERR_OK;
}

err_t body(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
  printf("body\n");

  if (p->tot_len > sizeof(timeBuffer) - 1) {
    printf("Received data exceeds buffer size\n");
    return ERR_BUF; // Indicate a buffer error
  }
  if (p != NULL) {
    pbuf_copy_partial(p, timeBuffer, p->tot_len, 0);
    printf("%s", timeBuffer);
    pbuf_free(p); // Free the pbuf
  }
  return ERR_OK;
}

void get_time_from_json(const char *json, char *time) {
  char *after_time_24 = strstr(json, "time_24");
  if (after_time_24 == NULL) {
    printf("Error finding 'time_24' field in JSON\n");
    return;
  }
  after_time_24 += 10;
  int indx = 0, written = 0;
  while (written < 5) {
    written++;
    time[indx++] = *after_time_24;
    after_time_24++;
  }
  time[indx] = '\0';
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
