/*
*	internet_if.h
*	
*	Last Modified: August 1, 2018
*	 Author: Trenton Taylor
*
*/

#ifndef _INTERNET_IF_H
#define _INTERNET_IF_H


#include "esp_err.h"
#include "esp_event.h"


#define EXAMPLE_ESP_WIFI_MODE_AP   CONFIG_ESP_WIFI_MODE_AP //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

static const char *TAG = "simple wifi";

volatile char ip_address[15];



/*
* @brief *** Mention that the event_handler handles changes to the connection ***
*
* @param
*
* @return
*/
void wifi_init_sta();

/*
* @brief
*
* @param
*
* @return
*/
void wifi_init_softap();

/*
* @brief
*
* @param
*
* @return
*/
void wifi_stop();



#endif

