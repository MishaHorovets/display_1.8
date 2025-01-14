#include "wifi_setup.h"

#include "lwip/apps/http_client.h"
#include "lwipopts.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <cyw43_country.h>
#include <hardware/rtc.h>
#include <lwip/tcp.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char ssid[64] = {0};
char pass[64] = {0};
const uint32_t country = CYW43_COUNTRY_WORLDWIDE;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;
char myBuff[2000];
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

void result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len,
            u32_t server_res, err_t err) {
  printf("%s\n", (char *)arg);
  printf("transfer complete!\n");
  printf("local result = %d\n", httpc_result);
  printf("http result = %d\n", server_res);
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
