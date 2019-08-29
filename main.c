// STM32F100 and SI4032 RTTY transmitter
// released under GPL v.2 by anonymous developer
// enjoy and have a nice day
// ver 1.5a
#include <stm32f10x_gpio.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_adc.h>
#include <stm32f10x_rcc.h>
#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <misc.h>
#include "f_rtty.h"
#include "init.h"
#include "config.h"
#include "radio.h"
#include "ublox.h"
#include "delay.h"
#include "aprs.h"
#include "locator.h"
#include "morse.h"
///////////////////////////// test mode /////////////
const unsigned char test = 0; // 0 - normal, 1 - short frame only cunter, height, flag
char callsign[15] = {RTTY_CALLSIGN};
char rtty_comment[25] = {RTTY_COMMENT};

#define GREEN  GPIO_Pin_7
#define RED  GPIO_Pin_8

unsigned int send_cun;        //frame counter
char status[2] = {'N'};

GPSEntry gpsData;
int8_t si4032_temperature;
uint16_t voltage;
volatile int adc_bottom = 2000;

unsigned int led_timeout = 600 * RTTY_SPEED; // Approx. 10 minutes
volatile uint8_t led_enabled = 1; // Flag to disable LEDs after a timeout

volatile char flaga = 0;
uint16_t CRC_rtty = 0x12ab;  //checksum
char buffer[200];
char locator[13];

volatile unsigned char pun = 0;
volatile unsigned int cun = 10;
volatile unsigned char tx_on = 0;
volatile unsigned int tx_on_delay;
volatile unsigned char tx_enable = 0;
rttyStates send_rtty_status = rttyZero;
volatile char *rtty_buf;
volatile uint16_t button_pressed = 0;
volatile uint8_t disable_armed = 0;

void collect_telemetry_data(void);
void send_rtty_packet(void);
uint16_t gps_CRC16_checksum (char *string);
void send_aprs_packet(void);
void send_morse_message(void);

char msg [32] = "----";
char rxbuf[32];
int rxbuf_pos = 0;	

/**
 * GPS data processing
 */
void USART1_IRQHandler(void) {
  if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
    ublox_handle_incoming_byte((uint8_t) USART_ReceiveData(USART1));
      }else if (USART_GetITStatus(USART1, USART_IT_ORE) != RESET) {
    USART_ReceiveData(USART1);
	USART_SendData(USART3, 69);	
  } else {
    USART_ReceiveData(USART1);
	USART_SendData(USART3, 70);	
  }
}

// Handler para tratamento da interrupcao na USART3
void USART3_IRQHandler()
{
  if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
  {	 
	  rxbuf[rxbuf_pos++] = USART_ReceiveData(USART3);		      		   
      if ((rxbuf[rxbuf_pos-1] == '\n') && rxbuf_pos != 0) 	 	       
         {				
           int tamanho = (strlen(rxbuf))-1;
           strncpy(msg, rxbuf,tamanho);
           rxbuf[0]=0;
           rxbuf_pos = 0;
           USART_SendData(USART3, '!');				
         }
  }
}

