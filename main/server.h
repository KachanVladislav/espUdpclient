#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
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
void wifi_scan(void *pvParameters);
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
void wifi_setup();
void http_server_netconn_serve(struct netconn *conn);
void http_server(void *pvParameters);
void IRAM_ATTR button_isr_handler(void* arg) ;