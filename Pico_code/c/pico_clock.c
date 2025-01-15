#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>
// helps to sync time
void set_time(struct tm *time_info) {
  struct timeval tv = {.tv_sec = mktime(time_info), .tv_usec = 0};
  settimeofday(&tv, NULL);
}
void get_time_from_json(char *input_json) {
  struct tm time_info;
  if (input_json == NULL || strlen(input_json) == 0) {
    printf("Input json is null\n");
    return;
  }
  char *datetime_str = strstr(input_json, "\"date_time\":\"");
  if (datetime_str == NULL) {
    printf("Datetime not found in response\n");
    return;
  }
  datetime_str += 13; // Length of "\"date_time\":\""
  // extract the date and time string (format: "2024-11-07 07:07:35")
  char datetime_str_value[20];
  strncpy(datetime_str_value, datetime_str, 19);
  datetime_str_value[19] = '\0'; // Null-termination
  sscanf(datetime_str_value, "%4d-%2d-%2d %2d:%2d:%2d", &time_info.tm_year,
         &time_info.tm_mon, &time_info.tm_mday, &time_info.tm_hour,
         &time_info.tm_min, &time_info.tm_sec);
  time_info.tm_year -= 1900;
  time_info.tm_mon -= 1;
  set_time(&time_info);

  // just to debug
  printf("Time set to: %d-%d-%d %d:%d:%d\n", time_info.tm_year + 1900,
         time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour,
         time_info.tm_min, time_info.tm_sec);
}
