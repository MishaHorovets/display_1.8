#include "icons.c"
#include "tusb.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  const char *icon;
  unsigned char *img;
} IconMapping;

// just like switch case for choosing correct image
// based on the data fetched from api
IconMapping icon_map[] = {
    {"01d", sun},         {"01n", moon},       {"02d", cloudy_sun},
    {"02n", cloudy_moon}, {"03d", cloudy},     {"03n", cloudy},
    {"04d", cloudy},      {"04n", cloudy},     {"10d", rain_day},
    {"09d", rain_day},    {"10n", rain_night}, {"09n", rain_night},
    {"11d", thunder},     {"11n", thunder},    {"13d", snow},
    {"13n", snow},        {"50n", mist}};

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
void read_from_stdin(char *input, size_t input_size) {
  if (stdio_usb_connected()) {
    char buffer[1028];
    if (fgets(buffer, sizeof(buffer), stdin)) {
      buffer[strcspn(buffer, "\n")] = '\0';
      strcpy(input, buffer);
    }
  }
}
void get_icon_from_json(char *input_json, char *res) {
  if (input_json == NULL || strlen(input_json) == 0) {
    printf("Input json is null\n");
    return;
  }
  char *icon = strstr(input_json, "icon");
  if (icon == NULL) {
    printf("Icon not found\n");
    return;
  }
  icon += 7;
  int indx = 0;
  while (icon != NULL && *icon != '"') {
    /*printf("%c\n", *icon);*/
    res[indx++] = *icon;
    icon++;
  }
  /*printf("Extracted icon: %s\n", res);*/
}
void get_temp_from_json(char *input_json, char *res) {
  if (input_json == NULL || strlen(input_json) == 0) {
    printf("Input json is null\n");
    return;
  }
  char *temp = strstr(input_json, "temp");
  if (temp == NULL) {
    printf("Temperature not found\n");
    return;
  }
  temp += 6;
  int indx = 0;
  while (temp != NULL && *temp != ',') {
    /*printf("%c\n", *temp);*/
    res[indx++] = *temp;
    temp++;
  }
  double temperature = atof(res);
  /*printf("Extracted temperature: %f\n", temperature);*/
  int temp_int = round(temperature);
  sprintf(res, "%d", temp_int);
  /*printf("Extracted temperature (integer): %s\n", res);*/
}
