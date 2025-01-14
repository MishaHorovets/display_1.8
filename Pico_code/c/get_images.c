
#include <lwip/tcp.h>
#include <pico/stdio_usb.h>
#include <stddef.h>
#include <wchar.h>
#define STB_IMAGE_IMPLEMENTATION
#include "DEV_Config.h"
#include "DEV_config.h"
#include "GUI_Paint.h"
#include "LCD_1in8.h"
#include "anime/photo.c"
#include "cJSON.c"
#include "cJSON.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "lwip/apps/http_client.h"
#include "lwipopts.h"
#include "pico/cyw43_arch.h"
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void download_image(const char *url, void (*callback)(char *data, size_t len)) {
  struct httpc_connection settings = {0};
  settings.result_fn = result;        // Called when transfer completes
  settings.headers_done_fn = headers; // Called when headers are received

  err_t err = httpc_get_file_dns("http://localhost::3001/photo-hex", 80, url,
                                 &settings, body_image, NULL, NULL);
  if (err != ERR_OK) {
    printf("HTTP request failed with error: %d\n", err);
  }
}

err_t body_image(void *arg, struct altcp_pcb *conn, struct pbuf *p, err_t err) {
  if (p) {
    char image_data[1024]; // Buffer for image data
    pbuf_copy_partial(p, image_data, p->tot_len, 0);
    pbuf_free(p);

    // Process image data here, e.g., decode and render
    display_image(image_data, p->tot_len);
  }
  return ERR_OK;
}
void update_display() {
  // Fetch and display the next frame
  static int frame_index = 0;
  char url[100];
  snprintf(url, sizeof(url), "", frame_index);

  download_image(url, display_image);
  frame_index = (frame_index + 1) % NUM_FRAMES; // Loop through frames
}
