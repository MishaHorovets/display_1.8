
/*char *host = "https://api.openweathermap.org/";*/
/*char *page = "data/2.5/"*/
/*             "weather?q=Netherlands&appid=1abe25c0ccddb9a35b2931cf9090a3b3&"*/
/*             "units=imperial";*/
void test(void) {
  while (true) {
    if (stdio_usb_connected()) {
      char buffer[128];
      int len = scanf("%127s", buffer);
      if (len > 0) {
        printf("You sent: %s\n", buffer);
      }
    }
  }
}
int get_index() {
  while (true) {
    if (stdio_usb_connected()) {
      char buffer[128];
      int len = scanf("%127s", buffer); // Read user input
      if (len > 0) {
        return atoi(buffer);
      }
    }
  }
}
