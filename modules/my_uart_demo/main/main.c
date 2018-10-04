/*
 * pm3003.c
 *
 *  Created on: August 24, 2018
 *      Author: Trenton Taylor <trentontaylor397@yahoo.com>
 */

/*
*	To do:
*
* - average multiple pm readings (modify the packet function and dont stop at just one packet)
* - add timeout when getting data from uart
*
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "PM";

#define PM_UART_CH   UART_NUM_2
#define PM_RXD_PIN   16
#define PM_TXD_PIN   17
#define BUF_SIZE     256 // NOTE: Rx_buffer_size should be greater than UART_FIFO_LEN
#define PM_PKT_LEN   24
#define MAX_NUM_PKT  5
#define TIMEOUT      50
#define PKT_PM1_HIGH 4
#define PKT_PM1_LOW  5
#define PKT_PM2_5_HIGH	6
#define PKT_PM2_5_LOW		7
#define PKT_PM10_HIGH		8
#define PKT_PM10_LOW		9
//#define PM_SET_PIN X
//#define PM_RESET_PIN X

/*
* @brief PM data struct
*
* Samples of three different sizes of particles will be recorded 
* represensted as PM1, PM2.5, PM10 in the documentaion.
* PM sensor data packets are defined as follows:
*
* PM Data is transmitted over UART in 24 byte packets. The first two 
* bytes are the packet header [0x42 0x4D] or [“BM”] in ASCII. Each piece 
* of the packet is 2 bytes, with the Most Significant Byte transmitted 
* first. The final two bytes are the packet checksum and represent a 16 
* bit (2 byte) number. This number should equal the sum of the first 22 
* bytes in the packet.
*
* Refer to PMS3003 documentation for more details.
*/
typedef struct 
{
  uint16_t pm1;             // Averaged PM1 samples
  uint16_t pm2_5;           // Averaged PM2.5 samples
  uint16_t pm10;            // Averaged PM10 samples
} pm_data_t;


/* Function Prototypes */
esp_err_t PM_init();
esp_err_t PM_get_data(pm_data_t *pm_data);
void vPM_task(void *pvParameters);
static bool check_sum_ok(uint8_t *buf);
static esp_err_t retrieve_data_from_packets(pm_data_t *pm_data, 
	uint8_t *buf, uint16_t buf_length);
static uint16_t read_uart(uint8_t *buf);



/*
* @brief
*
* @param
*
* @return
*
*/
void vPM_task(void *pvParameters)
{
  pm_data_t pm_data;
  uint8_t fail_count = 0;
  esp_err_t rv; 

  rv = PM_init();
  while(1)
  {
  	if(fail_count > 2)
  	{
  		// reset and reinitialize the PM sensor
      //PM_reset();
  		ESP_LOGI(TAG, "Reseting PM sensor due to errors.\n");
      fail_count = 0;
  	}

   	rv = PM_get_data(&pm_data);
   	if(rv == ESP_FAIL)
   		fail_count++;
    

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }//while 1
}


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
  esp_log_level_set(TAG, ESP_LOG_INFO);

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
  err = uart_driver_install(PM_UART_CH, BUF_SIZE, 0, 0, NULL, 0);

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
esp_err_t PM_get_data(pm_data_t *pm_data)
{
	int16_t read_len;
  uint8_t buf[BUF_SIZE];

  // flush uart buffer to get fresh data
  if(uart_flush(PM_UART_CH) == ESP_FAIL) 
    printf("Uart flush failed\n");

  //printf("Wait a sec to get new data\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // get data packets from UART
  read_len = read_uart(buf);
  if(read_len == 0)
  {
    //ESP_LOGI(TAG, "Read return 0\n");
  	return ESP_FAIL;
  }

	// get data we want out of the PM packets
  if(retrieve_data_from_packets(pm_data, buf, read_len) == ESP_FAIL)
  {
    //ESP_LOGI(TAG, "Retrieve packet error.\n");
  	return ESP_FAIL;
  }

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
static uint16_t read_uart(uint8_t *buf)
{
	size_t len = 0;
	int32_t read;

  if(uart_get_buffered_data_len(PM_UART_CH, &len) == ESP_FAIL)
  {
  	ESP_LOGI(TAG, "ERROR: Could not retrieve length of RX buffer.\n");
  	return 0;
  }

  if(len > 0)
  {
    read = uart_read_bytes(PM_UART_CH, buf, BUF_SIZE, 20 / portTICK_RATE_MS);
    if(read == -1 || read == 0)
    {
      ESP_LOGI(TAG, "ERROR: Could not read from UART channel %d.\n", PM_UART_CH);
      return 0;
    }

    return read;
  }

  return 0;
}


/*
* @brief
*
* @param
*
* @return
*
*/
static esp_err_t retrieve_data_from_packets(pm_data_t *pm_data, 
	uint8_t *buf, uint16_t buf_length)
{
	uint16_t i;
	uint16_t tmp;
	uint8_t tmp2;
	uint8_t packet[PM_PKT_LEN];
	
	// check parameters
	if(buf == NULL || buf_length < PM_PKT_LEN)
		return ESP_FAIL;

	// look for the start of a packet
  for(i = 0; i < buf_length; i++)
  {
    if(buf[i] == 'B' && buf[i+1] == 'M')
    {
      memcpy(packet, buf, sizeof(uint8_t) * PM_PKT_LEN);

      // check if packet data is valid
      if(check_sum_ok(packet))
      {
      	// PM1 data
      	tmp = packet[i + PKT_PM1_HIGH];
      	tmp2 = packet[i + PKT_PM1_LOW];
      	tmp = tmp << sizeof(uint8_t);
      	tmp = tmp | tmp2;
      	pm_data->pm1 = tmp;

      	// PM2 data
      	tmp = packet[i + PKT_PM2_5_HIGH];
      	tmp2 = packet[i + PKT_PM2_5_LOW];
      	tmp = tmp << sizeof(uint8_t);
      	tmp = tmp | tmp2;
      	pm_data->pm2_5 = tmp;

      	// PM10 data
      	tmp = packet[i + PKT_PM10_HIGH];
      	tmp2 = packet[i + PKT_PM10_LOW];
      	tmp = tmp << sizeof(uint8_t);
      	tmp = tmp | tmp2;
      	pm_data->pm10 = tmp;

      	printf("PM 1: %d\n", pm_data->pm1);
      	printf("PM 2.5: %d\n", pm_data->pm2_5);
      	printf("PM 10: %d\n", pm_data->pm10);
      	printf("------------------\n");

      	return ESP_OK;
      }//check_sum
    }//if B & M
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
static bool check_sum_ok(uint8_t *buf)
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
	esp_log_level_set(TAG, ESP_LOG_INFO);
  //PM_init(); maybe put in main? 

	xTaskCreate(vPM_task, "vPM_task", 2048, NULL, 12, NULL);
} 





