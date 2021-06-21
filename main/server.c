#include "server.h"

#define wifiScanPeriod 10000 // ms

#define EXAMPLE_ESP_WIFI_SSID      "TestSSID"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   2
#define EXAMPLE_MAX_STA_CONN       3


char *APWifiName;
char *APWifiPass;



uint16_t apCount = 0;
wifi_ap_record_t *list;
char htmlAnswer[1024] = "";

const static char http_html_hdr[] =
		"HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

// Build 404 header
const static char http_404_hdr[] =
"HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";

const static char http_wifiListStart[] =
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
		<title>Control</title><style>body{background-color:lightblue;font-size:24px;}</style></head>\
		<body><h1>Config</h1>\
		<label for='wifi'>Wifi:</label>\
		<select name='wifi' form = 'WifiSel'>";
const static char http_wifiListEnd[] =	
		"</select>\
		<br><br>\
		<form action='/wifisel' id = 'WifiSel'>\
	  	<label for='pass'>Pass:</label>\
	 	<input type='text' id='pass' name='pass'><br><br>\
	 	<input type='submit' value='Connect'>\
		</form></body></html>";	



void wifi_auto_connect(void *pvParameters) 
{
	while(true)	
	{	
		xEventGroupWaitBits(wifi_event_group, WIFI_AUTO_CONNECT_BIT, pdTRUE,pdTRUE, portMAX_DELAY);
		xEventGroupClearBits(wifi_event_group, WIFI_SCAN_ENABLE_BIT);
		esp_wifi_scan_stop();
		wifi_read_settings();
		wifi_config_t wifi_config = {
			.sta = {
				.ssid = "\0",
				.password = "\0",
			},
		};
		if(APWifiName != NULL)
		{
			strcpy((char*)wifi_config.sta.ssid,APWifiName);
		}
		if(APWifiPass != NULL)
		{
			strcpy((char*)wifi_config.sta.password,APWifiPass);
		}
		

		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
		esp_wifi_connect();
		vTaskDelay(10000/portTICK_PERIOD_MS);
	}
	
	vTaskDelete(NULL);
}

void wifi_read_settings()
{
	nvs_handle_t my_handle;
	esp_err_t ret;

    ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
        size_t req_size;
        ret = nvs_get_str(my_handle, "SSIDname", NULL, &req_size);
        switch (ret) {
            case ESP_OK:
                printf("Done\n");
				free(APWifiName);
				APWifiName = malloc(req_size);
				ret = nvs_get_str(my_handle, "SSIDname", APWifiName, &req_size);
				ret = nvs_get_str(my_handle, "SSIDPass", NULL, &req_size);
				if(ret == ESP_OK)
				{
					free(APWifiPass);
					APWifiPass = malloc(req_size);
					ret = nvs_get_str(my_handle, "SSIDPass", APWifiPass, &req_size);
				}
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
        }

       nvs_close(my_handle);
    }
}

void wifi_save_ssid(void* pvParametrs)
{
	nvs_handle_t my_handle;
	esp_err_t ret;
	ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    } else {
		ret = nvs_set_str(my_handle, "SSIDname", APWifiName);
		ret = nvs_set_str(my_handle, "SSIDPass", APWifiPass);
		ret = nvs_commit(my_handle);
	}
	vTaskDelete(NULL);
}

void wifi_scan(void *pvParameters) 
{

	wifi_scan_config_t scanConf = {
      .ssid = NULL,
      .bssid = NULL,
      .channel = 0,
      .show_hidden = true
   };

	while(true)
	{
		xEventGroupWaitBits(wifi_event_group, WIFI_SCAN_ENABLE_BIT, false, true, portMAX_DELAY);
    	ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, false));    //The true parameter cause the function to block until
                                                                 //the scan is done.
        vTaskDelay(wifiScanPeriod/ portTICK_PERIOD_MS);
    }
}

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
	EventBits_t wifi_group_status = xEventGroupGetBits(wifi_event_group);
	switch(event_id)
	{
		case SYSTEM_EVENT_SCAN_DONE:
			apCount = 0;
			esp_wifi_scan_get_ap_num(&apCount);
			printf("Number of access points found: %d\n",apCount);
			if (apCount == 0) {
				return;
			}
			free(list);
			list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
			ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));
		break;
		case SYSTEM_EVENT_STA_START:
			xEventGroupSetBits(wifi_event_group, WIFI_AUTO_CONNECT_BIT);
		break;
		case SYSTEM_EVENT_STA_GOT_IP:
			printf("got ip\n");
			vEventGroupSetBitsCallback(wifi_event_group, WIFI_GOT_IP_BIT);
		break;
		case SYSTEM_EVENT_STA_LOST_IP:
			printf("lost ip\n");
			vEventGroupClearBitsCallback(wifi_event_group, WIFI_GOT_IP_BIT);
		break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
			
			if((wifi_group_status & WIFI_CONNECTED_BIT )== 0)
			{
				xEventGroupSetBits(wifi_event_group, WIFI_AUTO_CONNECT_BIT|WIFI_SCAN_ENABLE_BIT);
				break;
			}
			xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);	
            wifi_config_t wifi_config = {
                .sta = {
                    .ssid = "\0",
                    .password = "\0",
                },
            };
            strcpy((char*)wifi_config.sta.ssid,APWifiName);
            strcpy((char*)wifi_config.sta.password,APWifiPass);
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
			xEventGroupClearBits(wifi_event_group, WIFI_SCAN_ENABLE_BIT);
			esp_wifi_scan_stop();
            esp_wifi_connect();
        break;
		case SYSTEM_EVENT_STA_CONNECTED:
			xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT|WIFI_SCAN_ENABLE_BIT);
			xEventGroupClearBits(wifi_event_group, WIFI_AUTO_CONNECT_BIT);
			xTaskCreate(&wifi_save_ssid, "save_ssid", 4096, NULL, 5, NULL);	
			printf("Connected\n");
		break;
	}
	
}