void TIM2_IRQHandler(void) {
  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    if (aprs_is_active()){
      aprs_timer_handler();
    } else {
      if (ALLOW_DISABLE_BY_BUTTON){
        if (ADCVal[1] > adc_bottom){
          button_pressed++;
          if (button_pressed > (RTTY_SPEED / 3)){
            disable_armed = 1;
            GPIO_SetBits(GPIOB, RED);
            GPIO_SetBits(GPIOB, GREEN);
          }
        } else {
          if (disable_armed){
            GPIO_SetBits(GPIOA, GPIO_Pin_12);
          }
          button_pressed = 0;
        }
    	if (button_pressed == 0) {
  	   adc_bottom = ADCVal[1] * 1.1;	// dynamical reference for power down level
		}
      }
      if (tx_on) {
        send_rtty_status = send_rtty((char *) rtty_buf);
        if (!disable_armed){
          if (send_rtty_status == rttyEnd) {
            if (led_enabled) GPIO_SetBits(GPIOB, RED);
            if (*(++rtty_buf) == 0) {
              tx_on = 0;
              tx_on_delay = TX_DELAY / (1000/RTTY_SPEED);
              tx_enable = 0;
              radio_disable_tx();
            }
          } else if (send_rtty_status == rttyOne) {
            radio_rw_register(0x73, RTTY_DEVIATION, 1);
            if (led_enabled) GPIO_SetBits(GPIOB, RED);
          } else if (send_rtty_status == rttyZero) {
            radio_rw_register(0x73, 0x00, 1);
            if (led_enabled) GPIO_ResetBits(GPIOB, RED);
          }
        }
      }
      if (!tx_on && --tx_on_delay == 0) {
        tx_enable = 1;
        tx_on_delay--;
      }
      if (--cun == 0) {
        if (pun) {
          if (led_enabled) GPIO_ResetBits(GPIOB, GREEN);
          pun = 0;
        } else {
          if (flaga & 0x80) {
            if (led_enabled) GPIO_SetBits(GPIOB, GREEN);
          }
          pun = 1;
        }
        cun = 200;
      }
    }

    if (!LED_ENABLED && led_enabled && !--led_timeout) led_enabled = 0;

  }

}

int main(void) {

	
#ifdef DEBUG
  debug();
#endif
  RCC_Conf();
  NVIC_Conf();
  init_port();

  init_timer(RTTY_SPEED);
  delay_init();
  ublox_init();
  

  if (led_enabled) GPIO_SetBits(GPIOB, RED);
  //USART_SendData(USART3, 0xc);

  radio_soft_reset();
  // setting RTTY TX frequency
  radio_set_tx_frequency(RTTY_FREQUENCY);

  // setting TX power
  radio_rw_register(0x6D, 00 | (TX_POWER & 0x0007), 1);

  // initial RTTY modulation
  radio_rw_register(0x71, 0x00, 1);

  // Temperature Value Offset
  radio_rw_register(0x13, 0xF0, 1);

  // Temperature Sensor Calibration
  radio_rw_register(0x12, 0x00, 1);

  // ADC configuration
  radio_rw_register(0x0f, 0x80, 1);
  rtty_buf = buffer;
  tx_on = 0;
  tx_enable = 1;

  aprs_init();
  uint8_t rtty_before_aprs_left = RTTY_TO_APRS_RATIO;
  uint8_t morse_countdown = RTTY_TO_MORSE_RATIO;

  while (1) {
    if (tx_on == 0 && tx_enable) {
      if (rtty_before_aprs_left){
        if (SEND_MORSE && !--morse_countdown) {
          send_morse_message();
          morse_countdown = RTTY_TO_MORSE_RATIO;
        }
        if (SEND_RTTY)
		   {
 			send_rtty_packet();
			USART_SendData(USART3, 'm');
		   }
        else _delay_ms(TX_DELAY);
        rtty_before_aprs_left--;
      } else {
        rtty_before_aprs_left = RTTY_TO_APRS_RATIO;
        if (SEND_APRS) send_aprs_packet();
      }

    } else {
      NVIC_SystemLPConfig(NVIC_LP_SEVONPEND, DISABLE);
      __WFI();
    }
  }
}

void collect_telemetry_data(void) {
  si4032_temperature = radio_read_temperature();
  voltage = ADCVal[0] * 600 / 4096;
  ublox_get_last_data(&gpsData);
  if (gpsData.fix >= 3) flaga |= 0x80;
  else flaga &= ~0x80;
  if (RTTY_WWL || SEND_MORSE_WWL)
    longlat2locator(gpsData.lon_raw, gpsData.lat_raw, locator);
}

