#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"

#include "tcpip_adapter.h"

#include "esp_event.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include <sys/param.h>

#include "tcpip_adapter.h"

#include "esp_http_server.h"
#include "esp_http_client.h"

#include "nvs_flash.h"


char *APWifiName;
char *APWifiPass;

EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_SCAN_ENABLE_BIT BIT1
#define WIFI_AUTO_CONNECT_BIT BIT2
#define WIFI_GOT_IP_BIT BIT3


void wifi_scan(void *pvParameters);
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
void wifi_setup();
void http_server_netconn_serve(struct netconn *conn);
void http_server(void *pvParameters);

void wifi_read_settings();
void wifi_auto_connect(void *pvParameters) ;