void wifi_setup()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
	esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

	wifi_event_group = xEventGroupCreate();
	xEventGroupClearBits(wifi_event_group,WIFI_GOT_IP_BIT| WIFI_SCAN_ENABLE_BIT|
											WIFI_CONNECTED_BIT|WIFI_AUTO_CONNECT_BIT);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}



void http_server_netconn_serve(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**) &buf, &buflen);

		/* Is this an HTTP GET command? (only check the first 5 chars, since
		 there are other formats for GET, and we're keeping it very simple )*/
		if (buflen >= 5 && strncmp("GET ",buf,4)==0) 
		{

			/*  sample:
			 * 	GET /l HTTP/1.1
				Accept: text/html, application/xhtml+xml, image/jxr,
				Referer: http://192.168.1.222/h
				Accept-Language: en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3
				User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.79 Safari/537.36 Edge/14.14393
				Accept-Encoding: gzip, deflate
				Host: 192.168.1.222
				Connection: Keep-Alive
			 *
			 */
			//Parse URL
			char* path = NULL;
			char* line_end = strchr(buf, '\n');
			if( line_end != NULL )
			{
				//Extract the path from HTTP GET request
				path = (char*)malloc(sizeof(char)*(line_end-buf+1));
				int path_length = line_end - buf - strlen("GET ")-strlen("HTTP/1.1")-2;
				strncpy(path, &buf[4], path_length );
				path[path_length] = '\0';
				//Get remote IP address
				ip_addr_t remote_ip;
				u16_t remote_port;
				netconn_getaddr(conn, &remote_ip, &remote_port, 0);
				printf("[ "IPSTR" ] GET %s\n", IP2STR(&(remote_ip.u_addr.ip4)),path);

			}

			/* Send the HTML header
			 * subtract 1 from the size, since we dont send the \0 in the string
			 * NETCONN_NOCOPY: our data is const static, so no need to copy it
			 */
			bool bNotFound = false;
			if(path != NULL)
			{
				if(strncmp("/wifisel?", path, strlen("/wifisel?")) == 0)
				{
					// path - /wifisel?wifi=ZTE_2.4G_kUKE3d&pass=Pass
					free(APWifiName);
					free(APWifiPass);
					
					int APWifiNamelength = strchr(path, '&') - path - strlen(" /wifisel?wifi=") + 2;
					printf("1= %d, %p, %p, %d",APWifiNamelength,strchr(path, '&'), path,strlen(" /wifisel?wifi=") );
					APWifiName = (char*)malloc(sizeof(char*) * APWifiNamelength);
					strncpy(APWifiName, &path[strlen("/wifisel?wifi=")], APWifiNamelength - 1);
					APWifiName[APWifiNamelength -1] = '\0';
					
					int APWifiPassLength = strlen(path) - (strchr(path, '&') - path) - strlen("pass=");
					APWifiPass = (char*)malloc(sizeof(char*)*APWifiPassLength);
					strncpy(APWifiPass, strchr(path, '&') + strlen("pass=") + 1, APWifiPassLength -1);
					APWifiPass[APWifiPassLength -1] = '\0';

					// wifi_config_t wifi_config;
					
                    xEventGroupClearBits(wifi_event_group, WIFI_SCAN_ENABLE_BIT);
					esp_wifi_scan_stop();
					
                    wifi_ap_record_t ap_info;
                    if(esp_wifi_sta_get_ap_info(&ap_info) == ESP_ERR_WIFI_NOT_CONNECT)
                    {
                        // esp_wifi_disconnect();
                        wifi_config_t wifi_config = {
                            .sta = {
                                .ssid = "\0",
                                .password = "\0",
                            },
                        };
                        strcpy((char*)wifi_config.sta.ssid,APWifiName);
                        strcpy((char*)wifi_config.sta.password,APWifiPass);
                        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
                        esp_wifi_connect();
                    }
                    else
                    {
                        esp_wifi_disconnect();
                    }
				}
				free(path);
				path=NULL;
			}

			//Send HTTP response header
			htmlAnswer[0] = '\0';

			if(bNotFound)
			{
				netconn_write(conn, http_404_hdr, sizeof(http_404_hdr) - 1,
					NETCONN_NOCOPY);
				netconn_close(conn);
				netbuf_delete(inbuf);
				return;
			}
			else
			{
				strcpy(htmlAnswer, http_html_hdr);
			}

			// Send HTML content
			strcat(htmlAnswer, http_wifiListStart);
			for (int i=0; i<apCount; i++) 
			{
				strcat(htmlAnswer, "<option>");
				strcat(htmlAnswer, (char*)list[i].ssid);
				strcat(htmlAnswer, "</option>");
			}
 			strcat(htmlAnswer, http_wifiListEnd);
			netconn_write(conn, htmlAnswer, strlen(htmlAnswer) - 1,
					NETCONN_COPY);
		}
	}
	// Close the connection (server closes in HTTP)
	netconn_close(conn);

	// Delete the buffer (netconn_recv gives us ownership,
	// so we have to make sure to deallocate the buffer)
	netbuf_delete(inbuf);
}

void http_server(void *pvParameters) {
	struct netconn *conn, *newconn;	//conn is listening thread, newconn is new thread for each client
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while (err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}
