/* Simple WiFi Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
/*
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
*/
#include "nvs_flash.h"
#include "esp_log.h"

#include "internet_if.h"
#include "pm_if.h"

/* Global constants */

/* Global vairables */


/* Function prototypes */




/*
* @brief
*
* @param
*
* @return
*
*/
void app_main()
{
  esp_log_level_set(TAG_PM, ESP_LOG_INFO);

  PM_init();
  

  //Create a task to handler UART event from ISR
  //xTaskCreate(vPM_task, "vPM_task", 2048, NULL, 12, NULL);


}






