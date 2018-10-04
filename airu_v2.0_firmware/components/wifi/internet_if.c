/*
* internet_if.c
* 
* Created: August 1, 2018
*  Author: Trenton Taylor
*
*/

#include "internet_if.h"

#include <string.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

//#include "lwip/err.h"
//#include "lwip/sys.h"
//#include "esp_system.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;




/*
* @brief
*
* @param
*
* @return
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch(event->event_id) 
  {
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(TAG, "got ip:%s",
               ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
      strcpy(ip_address, &event->event_info.got_ip.ip_info.ip);
      break;

    case SYSTEM_EVENT_AP_STACONNECTED:
      ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
              MAC2STR(event->event_info.sta_connected.mac),
               event->event_info.sta_connected.aid);
      break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
              MAC2STR(event->event_info.sta_disconnected.mac),
              event->event_info.sta_disconnected.aid);
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
      break;

    case SYSTEM_EVENT_AP_STOP:
      ESP_LOGI(TAG, "AP mode stopped.");
      xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
      wifi_init_sta();
      break;

    default:
        break;

    }//switch

    return ESP_OK;
}


/*
* @brief
*
* @param
*
* @return
*/
void wifi_init_sta()
{
  wifi_config_t wifi_config = 
  {
      .sta = 
      {
          .ssid = "airu",
          .password = "cleantheair"
      },
  };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             "airu", "cleantheair");
}


/*
* @brief
*
* @param
*
* @return
*/
void wifi_init_softap()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = 
    {
        .ap = 
        {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) 
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}


/*
* @brief
*
* @param
*
* @return
*/
void wifi_stop()
{
  esp_wifi_stop();
}





