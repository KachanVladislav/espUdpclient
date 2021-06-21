#include "udpclient.h"
extern EventGroupHandle_t wifi_event_group;

#define HOST_IP_ADDR "192.168.100.9"
#define PORT 20001
// static const char *payload = b'gt';
static const char udp_cmd_get_target[] = {'g','t'};

void udp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) 
    {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) 
        {
            printf("Unable to create socket: errno %d", errno);
            break;
        }
        printf("Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);

        while (1) {

            int err = sendto(sock, udp_cmd_get_target, sizeof(udp_cmd_get_target), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err < 0) {
                printf( "Error occurred during sending: errno %d", errno);
                break;
            }
            printf("Message sent");

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                printf("recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                printf("Received %d bytes from %s:", len, host_ip);
                printf("%s", rx_buffer);
                if (strncmp(rx_buffer, "OK: ", 4) == 0) {
                    printf("Received expected message, reconnecting");
                    break;
                }
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            printf("Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}