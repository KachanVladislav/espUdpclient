


#include "mynewcomponent.h"

// #define wifiScanPeriod 10000 // ms

// void wifi_scan(void *pvParameters) 
// {

// 	wifi_scan_config_t scanConf = {
//       .ssid = NULL,
//       .bssid = NULL,
//       .channel = 0,
//       .show_hidden = true
//    };

// 	while(true)
// 	{
		
//     	ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, true));    //The true parameter cause the function to block until
//                                                                  //the scan is done.
//     	vTaskDelay(wifiScanPeriod/ portTICK_PERIOD_MS);
//     }
// }