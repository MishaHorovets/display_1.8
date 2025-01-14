// initialization of wifi module
// connects based on the passed country (WorldWide is prefferable)
int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth,
          const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);
