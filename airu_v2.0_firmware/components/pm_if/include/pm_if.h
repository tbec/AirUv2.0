/*
*	pm_if.h
*	
*	Last Modified: October 6, 2018
*	 Author: Trenton Taylor
*
*/


#ifndef _PM_IF_H
#define _PM_IF_H

#include "freertos/queue.h"
#include "esp_err.h"

static const char *TAG_PM = "PM";

#define PM_UART_CH   UART_NUM_2
#define PM_RXD_PIN   16
#define PM_TXD_PIN   17
#define BUF_SIZE     144 // NOTE: Rx_buffer_size should be greater than UART_FIFO_LEN (128 bytes)
#define PM_PKT_LEN   24
#define MAX_PKTS_IN_BUFFER 6
#define MAX_NUM_PKT  5
#define TIMEOUT      50
#define PKT_PM1_HIGH 4
#define PKT_PM1_LOW  5
#define PKT_PM2_5_HIGH  6
#define PKT_PM2_5_LOW   7
#define PKT_PM10_HIGH   8
#define PKT_PM10_LOW    9
//#define PM_SET_PIN    X
//#define PM_RESET_PIN  X


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
  uint32_t sample_count;    // Number of valid data points recieved
  uint16_t pm1;             // Most recent PM1 samples
  uint16_t pm2_5;           // Most recent PM2.5 samples 
  uint16_t pm10;            // Most recent PM10 samples
} pm_data_t;


/* Global variables */
static QueueHandle_t PM_event_queue;
pm_data_t pm_data;
uint8_t pm_buf[BUF_SIZE];

/*
* @brief
*
* @param
*
* @return
*
*/
esp_err_t PM_init();

/*
* @brief
*
* @param
*
* @return
*
*/
esp_err_t PM_get_data();

/*
* @brief
*
* @param
*
* @return
*
*/
esp_err_t PM_reset();



#endif

