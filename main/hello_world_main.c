
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "sdkconfig.h"

#include "driver/gpio.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "esp_event.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "lwip/err.h"
#include "lwip/sys.h"
//#include <sys/param.h>
#include "tcpip_adapter.h"

#include <esp_http_server.h>

#include "mynewcomponent.h"
#include "server.h"
#include "udpclient.h"





void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    wifi_setup();
  

    xTaskCreate(&wifi_auto_connect, "auto_connect", 8192, NULL, 5, NULL);
    xTaskCreate(&http_server, "http_server", 8192, NULL, 5, NULL);
	xTaskCreate(&wifi_scan, "wifi_scan", 8192, NULL, 5, NULL);
    xTaskCreate(&udp_client_task, "wifi_scan", 8192, NULL, 5, NULL);
    
    while(true){
        vTaskDelay(10);
    }

}