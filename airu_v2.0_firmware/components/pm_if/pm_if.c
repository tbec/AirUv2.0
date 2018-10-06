/*
*	pm_if.c
*	
*	Last Modified: October 6, 2018
*	 Author: Trenton Taylor
*
*/

/*
*   To do:
*
* -  add a timer that clears the packet buffer every so often and maybe when you get data.
* -	 put lock around the buffer when clearing it in the interupt handler
* -  timer code may need to be in IRAM. NOTE: theres only 124K of IRAM.
*
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "pm_if.h"


/* Function prototypes */
esp_err_t PM_init();
esp_err_t PM_get_data();
esp_err_t PM_reset();
static void vPM_task(void *pvParameters);
static esp_err_t get_packet_from_buffer();
static esp_err_t get_data_from_packet(uint8_t *packet);
static bool check_sum(uint8_t *buf);



/*
* @brief
*
* @param
*
* @return
*
*/
esp_err_t PM_init()
{
  esp_err_t err = ESP_OK;
  esp_log_level_set(TAG_PM, ESP_LOG_INFO);

  // configure parameters of the UART driver
  uart_config_t uart_config = 
  {
    .baud_rate = 9600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };
  err = uart_param_config(PM_UART_CH, &uart_config);

  // set UART pins
  err = uart_set_pin(PM_UART_CH, PM_TXD_PIN, PM_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  // install UART driver
  err = uart_driver_install(PM_UART_CH, BUF_SIZE, 0, 20, &PM_event_queue, 0);

  // create a task to handler UART event from ISR for the PM sensor
  xTaskCreate(vPM_task, "vPM_task", 2048, NULL, 12, NULL);

  return err;
}


/*
* @brief
*
* @param
*
* @return
*
*/
esp_err_t PM_get_data()
{

	/* not done yet */

	get_packet_from_buffer();
	pm_data.sample_count++;

	return ESP_FAIL;
}

/*
* @brief
*
* @param
*
* @return
*
*/
esp_err_t PM_reset()
{

	return ESP_FAIL;
}


/*
* @brief
*
* @param
*
* @return
*
*/
static void vPM_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t buf[BUF_SIZE];
    uint16_t i_buf = 0;
    int i;


    for(;;) 
    {
        //Waiting for UART event.
        if(xQueueReceive(PM_event_queue, (void * )&event, (portTickType)portMAX_DELAY)) 
        {
            bzero(buf, BUF_SIZE);
            ESP_LOGI(TAG_PM, "uart[%d] event:", PM_UART_CH);
            switch(event.type) 
            {
                case UART_DATA:
                    printf("____UART_DATA____\n");
                    ESP_LOGI(TAG_PM, "[UART DATA]: %d", event.size);

                    if(event.size == 24) 
                    {
                      uart_read_bytes(PM_UART_CH, buf, event.size, portMAX_DELAY); 
                      memcpy(pm_buf + i_buf, buf, sizeof(uint8_t) * PM_PKT_LEN);
                      if(i_buf == BUF_SIZE)
                        i_buf = 0;
                      else
                        i_buf = (i_buf + PM_PKT_LEN); 
                    }

                    // get some data from the buffer
                    //get_packet_from_buffer();
                    PM_get_data();

                    printf("buffer: \n");
                    for(i = 0; i < BUF_SIZE; i++)
                    {
                      printf("%d", pm_buf[i]);
                    }
                    printf("\n");

                    printf("------------------\n");
                    printf("PM 1: %d\n", pm_data.pm1);
                    printf("PM 2.5: %d\n", pm_data.pm2_5);
                    printf("PM 10: %d\n", pm_data.pm10);
                    printf("------------------\n");
                    
                    ESP_LOGI(TAG_PM, "[DATA EVT]:");
                    uart_write_bytes(PM_UART_CH, (const char*) buf, event.size);
                    break;

                case UART_FIFO_OVF:
                printf("____UART_FIFO_OVF____\n");
                    ESP_LOGI(TAG_PM, "hw fifo overflow");
                    uart_flush_input(PM_UART_CH);
                    xQueueReset(PM_event_queue);
                
                case UART_BUFFER_FULL:
                    printf("____UART_BUFFER_FULL____\n");
                    ESP_LOGI(TAG_PM, "ring buffer full");
                    uart_flush_input(PM_UART_CH);
                    xQueueReset(PM_event_queue);
                    break;
            
                case UART_BREAK:
                    printf("____UART_BREAK____\n");
                    ESP_LOGI(TAG_PM, "uart rx break");
                    break;
                
                case UART_PARITY_ERR:
                    printf("____UART_PARITY_ERR____\n");
                    ESP_LOGI(TAG_PM, "uart parity error");
                    break;
                
                case UART_FRAME_ERR:
                    printf("____UART_FRAME_ERR____\n");
                    ESP_LOGI(TAG_PM, "uart frame error");
                    break;

                default:
                    ESP_LOGI(TAG_PM, "uart event type: %d", event.type);
                    break;
            }//case
        }//if
    }//for
    
    vTaskDelete(NULL);
}


/*
* @brief
*
* @param
*
* @return
*
*/
static esp_err_t get_packet_from_buffer()
{
  uint8_t packet[PM_PKT_LEN];
  uint16_t i;


  // Look for the first valid packet at the end of the buffer and if its not there
  // then start moving to the front of the buffer looking for one.
  for(i = BUF_SIZE - PM_PKT_LEN; i > 0; i--)
  {
  	if(pm_buf[i] == 'B' && pm_buf[i+1] == 'M')
  	{
  		memcpy(packet, pm_buf + i, sizeof(uint8_t) * PM_PKT_LEN);

  		if(check_sum(packet))
  		{
  			// Store the data from the valid packet found in the global pm struct.
  			//printf("Found valid packet at index: %d\n", i);
  			get_data_from_packet(packet);
  			pm_data.sample_count++;
  			return ESP_OK;
  		}
  	}
  }//for

  return ESP_FAIL;
}


/*
* @brief
*
* @param
*
* @return
*
*/
static esp_err_t get_data_from_packet(uint8_t *packet)
{
  uint16_t tmp;
  uint8_t tmp2;

    
  if(packet == NULL)
    return ESP_FAIL;

  // PM1 data
  tmp = packet[PKT_PM1_HIGH];
  tmp2 = packet[PKT_PM1_LOW];
  tmp = tmp << sizeof(uint8_t);
  tmp = tmp | tmp2;
  pm_data.pm1 = tmp;

  // PM2.5 data
  tmp = packet[PKT_PM2_5_HIGH];
  tmp2 = packet[PKT_PM2_5_LOW];
  tmp = tmp << sizeof(uint8_t);
  tmp = tmp | tmp2;
  pm_data.pm2_5 = tmp;

  // PM10 data
  tmp = packet[PKT_PM10_HIGH];
  tmp2 = packet[PKT_PM10_LOW];
  tmp = tmp << sizeof(uint8_t);
  tmp = tmp | tmp2;
  pm_data.pm10 = tmp;


  return ESP_OK;
}


/*
* @brief
*
* @param
*
* @return
*
*/
static bool check_sum(uint8_t *buf)
{
  uint16_t checksum;
  uint16_t sum;
  uint16_t i;

  if(buf[0] != 'B' && buf[1] != 'M')
  {
    return false;
  }

  checksum = ((uint16_t) buf[PM_PKT_LEN-2]) << 8;
  checksum += (uint16_t) buf[PM_PKT_LEN-1];

  sum = 0;
  for(i = 0; i < PM_PKT_LEN-2 ; i++)
  {
    sum += buf[i];
  }

  return (sum == checksum);
}


