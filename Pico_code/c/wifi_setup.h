/*#ifndef MYBUFF_H*/
/*#define MYBUFF_H*/
/**/
/*// Declare myBuff as an external variable*/
/*extern char myBuff[1024]; // Adjust the size if needed*/
/**/
/*#endif // MYBUFF_H*/
#include "lwip/apps/http_client.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <stdio.h>

// initialization of wifi module
// connects based on the passed country (WorldWide is prefferable)
int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth,
          const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);