void send_rtty_packet(void) {
  start_bits = RTTY_PRE_START_BITS;
  collect_telemetry_data();
  uint8_t lat_d = (uint8_t) abs(gpsData.lat_raw / 10000000);
  uint32_t lat_fl = (uint32_t) abs(abs(gpsData.lat_raw) - lat_d * 10000000) / 1000;
  uint8_t lon_d = (uint8_t) abs(gpsData.lon_raw / 10000000);
  uint32_t lon_fl = (uint32_t) abs(abs(gpsData.lon_raw) - lon_d * 10000000) / 1000;

  /*int packetLength = sprintf(buffer, "$$$$%s,%d", callsign, send_cun);
  if (SEND_RTTY_TIME)
    packetLength += sprintf(buffer + packetLength, ",%02u:%02u:%02u", gpsData.hours, gpsData.minutes, gpsData.seconds);
  if (SEND_RTTY_LATLON)
    packetLength += sprintf(buffer + packetLength, ",%s%u.%04lu,%s%u.%04lu",
      gpsData.lat_raw < 0 ? "-" : "", lat_d, lat_fl,
      gpsData.lon_raw < 0 ? "-" : "", lon_d, lon_fl);
  if (SEND_RTTY_HEIGHT)
    packetLength += sprintf(buffer + packetLength, ",%ld", (gpsData.alt_raw / 1000));
  if (SEND_RTTY_SPEED)
    packetLength += sprintf(buffer + packetLength, ",%ld", gpsData.speed_raw);
  if (SEND_RTTY_MESSAGE)
    packetLength += sprintf(buffer + packetLength, ",%s", RTTY_WWL ? locator : rtty_comment);
  if (SEND_RTTY_TEMPERATURE)
    packetLength += sprintf(buffer + packetLength, ",%d", si4032_temperature);
  if (SEND_RTTY_VOLTAGE)
    packetLength += sprintf(buffer + packetLength, ",%d.%02d", voltage/100, voltage-voltage/100*100);
  if (SEND_RTTY_SATELLITES)
    packetLength += sprintf(buffer + packetLength, ",%d", gpsData.sats_raw);
  if (SEND_RTTY_GPSDATA)
    packetLength += sprintf(buffer + packetLength, ",%d,%d,%02x",
      gpsData.ok_packets,
      gpsData.bad_packets,
      flaga);*/

  int packetLength = sprintf(buffer, "$$$$%s", msg); //transmite pacote com dados do sensor
  CRC_rtty = gps_CRC16_checksum(buffer + 4);
  sprintf(buffer + packetLength, "*%04X\n", CRC_rtty & 0xffff);
  rtty_buf = buffer;
  radio_enable_tx();
  tx_on = 1;

  send_cun++;
}

uint16_t gps_CRC16_checksum(char *string) {
  uint16_t crc = 0xffff;
  char i;
  while (*(string) != 0) {
    crc = crc ^ (*(string++) << 8);
    for (i = 0; i < 8; i++) {
      if (crc & 0x8000)
        crc = (uint16_t) ((crc << 1) ^ 0x1021);
      else
        crc <<= 1;
    }
  }
  return crc;
}

void send_aprs_packet(void) {
  radio_enable_tx();
  collect_telemetry_data();
  USART_Cmd(USART1, DISABLE);
  aprs_send_position(gpsData, si4032_temperature, voltage);
  USART_Cmd(USART1, ENABLE);
  radio_disable_tx();
}

void send_morse_message(void) {
  tx_enable = 0;
  collect_telemetry_data();
  int messageLength = sprintf(buffer, "%s", MORSE_PREFIX);
  if (SEND_MORSE_WWL)
    messageLength += sprintf(buffer + messageLength, " IN %s", locator);
  if (SEND_MORSE_HEIGHT)
    messageLength += sprintf(buffer + messageLength, " ASL %ld", (gpsData.alt_raw / 1000));
  if (SEND_MORSE_VOLTAGE)
    messageLength += sprintf(buffer + messageLength, " BAT %d.%02d", voltage/100, voltage-voltage/100*100);
  messageLength += sprintf(buffer + messageLength, "%s", MORSE_SUFFIX);

  // Set CW offset
  radio_rw_register(0x73, 1, 1);

  sendMorse(buffer);
  _delay_ms(2000);
  tx_enable = 1;
}


#ifdef  DEBUG
void assert_failed(uint8_t* file, uint32_t line)
{
    while (1);
}
#endif