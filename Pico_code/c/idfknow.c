#include "pico/stdio_usb.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 120000 // Maximum size of the input string
#define PIXEL_COUNT 20480      // Number of pixels in the array

int main() {
  char input[MAX_BUFFER_SIZE];
  unsigned char pixels[PIXEL_COUNT];
  int pixel_index = 0;

  // Initialize USB serial communication
  stdio_init_all();

  printf("Waiting for file content...\n");

  while (true) {
    if (stdio_usb_connected()) {
      // Read incoming data
      pixel_index = 0;
      memset(input, 0, MAX_BUFFER_SIZE); // Clear input buffer
      int i = 0;

      // Read input until newline or buffer is full
      while (true) {
        int c = getchar_timeout_us(100000); // Non-blocking read
        if (c == PICO_ERROR_TIMEOUT) {
          continue; // No data received
        }
        if (c == '\n' || i >= MAX_BUFFER_SIZE - 1) {
          input[i] = '\0'; // Null-terminate the string
          break;
        }
        input[i++] = (char)c;
      }

      // Parse input and fill pixel array
      char *token = strtok(input, ",");
      while (token != NULL && pixel_index < PIXEL_COUNT) {
        pixels[pixel_index++] = (unsigned char)strtol(token, NULL, 16);
        token = strtok(NULL, ",");
      }

      // Print received pixel data (optional)
      printf("Received %d pixels:\n", pixel_index);
      for (int j = 0; j < pixel_index; j++) {
        printf("0x%02X ", pixels[j]);
        if ((j + 1) % 16 == 0)
          printf("\n");
      }
      printf("\n");
    } else {
      printf("USB not connected. Waiting...\n");
      sleep_ms(1000);
    }
  }

  return 0;
}
