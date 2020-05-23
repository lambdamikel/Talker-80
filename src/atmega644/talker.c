/* 
   Talker/80 - New Dimensions in Voice Synthesis for the TRS-80 Model 1
   Copyright (C) 2020 Michael Wessel 

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

   Talker/80
   Copyright (C) 2020 Michael Wessel
   LambdaSpeak comes with ABSOLUTELY NO WARRANTY. 
   This is free software, and you are welcome to redistribute it
   under certain conditions. 

*/ 

//
// Talker/80 
// Version 1  
// License: GPL 3 
// 
// (C) 2019 Michael Wessel 
// mailto:miacwess@gmail.com
// https://www.michael-wessel.info
// 

/*
IMPORTANT LICENSE INFORMATION: 
Talker/80 uses GPL 3. 
This code uses the Epson S1V30120 firmware image from the TextToSpeech 
click board library from MikroElektronika released under GPL2: 
https://github.com/MikroElektronika/Click_TextToSpeech_S1V30120/blob/master/library/include/text_to_speech_img.h

By using this code, you are also bound to the Epson license terms for the S1V30120 firmware:
https://global.epson.com/products_and_drivers/semicon/products/speech/voice/sla/s1v30120_init_data.html
*/ 

#include <avr/pgmspace.h>
#include <avr/io.h>

#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <avr/wdt.h>
#include <string.h>

#include "S1V30120_defines.h" 
#include "text_to_speech_img.h"

//
//
//

#define VERSION 1
#define BOOTMESSAGE

#define SINGLE_PHONEME_MARKER 255 

//
//
//

#define COUNTER_SPEAK_DELAY 15

//
//
// 

#include "talker-hardware.h"
// #include "HAL9000_defines.h"    
#include "HAL9000_defines_full.h"    

//
// TRS80 Databus 
// 

static volatile uint8_t DATABUS = 0; 
static volatile uint8_t CONTROL_AVAILABLE = 0; 
static volatile uint8_t BUSY = 0; 

//
// Speech Buffer
// 
 
static char command_string[188];  // longest string in HAL quotes and copyright messages

// for all communication to Epson chip

#define SEND_BUFFER_SIZE 264  // 256 + 8 extra bytes for headers

// for Input Buffering from TRS80: 

#define SPEECH_BUFFER_SIZE 256 

//
// Timers 
// 

static volatile uint8_t TIMER_COUNTER = 0; 
static volatile uint8_t PHONEME_COUNTER = 0; 
static volatile uint8_t BLOCKING_SPEECH = 0; 

//
//
// 

#define configure_timer  TCCR0A = 0; TCCR0B = 0; setBit(TCCR0A, WGM01); OCR0A = 0xF9; setBit(TIMSK0, OCIE0A)
#define start_timer      TIMER_COUNTER = 0; setBit(TCCR0B, CS02); setBit(TCCR0B, CS00)
#define stop_timer       TCCR0B &= ~(1 << CS02); TCCR0B &= ~(1 << CS00) 

//
// Buffers 
// 

static volatile uint8_t STOP_NOW = 0; 
static volatile uint8_t TRS_COUNTER_BASED_SPEAKING = 0; 

static volatile uint8_t window_open = 0; 
static volatile uint8_t speak_until = 0; 
static volatile uint8_t last_speak_until = 0; 
static volatile uint8_t write_cur = 0; 
static volatile uint8_t read_cur = 0; 
static volatile uint8_t log_in = 0; 
static volatile uint8_t ctrl = 0; 
static volatile uint8_t nibble1 = 0; 
static volatile uint8_t nibble2 = 0; 

static volatile uint8_t buffer[SPEECH_BUFFER_SIZE] = { 0 }; 

static char rcvd_msg[20] = { 0 };
static char send_msg[SEND_BUFFER_SIZE] = { 0 };

//
// Flow Control 
// 

#define WAIT_UNTIL_FINISHED { while ( ! tts_parse_response(ISC_TTS_FINISHED_IND, 0x0000, 16) ) { if ( STOP_NOW) break; } }
 
//
// Utilities 
// 

#define BV(bit) (1 << (bit))
#define toggleBit(byte, bit)  (byte ^= BV(bit))
#define setBit(byte, bit) (byte |= BV(bit)) 
#define clearBit(byte, bit) (byte &= ~BV(bit))

//
//
//

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C // 00001100  

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

//
//
// 

static volatile unsigned short TTS_DATA_IDX;

static volatile unsigned short msg_index;

static volatile unsigned short msg_len;
static volatile unsigned short txt_len;


// 
//
// 

#define LEDS_OFF       { clearBit(LED_PORT, LED_DATA); clearBit(LED_PORT, LED_MODE0); clearBit(LED_PORT, LED_MODE1);  }

#define LEDS_ON        { setBit(LED_PORT, LED_DATA); setBit(LED_PORT, LED_MODE0); setBit(LED_PORT, LED_MODE1);  }

#define LEDS_SHOW_MODE { LED_PORT = (CUR_MODE << (LED_MODE0)) |  0b01000011 ;  } // make sure PULLUPS are not getting diabled!!

#define LED_DATA_ON      setBit(LED_PORT, LED_DATA) 
#define LED_DATA_OFF   clearBit(LED_PORT, LED_DATA) 

#define LED_BLOCKING_ON      setBit(LED_PORT, LED_BLOCKING) 
#define LED_BLOCKING_OFF   clearBit(LED_PORT, LED_BLOCKING) 

#define LED_SPEAKING_ON      setBit(LED_PORT, LED_SPEAKING) 
#define LED_SPEAKING_OFF   clearBit(LED_PORT, LED_SPEAKING) 

#define LED_SINGLE_PHONEME_SPEAKING_ON      setBit(LED_PORT, LED_SINGLE_PHONEME) 
#define LED_SINGLE_PHONEME_SPEAKING_OFF   clearBit(LED_PORT, LED_SINGLE_PHONEME)

//
//
//

typedef enum { EPSON_M = 0, TRS_M = 1, VS100_M = 2, DECTALK_M = 3 } SYNTH_MODE; 

static volatile SYNTH_MODE CUR_MODE = EPSON_M; 
static volatile SYNTH_MODE LAST_MODE = EPSON_M;  

// 
//
//

#define VOLUME_DEFAULT 12
#define SPEAK_RATE_DEFAULT 11

static volatile uint8_t VOICE = ISC_TTS_VOICE + 1; 
static volatile uint8_t BASE_PITCH = 10; 
static volatile uint8_t SPEAK_RATE = SPEAK_RATE_DEFAULT; 
static volatile uint8_t LANGUAGE = ISC_TTS_LANGUAGE;  
static volatile uint8_t VOLUME = VOLUME_DEFAULT; 
static volatile uint8_t CONFIRM_COMMANDS = 1; 
static volatile uint8_t ALTERNATE = 0; 
static volatile uint8_t DISABLED = 0; 

//
//
// 

static char*   sc01_map[0x40]; 
static uint8_t sc01_len[0x40]; 

static uint8_t trs_to_sc01_map[0xFF]; 

//
//
// 

inline static void setBitOrder(uint8_t bitOrder) {
  if (bitOrder == LSBFIRST) SPCR |= _BV(DORD);
  else SPCR &= ~(_BV(DORD));
}

inline static void setDataMode(uint8_t dataMode) {
  SPCR = (SPCR & ~SPI_MODE_MASK) | dataMode;
}

inline static void setClockDivider(uint8_t clockDiv) {
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (clockDiv & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((clockDiv >> 2) & SPI_2XCLOCK_MASK);
}

//
//
//

#define soft_reset() do { wdt_enable(WDTO_15MS); for(;;) {}} while(0) 

void process_reset(void) {
  soft_reset(); 
}

ISR(SOFT_RESET_INT_VEC) { // reset handler PC6

  if (bit_is_clear(SOFT_RESET_PIN, SOFT_RESET_PIN_NUMBER))
    process_reset();  
  
}
  
//
//
// 

void init_reset_handler(void) {

  setBit(PCICR, PCIE2); // change interrupts on 
  setBit(PCMSK2, SOFT_RESET_INT); // RESET PIN = PC6 = D22 

}

void wdt_init(void)
{

  cli();

  wdt_reset();
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = 0x00;

  sei();
   
  return;

}  

//
// SPI Epson TTS 
// 

void spi_begin_epson(void) { 

  setBit(TTS_OUTPUT, S1V30120_CS); // S1V30120 not selected

  // make the MOSI, SCK, and CS pins outputs
  TTS_DDR |= ( 1 << MOSI ) | ( 1 << SCK ) | ( 1 << CS );
 
  // make sure the MISO pin is input
  TTS_DDR &= ~( 1 << MISO );
  // pullup on MISO? 
  SPI_PORT |= (1 << MISO);

  setBitOrder(MSBFIRST); 
  setDataMode(SPI_MODE3);
  setClockDivider(SPI_CLOCK_DIV2); 
  //setClockDivider(SPI_CLOCK_DIV8); 
  //setClockDivider(SPI_CLOCK_DIV32); 

  SPCR |= ( 1 << SPE ) | ( 1 << MSTR ); 
  
} 

//Function to send and receive data for both master and slave
unsigned char spi_transceive_epson (unsigned char data) {
  // Load data into the buffer
  SPDR = data;
 
  //Wait until transmission complete
  while(!(SPSR & (1<<SPIF) ));

  _delay_us(3); 
 
  // Return received data
  return(SPDR);
}

void spi_transfer_epson(unsigned char data) {

  spi_transceive_epson(data); 

}


//
//
// 

void blink_led_fast(int n) {

  for (int i = 0; i < n; i++) {
     LEDS_ON; 
    _delay_ms(10); 
     LEDS_OFF; 
    _delay_ms(10);             
  }

  _delay_ms(50); 

}


void blink_led_slow(int n) {

  for (int i = 0; i < n; i++) {
     LEDS_ON; 
    _delay_ms(200); 
     LEDS_OFF; 
    _delay_ms(200);             
  }

  _delay_ms(1000); 

}

void blink_leds(void) {
  blink_led_fast(10); 
}


void check_for_error(int response)
{
  if (response) 
    return; 
  else
    {
      blink_led_slow(10); 
      process_reset();      
    }
  
}

//
//
// 


void tts_reset(void)
{

  setBit(TTS_OUTPUT, S1V30120_CS); // S1V30120 not selected
  clearBit(TTS_OUTPUT, S1V30120_RST);

  // send one dummy byte, this will leave the clock line high
  // SPI.beginTransaction(SPISettings(750000, MSBFIRST, SPI_MODE3));

  spi_transfer_epson(0x00);

  _delay_ms(5); 

  setBit(TTS_OUTPUT, S1V30120_RST);

  _delay_ms(150);

}

void tts_send_message(char message[], unsigned char message_length) {
  // Check to see if there's an incoming response or indication

  while( bit_is_set(TTS_INPUT, S1V30120_RDY)  );  // blocking
  // OK, we can proceed
  
  clearBit(TTS_OUTPUT, S1V30120_CS);

  spi_transfer_epson(0xAA);  // Start Message Command
  for (int i = 0; i < message_length; i++) {
    spi_transfer_epson(message[i]); 
  }  

}

uint16_t convert_speak_rate(uint8_t rate) {  
  return 15 + 25 * rate; 
} 

uint8_t convert_volume(uint8_t vol) {  
  return 7 + 4 * vol; 
} 

// Padding function
// Sends a num_padding_bytes over the SPI bus
void tts_send_padding(unsigned short num_padding_bytes)
{
  for (int i = 0; i < num_padding_bytes; i++)
    {
      spi_transfer_epson(0x00);
    }  
}

unsigned short tts_get_version(void)
{
  // Query version
  unsigned short S1V30120_version = 0;

  // Sending ISC_VERSION_REQ = [0x00, 0x04, 0x00, 0x05];
  char msg_ver[] = {0x04, 0x00, 0x05, 0x00};

  tts_send_message(msg_ver, 0x04);    
  //wait for ready signal

  while( bit_is_clear(TTS_INPUT, S1V30120_RDY));
    
  // receive 20 bytes
  clearBit(TTS_OUTPUT, S1V30120_CS); 

  // wait for message start
  while(spi_transceive_epson(0x00) != 0xAA);
  for (int i = 0; i < 20; i++)
    {
      rcvd_msg[i] = spi_transceive_epson(0x00);
    }
  // Send 16 bytes padding

  tts_send_padding(16);  

  setBit(TTS_OUTPUT, S1V30120_CS);

  S1V30120_version = rcvd_msg[4] << 8 | rcvd_msg[5];

  return S1V30120_version;

}


int tts_parse_response(unsigned short expected_message, unsigned short expected_result, unsigned short padding_bytes)
{
  unsigned short rcvd_tmp; 

  //wait for ready signal
  while( bit_is_clear(TTS_INPUT, S1V30120_RDY) ) {
    if (STOP_NOW) 
      return 1; 
  }

  // receive 6 bytes
  clearBit(TTS_OUTPUT, S1V30120_CS);

  // wait for message start
  // check for stop!
  while ( spi_transceive_epson(0x00) != 0xAA ) {
    if (STOP_NOW) 
      return 1; 
  }

  for (int i = 0; i < 6; i++) {
    rcvd_msg[i] = spi_transceive_epson(0x00);
  }

  tts_send_padding(padding_bytes); 

  setBit(TTS_OUTPUT, S1V30120_CS);

  // Are we successfull? We shall check 
  rcvd_tmp = rcvd_msg[3] << 8 | rcvd_msg[2];
  if (rcvd_tmp == expected_message) // Have we received ISC_BOOT_RUN_RESP?
    { 
      // We check the response
      rcvd_tmp = rcvd_msg[5] << 8 | rcvd_msg[4];
      if (rcvd_tmp == expected_result) // success, return 1
	return 1;
      else
	return 0;
    }
  else // We received something else
    return 0;

}


int tts_configure_audio(void) {

  msg_len = 0x0C;

  send_msg[0] = msg_len & 0xFF;          // LSB of msg len
  send_msg[1] = (msg_len & 0xFF00) >> 8; // MSB of msg len
  send_msg[2] = ISC_AUDIO_CONFIG_REQ & 0xFF;
  send_msg[3] = (ISC_AUDIO_CONFIG_REQ & 0xFF00) >> 8;
  send_msg[4] = TTS_AUDIO_CONF_AS;

  send_msg[5] = convert_volume(VOLUME); 
  // send_msg[5] = TTS_AUDIO_CONF_AG; 

  send_msg[6] = TTS_AUDIO_CONF_AMP;
  send_msg[7] = TTS_AUDIO_CONF_ASR;
  send_msg[8] = TTS_AUDIO_CONF_AR;
  send_msg[9] = TTS_AUDIO_CONF_ATC;
  send_msg[10] = TTS_AUDIO_CONF_ACS;
  send_msg[11] = TTS_AUDIO_CONF_DC;

  tts_send_message(send_msg, msg_len); 

  return tts_parse_response(ISC_AUDIO_CONFIG_RESP, 0x0000, 16); 

}

int tts_registration(void)
{

  char reg_code[] = {0x0C, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  tts_send_message(reg_code, 0x0C);

  return tts_parse_response(ISC_TEST_RESP, 0x0000, 16);

}    

int tts_load_chunk(unsigned short chunk_len)
{
  // Load a chunk of data
  char len_msb = ((chunk_len + 4) & 0xFF00) >> 8;
  char len_lsb = (chunk_len + 4) & 0xFF;

  clearBit(TTS_OUTPUT, S1V30120_CS);

  spi_transfer_epson(0xAA);  // Start Message Command
  spi_transfer_epson(len_lsb);  // Message length is 2048 bytes = 0x0800
  spi_transfer_epson(len_msb);  // LSB first
  spi_transfer_epson(0x00);  // Send SC_BOOT_LOAD_REQ (0x1000)
  spi_transfer_epson(0x10);
  for (int chunk_idx = 0; chunk_idx < chunk_len; chunk_idx++)
    {
      //spi_transfer_epson(TTS_INIT_DATA[TTS_DATA_IDX]);
      unsigned char x = pgm_read_byte(&(TTS_INIT_DATA[TTS_DATA_IDX])); 
      spi_transfer_epson(x); 

      TTS_DATA_IDX++;
    }   

  setBit(TTS_OUTPUT, S1V30120_CS);

  return tts_parse_response(ISC_BOOT_LOAD_RESP, 0x0001, 16);

}

int tts_download(void)
{
  // TTS_INIT_DATA is of unsigned char type (one byte)
  unsigned short len = sizeof (TTS_INIT_DATA);

  unsigned short fullchunks;
  unsigned short remaining;

  int chunk_result;

  // We are loading chunks of data
  // Each chunk, including header must be of maximum 2048 bytes
  // as the header is 4 bytes, this leaves 2044 bytes to load each time
  // Computing number of chunks
  fullchunks = len / 2044;
  remaining = len - fullchunks * 2044;

  // Load a chunk of data
  for (int num_chunks = 0; num_chunks < fullchunks; num_chunks++)
    {
      chunk_result = tts_load_chunk (2044);
      if (chunk_result)
	{
       
	}  
      else
	{
	  return 0;
	}
    }
  // Now load the last chunk of data  
  chunk_result = tts_load_chunk (remaining);
  if (chunk_result)
    {
     
    }  
  else
    {
      return 0;
    }
  // All was OK, returning 1
  return 1;   
}

int tts_boot_run(void)
{
  char boot_run_msg[] = {0x04, 0x00, 0x02, 0x10};
  tts_send_message(boot_run_msg, 0x04);
  return tts_parse_response(ISC_BOOT_RUN_RESP, 0x0001, 8);
}

int tts_speech(char* text_to_speech, unsigned char flush_enable)
{
  int response;

  txt_len = strlen(text_to_speech); 
  msg_len = txt_len + 6;

  send_msg[0] = msg_len & 0xFF;          // LSB of msg len
  send_msg[1] = (msg_len & 0xFF00) >> 8; // MSB of msg len
  send_msg[2] = ISC_TTS_SPEAK_REQ & 0xFF;
  send_msg[3] = (ISC_TTS_SPEAK_REQ & 0xFF00) >> 8;
  send_msg[4] = flush_enable; // flush control

  for (int i = 0; i < txt_len; i++) {
    send_msg[i+5] = text_to_speech[i];
  }

  send_msg[msg_len-1] = '\0'; // null character
  tts_send_message(send_msg, msg_len);

  response = tts_parse_response(ISC_TTS_SPEAK_RESP, 0x0000, 16); 
  
  WAIT_UNTIL_FINISHED; 
  
  return response; 

}

int tts_speech_progmem(PGM_P flash_address, unsigned char flush_enable)
{

  int response;
  int msg_len = 0; 
  char c = 0; 

  do {
    c = (char) pgm_read_byte(flash_address++);
    send_msg[msg_len + 5] = c; 
    msg_len++; 
  } while (c!='\0');
    
  msg_len += 5; 
  send_msg[0] = msg_len & 0xFF;          // LSB of msg len
  send_msg[1] = (msg_len & 0xFF00) >> 8; // MSB of msg len
  send_msg[2] = ISC_TTS_SPEAK_REQ & 0xFF;
  send_msg[3] = (ISC_TTS_SPEAK_REQ & 0xFF00) >> 8;
  send_msg[4] = flush_enable; // flush control

  tts_send_message(send_msg, msg_len); 

  response = tts_parse_response(ISC_TTS_SPEAK_RESP, 0x0000, 16); 

  WAIT_UNTIL_FINISHED; 

  return response; 

}

void tts_speech_start(unsigned char flush_enable)
{
  send_msg[0] = 0; // length to be set later!
  send_msg[1] = 0; 
  send_msg[2] = ISC_TTS_SPEAK_REQ & 0xFF;
  send_msg[3] = (ISC_TTS_SPEAK_REQ & 0xFF00) >> 8;
  send_msg[4] = flush_enable; // flush control

  msg_index = 0; 

}

int tts_speech_content(char* text_to_speech, int done)
{
  int response = 0; 
  txt_len = strlen(text_to_speech); 

  for (int i = 0; i < txt_len; i++) { 
    send_msg[5 + msg_index] = text_to_speech[i]; 
    msg_index++; 
  }
  
  if (done) {

    send_msg[5 + msg_index] = '\0'; // null character

    msg_index += 6 ;
    send_msg[0] =  msg_index & 0xFF;          // LSB of msg len
    send_msg[1] = (msg_index & 0xFF00) >> 8; // MSB of msg len

    tts_send_message(send_msg, msg_index);

    response = tts_parse_response(ISC_TTS_SPEAK_RESP, 0x0000, 16); 

    WAIT_UNTIL_FINISHED; 
    
  }

  return response; 

}

int tts_speech_char(char text_to_speech, int done)
{
  int response = 0; 

  send_msg[5 + msg_index] = text_to_speech; 
  msg_index++; 

  if (done) {

    send_msg[5 + msg_index] = '\0'; // null character

    msg_index += 6 ;
    send_msg[0] =  msg_index & 0xFF;          // LSB of msg len
    send_msg[1] = (msg_index & 0xFF00) >> 8; // MSB of msg len

    tts_send_message(send_msg, msg_index);

    response = tts_parse_response(ISC_TTS_SPEAK_RESP, 0x0000, 16); 

    WAIT_UNTIL_FINISHED; 

  }

  return response; 

}

int tts_configure(SYNTH_MODE mode)
{
  msg_len = 0x0C;

  send_msg[0] = msg_len & 0xFF;          // LSB of msg len
  send_msg[1] = (msg_len & 0xFF00) >> 8; // MSB of msg len
  send_msg[2] = ISC_TTS_CONFIG_REQ & 0xFF;
  send_msg[3] = (ISC_TTS_CONFIG_REQ & 0xFF00) >> 8;
  send_msg[4] = ISC_TTS_SAMPLE_RATE;
  send_msg[5] = VOICE-1; // we are starting with voice 1; 0 is needed for getter methods as termination token!
  send_msg[6] = (mode == EPSON_M ? ISC_TTS_EPSON_PARSE : ISC_TTS_DECTALK_PARSE); 
  send_msg[7] = LANGUAGE;
  
  send_msg[8] = convert_speak_rate(SPEAK_RATE) % 256; 
  send_msg[9] = convert_speak_rate(SPEAK_RATE) / 256; 

  send_msg[10] = ISC_TTS_DATASOURCE;
  send_msg[11] = 0x00;

  tts_send_message(send_msg, msg_len);

  return tts_parse_response(ISC_TTS_CONFIG_RESP, 0x0000, 16); 
}


int tts_configure_dectalk(void) { 
  return tts_configure(DECTALK_M); 
}

int tts_configure_epson(void) { 
  return tts_configure(EPSON_M); 
}

int tts_configure_current(void) {  
  return tts_configure(CUR_MODE);  
}

int tts_set_volume(void) {
  char setvol_code[]={0x06, 0x00, 0x0A, 0x00, 0x00, 0x00};
  tts_send_message(setvol_code, 0x06);
  return tts_parse_response(ISC_AUDIO_VOLUME_RESP, 0x0000, 16); 
}

int tts_stop(void) {
  STOP_NOW = 0; 
  char setvol_code[]={0x06, 0x00, 0x18, 0x00, 0x00, 0x00};
  tts_send_message(setvol_code, 0x06);
  return tts_parse_response(ISC_TTS_STOP_RESP, 0x0000, 16); 
}


void tts_setup(void) {
   
  spi_begin_epson(); 
  tts_reset();

  int tmp = tts_get_version();
  check_for_error(tmp == 0x0402); 

  int success = tts_download();
  check_for_error(success);

  success = tts_boot_run();  
  check_for_error(success);

  _delay_ms(150); // Wait for the boot image to execute

  success = tts_registration();
  check_for_error(success);

  // Once again print version information 
  tts_get_version(); 

  success = tts_configure_audio();
  check_for_error(success);

  success = tts_set_volume();
  check_for_error(success); 

  success = tts_configure_epson();
  check_for_error(success);

#ifdef BOOTMESSAGE
  char mytext[] = "T R S Talker 80 initialized."; 

  success = tts_speech(mytext,0);
  check_for_error(success);
#endif

}

void speak(char message[]) {
  int success = tts_speech(message, 0);
  check_for_error(success);
 
}

void speak_progmem(PGM_P flash_address ) {
  int success = tts_speech_progmem(flash_address, 0);
  check_for_error(success); 
}

//
// Talker/80
// 

/* 

JACKET	JH AE K AH T
JACKET(2)	JH AE K IH T
ENLIST 	EH N L IH S T
HEAVY	HH EH V IY
BUTTER	B AH T ER
MADE	M EY D
AZURE	AE ZH ER
HONEST	AA N AH S T
INHIBIT	IH N HH IH B AH T
MAT	M AE T
SUN	S AH N
BAG	B AE G
VAN	V AE N
CHIP	CH IH P
SHOP	SH AA P
ZOO	Z UW
AWFUL	AA F AH L
AWFUL(2)	AO F AH L
THING	TH IH NG
FATHER	F AA DH ER
LOOKING	L UH K IH NG
BOOK	B UH K
LAND	L AE N D
TRICK 	T R IH K
JUDGE 	JH AH JH
HELLO	HH AH L OW
HELLO(2)	HH EH L OW
GET	G EH T
GET(2)	G IH T
FAST	F AE S T
PAID	P EY D
PASS	P AE S
DAY	D EY
YARD	Y AA R D
MISSION	M IH SH AH N
MOP	M AA P
PAST	P AE S T
COLD	K OW L D
PIN	P IH N
MOVE	M UW V
ANY	EH N IY
TAP	T AE P
RED	R EH D
MEET	M IY T
WIN	W IH N
DAD	D AE D
AFTER	AE F T ER
SALTY	S AO L T IY
ABOUT	AH B AW T
UNCLE	AH NG K AH L
CUP	K AH P
FOR	F AO R
FOR(2)	F ER
FOR(3)	F R ER
ABOARD	AH B AO R D
YOU	Y UW
THE	DH AH
THE(2)	DH IY
THIN	TH IH N
BIRD	B ER D
GET	G EH T
GET(2)	G IH T
BE	B IY
CALL	K AO L

*/

void init_allophones(void) {

  // VOTRAX SC01 -> ARPABET (DECTALK) 

  sc01_map[0x00] = "ih"; // EH3 jackEt
  sc01_len[0x00] = 59;

  sc01_map[0x01] = "eh"; // EH2 Enlist   
  sc01_len[0x01] = 71;

  sc01_map[0x02] = "eh"; // EH1 hEAvy  
  sc01_len[0x02] = 121; 

  sc01_map[0x03] = "_";  // PA0 Pause 
  //sc01_map[0x03] = "";  // PA0 Pause 
  sc01_len[0x03] = 47;

  sc01_map[0x04] = "t"; // DT buTTer 
  sc01_len[0x04] = 47;

  sc01_map[0x05] = "ey"; // A2 mAde 
  sc01_len[0x05] = 71;

  sc01_map[0x06] = "ey"; // A1 mAde 
  sc01_len[0x06] = 103; 

  sc01_map[0x07] = "zh"; // ZH aZure
  sc01_len[0x07] = 90;

  sc01_map[0x08] = "aa"; // AH2 hOnest
  sc01_len[0x08] = 71;

  sc01_map[0x09] = "ah"; // I3 inhibIt 
  sc01_len[0x09] = 55;

  sc01_map[0x0a] = "ih"; // I2 Inhibit 
  sc01_len[0x0a] = 80;

  sc01_map[0x0b] = "ih"; // I1 InhIbit 
  sc01_len[0x0b] = 121; 

  sc01_map[0x0c] = "m"; // M Mat
  sc01_len[0x0c] = 103;

  sc01_map[0x0d] = "n"; // N suN
  sc01_len[0x0d] = 80;

  sc01_map[0x0e] = "b"; // B Bag
  sc01_len[0x0e] = 71;

  sc01_map[0x0f] = "v"; // V Van
  sc01_len[0x0f] = 71;

  sc01_map[0x10] = "ch"; // CH* CHip, T must precede CD to produce CH sound 
  sc01_len[0x10] = 71;

  sc01_map[0x11] = "sh"; // SH SHop
  sc01_len[0x11] = 121;

  sc01_map[0x12] = "z"; // Z Zoo 
  sc01_len[0x12] = 71;

  sc01_map[0x13] = "ao"; // AW1 lAWful   
  sc01_len[0x13] = 146;

  sc01_map[0x14] = "nx"; // NG thiNG   // ng ??
  sc01_len[0x14] = 121;

  sc01_map[0x15] = "aa"; // AH1 FAther 
  sc01_len[0x15] = 146;

  sc01_map[0x16] = "uh"; // OO1 lOOking 
  sc01_len[0x16] = 103;

  sc01_map[0x17] = "uh"; // OO bOOk 
  sc01_len[0x17] = 185;

  sc01_map[0x18] = "ll"; // L Land 
  sc01_len[0x18] = 103;

  sc01_map[0x19] = "k"; // K triCK 
  sc01_len[0x19] = 80;

  sc01_map[0x1a] = "jh"; // J Judge, D must precede J to produce J sound 
  sc01_len[0x1a] = 47;

  sc01_map[0x1b] = "hx"; // H Hello  // hh ??
  sc01_len[0x1b] = 71;

  sc01_map[0x1c] = "g"; // G Get
  sc01_len[0x1c] = 71;

  sc01_map[0x1d] = "f"; // F Fast
  sc01_len[0x1d] = 103;

  sc01_map[0x1e] = "d"; // D paiD
  sc01_len[0x1e] = 55;

  sc01_map[0x1f] = "s"; // S paSS 
  sc01_len[0x1f] = 90;

  sc01_map[0x20] = "ey"; // A dAY 
  sc01_len[0x20] = 185;

  sc01_map[0x21] = "ey"; // AY dAY 
  sc01_len[0x21] = 65;

  sc01_map[0x22] = "yx"; // Y1 Yard // y ? 
  sc01_len[0x22] = 80;

  sc01_map[0x23] = "ah"; // UH3 missIOn
  sc01_len[0x23] = 47;

  sc01_map[0x24] = "aa"; // AH mOp 
  sc01_len[0x24] = 250;

  sc01_map[0x25] = "p"; // P Past 
  sc01_len[0x25] = 103;

  sc01_map[0x26] = "ow"; // O cOld  
  sc01_len[0x26] = 185;

  sc01_map[0x27] = "ih"; // I pIn 
  sc01_len[0x27] = 185;

  sc01_map[0x28] = "uw"; // U mOve  
  sc01_len[0x28] = 185;

  sc01_map[0x29] = "iy"; // Y anY   
  sc01_len[0x29] = 103;

  sc01_map[0x2a] = "t"; // T Tap 
  sc01_len[0x2a] = 71;

  sc01_map[0x2b] = "r"; // R Red
  sc01_len[0x2b] = 90;

  sc01_map[0x2c] = "iy"; // E mEEt 
  sc01_len[0x2c] = 185; 

  sc01_map[0x2d] = "w"; // W Win
  sc01_len[0x2d] = 80;

  sc01_map[0x2e] = "ae"; // AE dAd 
  sc01_len[0x2e] = 185;

  sc01_map[0x2f] = "ae"; // AE1 After 
  sc01_len[0x2f] = 103;

  sc01_map[0x30] = "ao"; // AW2 sAlty 
  sc01_len[0x30] = 90;

  sc01_map[0x31] = "ah"; // UH2 About 
  sc01_len[0x31] = 71;

  sc01_map[0x32] = "ah"; // UH1 Uncle 
  sc01_len[0x32] = 103;

  sc01_map[0x33] = "ah"; // UH3 cUp 
  sc01_len[0x33] = 185;

  sc01_map[0x34] = "ao"; // O2 fOr 
  sc01_len[0x34] = 80;

  sc01_map[0x35] = "ao"; // O1 abOArd 
  sc01_len[0x35] = 121;

  sc01_map[0x36] = "yx"; // IU yOU 
  sc01_len[0x36] = 159;

  sc01_map[0x37] = "uw"; // U1 yOU 
  sc01_len[0x37] = 90;

  sc01_map[0x38] = "dh"; // THV THe 
  sc01_len[0x38] = 80;

  sc01_map[0x39] = "th"; // TH THin
  sc01_len[0x39] = 71;

  sc01_map[0x3a] = "er"; // ER bIrd 
  sc01_len[0x3a] = 146;

  sc01_map[0x3b] = "eh"; // EH gEt 
  sc01_len[0x3b] = 185;

  sc01_map[0x3c] = "iy"; // E1 bE 
  sc01_len[0x3c] = 121;

  sc01_map[0x3d] = "ao"; // AW cAll
  sc01_len[0x3d] = 250;

  //sc01_map[0x3e] = "_"; // PA1 = no sound 
  sc01_map[0x3e] = "_"; // PA1 = no sound 
  sc01_len[0x3e] = 185;

  sc01_map[0x3f] = "_"; // STOP = no sound
  sc01_len[0x3f] = 47;

  // RS TRS80 Voice Synthesizer 

    // codes 00 to 31 
    trs_to_sc01_map['@'] = 0x06; // A1
    trs_to_sc01_map['A'] = 0x08; // A2
    trs_to_sc01_map['B'] = 0x0e; // B 
    trs_to_sc01_map['C'] = 0x10; // CH
    trs_to_sc01_map['D'] = 0x1e; // D
    trs_to_sc01_map['E'] = 0x3c; // E1
    trs_to_sc01_map['F'] = 0x1d; // F
    trs_to_sc01_map['G'] = 0x1c; // G
    trs_to_sc01_map['H'] = 0x1b; // H
    trs_to_sc01_map['I'] = 0x0b; // I1
    trs_to_sc01_map['J'] = 0x1a; // J
    trs_to_sc01_map['K'] = 0x19; // K
    trs_to_sc01_map['L'] = 0x18; // L
    trs_to_sc01_map['M'] = 0x0c; // M
    trs_to_sc01_map['N'] = 0x0d; // N
    trs_to_sc01_map['O'] = 0x35; // O1
    trs_to_sc01_map['P'] = 0x25; // P
    trs_to_sc01_map['Q'] = 0x38; // DH 
    trs_to_sc01_map['R'] = 0x2b; // R
    trs_to_sc01_map['S'] = 0x1f; // S
    trs_to_sc01_map['T'] = 0x2a; // T
    trs_to_sc01_map['U'] = 0x37; // U1
    trs_to_sc01_map['V'] = 0x0f; // V 
    trs_to_sc01_map['W'] = 0x2d; // W 
    trs_to_sc01_map['X'] = 0x07; // ZH
    // trs_to_sc01_map['Y'] = 0x22; // Y1
    trs_to_sc01_map['Y'] = 0x29; // Y - corrected by MW
    trs_to_sc01_map['Z'] = 0x12; // Z 
    trs_to_sc01_map['['] = 0x34; // O2
    trs_to_sc01_map['\\'] = 0x26; // O
    trs_to_sc01_map[']'] = 0x31; // AH
    trs_to_sc01_map['^'] = 0x20; // A 
    trs_to_sc01_map['_'] = 0x03; // NULL (PA0?) 
 
    // codes 32 to 62 (63 = "?" = OPEN/CLOSE WINDOW) 

    trs_to_sc01_map[' '] = 0x3e; // PA1
    trs_to_sc01_map['!'] = 0x0a; // I2
    trs_to_sc01_map['\"'] = 0x27; // I
    trs_to_sc01_map['#'] = 0x09; // I3
    trs_to_sc01_map['$'] = 0x17; // OO
    trs_to_sc01_map['%'] = 0x16; // OO1
    // trs_to_sc01_map['&'] = 0x29; // Y
    trs_to_sc01_map['&'] = 0x22; // Y1 - corrected by MW
    trs_to_sc01_map['\''] = 0x28; // U
    trs_to_sc01_map['('] = 0x36; // IU
    trs_to_sc01_map[')'] = 0x05; // A2
    trs_to_sc01_map['*'] = 0x21; // AY
    trs_to_sc01_map['+'] = 0x14; // NG
    trs_to_sc01_map[','] = 0x3d; // AW

    // ??? : 
    trs_to_sc01_map['-'] = 0x3f; // 0 DEC. ??? GUESSING STOP 

    trs_to_sc01_map['.'] = 0x2c; // E
    trs_to_sc01_map['/'] = 0x3a; // ER
    trs_to_sc01_map['0'] = 0x03; // PA0 
    trs_to_sc01_map['1'] = 0x13; // AW1
    trs_to_sc01_map['2'] = 0x30; // AW2
    trs_to_sc01_map['3'] = 0x02; // EH1
    trs_to_sc01_map['4'] = 0x01; // EH2
    trs_to_sc01_map['5'] = 0x00; // EH3
    trs_to_sc01_map['6'] = 0x32; // UH1
    trs_to_sc01_map['7'] = 0x31; // UH2
    trs_to_sc01_map['8'] = 0x33; // UH3
    trs_to_sc01_map['9'] = 0x2f; // AE1
    trs_to_sc01_map[':'] = 0x2e; // AE
    trs_to_sc01_map[';'] = 0x15; // AH1
    trs_to_sc01_map['<'] = 0x38; // THV
    trs_to_sc01_map['='] = 0x39; // TH
    trs_to_sc01_map['>'] = 0x11; // SH 

}

/* 
void init_allophones(void) {
  sc01_map[00] = "eh"; // EH3 jackEt
  sc01_map[01] = "eh"; // EH2 Enlist   
  sc01_map[02] = "eh"; // EH1 hEAvy  ?
  sc01_map[03] = "_";  // pause 
  sc01_map[04] = "tx"; // DT buTTer 
  sc01_map[05] = "ey"; // A2 mAde 
  sc01_map[06] = "ey"; // A1 mAde 
  sc01_map[07] = "zh"; // ZH aZure
  sc01_map[08] = "ah"; // AH2 hOnest
  sc01_map[09] = "ih"; // I3 inhibIt 
  sc01_map[0a] = "ih"; // I2 Inhibit 
  sc01_map[0b] = "ih"; // I3 InhIbit 
  sc01_map[0c] = "m"; // M Mat
  sc01_map[0d] = "n"; // N suN
  sc01_map[0e] = "b"; // B Bag
  sc01_map[0f] = "v"; // V Van

  sc01_map[10] = "ch"; // CH* CHip, T must precede CD to produce CH sound 
  sc01_map[11] = "sh"; // SH SHop
  sc01_map[12] = "z"; // Z Zoo 
  sc01_map[13] = "aw"; // AW1 lAWful  ?? 
  sc01_map[14] = "nx"; // MG thiNG  
  sc01_map[15] = "ax"; // AH1 FAther ?
  sc01_map[16] = "aa"; // OO1 lOOking 
  sc01_map[17] = "uw"; // OO bOOk 
  sc01_map[18] = "l"; // L Land 
  sc01_map[19] = "k"; // K triCK 
  sc01_map[1a] = "jh"; // J Judge, D must precede J to produce J sound 
  sc01_map[1b] = "hx"; // H Hello 
  sc01_map[1c] = "g"; // G Get
  sc01_map[1d] = "f"; // F Fast
  sc01_map[1e] = "d"; // D paiD
  sc01_map[1f] = "s"; // S paSS 

  sc01_map[20] = "ey"; // A dAY 
  sc01_map[21] = "ey"; // AY dAY 
  sc01_map[22] = "yx"; // Y1 Yard 
  sc01_map[23] = "uh"; // UH3 missIOn
  sc01_map[24] = "ah"; // AH mOp 
  sc01_map[25] = "p"; // P Past 
  sc01_map[26] = "ow"; // O cOld  ? 
  sc01_map[27] = "ih"; // I pIn 
  sc01_map[28] = "uw"; // U mOve  ?
  sc01_map[29] = "iy"; // Y  anY  ?? 
  sc01_map[2a] = "t"; // T Tap 
  sc01_map[2b] = "r"; // R Red
  sc01_map[2c] = "ir"; // E mEEt 
  sc01_map[2d] = "w"; // W Win
  sc01_map[2e] = "ae"; // AE dAd 
  sc01_map[2f] = "ae"; // AE1 After 

  sc01_map[30] = "aw"; // AE2 sAlty 
  sc01_map[31] = "uh"; // UH2 About 
  sc01_map[32] = "uh"; // UH1 Uncle 
  sc01_map[33] = "uh"; // UH3 cUp 
  sc01_map[34] = "or"; // O2 fOr 
  sc01_map[35] = "ao"; // O1 abOArd 
  sc01_map[36] = "yu"; // IO yOU  ??
  sc01_map[37] = "yu"; // U1 yOU  ??
  sc01_map[38] = "dh"; // THV THe 
  sc01_map[39] = "th"; // TH THin
  sc01_map[3a] = "er"; // ER bIrd 
  sc01_map[3b] = "eh"; // EH gEt 
  sc01_map[3c] = "iy"; // E1 bE  ??
  sc01_map[3d] = "aw"; // AW cAll
  sc01_map[3e] = "_"; // no sound 
  sc01_map[3f] = "_"; // no sound

}

*/ 


//
// Initialization
// 


void init_pins(void) {

  CONFIGURE_TO_TRS80_INPUT;    // DDRD 
  CONFIGURE_FROM_TRS80_OUTPUT; // DDRA  
  CONFIGURE_TTS_INOUT;         // DDRB 
  CONFIGURE_LEDS;              // DDRC 
  CONFIGURE_RESET;             // PORTC Pullup 
  return; 

}

//
//
//
 
void speak_buffer(void) {

  uint8_t i = read_cur; 
  uint8_t speak_u = speak_until; 

  read_cur = speak_u;

  if (CUR_MODE == TRS_M || CUR_MODE == VS100_M ) {

    tts_speech_start(0);     
    sprintf(command_string, "[:phone arpa speak on][:rate %d][:n%d][", convert_speak_rate(SPEAK_RATE), VOICE-1);    
    tts_speech_content(command_string, 0);  
    
    while (i != speak_u) {     

      uint8_t buffered = buffer[i++]; 

      if ( buffered == SINGLE_PHONEME_MARKER ) 
	continue; 

      uint8_t single = (i != speak_u) && (buffer[i] == SINGLE_PHONEME_MARKER); 

      uint8_t byte  = (CUR_MODE == VS100_M) ? (buffered & 0b00111111) : buffered;   
      uint8_t pitch = (CUR_MODE == VS100_M) ? (buffered & 0b11000000) >> 6 : BASE_PITCH; 

      if (single) {
	LED_SINGLE_PHONEME_SPEAKING_ON; 
      }  else {
	LED_SINGLE_PHONEME_SPEAKING_OFF; 
      }

      /* 

      if (CUR_MODE == VS100_M) {
	switch (pitch) {
          case 0 :  pitch = BASE_PITCH ; break; 
          case 1 :  pitch = BASE_PITCH + 2; break; 
          case 2 :  pitch = BASE_PITCH + 4; break; 
          default : pitch = BASE_PITCH + 6; 
	}
      }
      */ 

      uint8_t map_index = (CUR_MODE == TRS_M) ? trs_to_sc01_map[byte] : byte; 

      char*    allo = sc01_map[map_index]; 
      uint8_t   dur = sc01_len[map_index]; 

      if (single) {
	if ( *allo == '_') {
	  //tts_speech_content(" ", 0); 
	} else {
	  tts_speech_content(allo, 0); 
	}
      } else {	
	if (ALTERNATE) { 
	  // ALTERNATE uses DECtalk "singing mode"
	  if ( *allo == '_') {
	    sprintf(command_string, " "); 
	  } else {
	    sprintf(command_string, " %s<%d,%d>", allo, dur, pitch); 
	  }
	  tts_speech_content(command_string, 0);  
	} else {
	  // NORMAL Mode 
	  tts_speech_content(allo, 0); 
	  tts_speech_content(" ", 0); 
	}      
      }      
    }  

    //
    // end of phoneme buffer speak loop 
    // 

    sprintf(command_string, "][:n%d]", VOICE-1);
    //sprintf(command_string, "]"); 
    tts_speech_content(command_string, 1); 

  } else {

    // DECtalk / EPSON Speaking 
    
    tts_speech_start(0); 
    
    while (i != speak_u) {      
      char c = buffer[i++]; 
      tts_speech_char(c, speak_u == i ? 1 : 0 ); 
    }
  }

  return;

}

//
//
//

void speak_dectalk(char* speak_buffer) {

  unsigned short txt_len = strlen(speak_buffer); 
 
  tts_speech_start(0); 
  // ADD VOICE AND RATE HERE!!!

  sprintf(command_string, "[:rate %d][:n%d]", convert_speak_rate(SPEAK_RATE), VOICE-1);
  tts_speech_content(command_string, 0); 
   
  for (int i = 0; i < txt_len; i++) {
    char c = speak_buffer[i]; 
    tts_speech_char(c, 0); 
  } 

  sprintf(command_string, "[:n%d]", VOICE-1);
  tts_speech_content(command_string, 1); 

  return;

}

//
//
// 

void command_confirm(char* message) {
  if (CONFIRM_COMMANDS) {
    //if (CUR_MODE != EPSON_M) 
    // also ways do that, because voice might have changed etc. 
    tts_configure_epson();
    speak(message);  
    //if (CUR_MODE != EPSON_M) 
    tts_configure_current();
  }
} 

void speech_ready_message(void) {

  int confirm = CONFIRM_COMMANDS; 
  CONFIRM_COMMANDS = 1;   

  LEDS_ON; 

  switch (CUR_MODE) {
  case TRS_M : TRS_COUNTER_BASED_SPEAKING ? command_confirm("T R S Voice Synthesizer mode 2.") : command_confirm("T R S Voice Synthesizer mode."); break; 
  case VS100_M : command_confirm("V S 100 mode."); break; 
  case EPSON_M : command_confirm("Epson mode."); break; 
  case DECTALK_M : command_confirm("DecTalk mode."); break; 
  default : break; 

  }

  if (BLOCKING_SPEECH) {
    command_confirm("Blocking speech."); 
  } else {
    command_confirm("Non-blocking speech."); 
  }

  CONFIRM_COMMANDS = confirm; 

  return; 

}

//
//
// 

void native_mode_epson(void) {
  CUR_MODE = EPSON_M; 
  command_confirm("Native Epson mode."); 
}

void native_mode_dectalk(void) {
  CUR_MODE = DECTALK_M; 
  command_confirm("Native DecTalk mode.");  
  tts_configure_dectalk();
}

void trs_mode(void) {
  CUR_MODE = TRS_M; 
  TRS_COUNTER_BASED_SPEAKING = 0; 
  command_confirm("T R S Voice Synthesizer mode."); 
  tts_configure_dectalk();  
}

void trs_mode_counter_based(void) {
  CUR_MODE = TRS_M; 
  TRS_COUNTER_BASED_SPEAKING = 1; 
  command_confirm("T R S Voice Synthesizer mode 2."); 
  tts_configure_dectalk();  
}

void vs100_mode(void) {
  CUR_MODE = VS100_M; 
  command_confirm("V S 100 mode."); 
  tts_configure_dectalk();  
}


//
//
// 

void confirmations_on(void) {
  CONFIRM_COMMANDS = 1; 
  command_confirm("Confirmations on."); 
}

void confirmations_off(void) {
  //confirm_command("Confirmations off."); 
  CONFIRM_COMMANDS = 0;   
}

void set_voice(uint8_t voice) {  

  VOICE = voice; 
  sprintf(command_string, "Voice set to %d.", voice); 
  command_confirm(command_string); 
}

void set_voice_default(void) {    
  set_voice(ISC_TTS_VOICE+1);   
}

void set_volume(uint8_t volume) {  

  VOLUME = volume; 
  tts_configure_audio();
  sprintf(command_string, "Volume set to %d.", volume); 
  command_confirm(command_string); 
} 

void set_volume_default(void) {    
  set_volume(VOLUME_DEFAULT);   
}

static uint8_t pitch_bytes[] = { 1, 3, 4, 5, 6, 8, 10, 12, 13, 17, 22, 24, 29, 32, 34, 37 }; 


void set_pitch(uint8_t voice) {  

  BASE_PITCH = pitch_bytes[voice]; 
  sprintf(command_string, "Pitch set to %d.", BASE_PITCH); 
  command_confirm(command_string); 
}


void set_rate(uint8_t rate) {  
  SPEAK_RATE = rate; 
  sprintf(command_string, "Speech rate set to %d.", rate); 
  command_confirm(command_string); 
} 

void set_rate_default(void) {    
  set_rate(SPEAK_RATE_DEFAULT);   
}

void announce_cur_mode(void) {  
  speech_ready_message();

}

void english(void) {
  LANGUAGE = 0; 
  command_confirm("English mode."); 
}

void spanish(void) {
  LANGUAGE = 1; 
  command_confirm("Castilian spanish mode."); 
}

void speak_version() {  
  sprintf(command_string, "This is Talker 80 version %d.", VERSION); 
  command_confirm(command_string); 
} 

//
//
//

void disable() {  
  sprintf(command_string, "Sleeping until reset."); 
  command_confirm(command_string); 
  DISABLED = 1;   
} 


//
//
//

void blocking_speech() {  
  sprintf(command_string, "Blocking speech."); 
  command_confirm(command_string); 
  BLOCKING_SPEECH = 1;   
} 

void non_blocking_speech() {  
  sprintf(command_string, "Non-blocking speech."); 
  command_confirm(command_string); 
  BLOCKING_SPEECH = 0;   
} 

//
//
//

void enable_alternate(void) {
  ALTERNATE = 1; 
  command_confirm("Alternate phoneme synthesis."); 
}

void disable_alternate(void) {
  ALTERNATE = 0; 
  command_confirm("Normal phoneme synthesis."); 
}

//
//
// 

void speak_progmem_string_from(PGM_P flash_address) {
  tts_configure_epson(); 
  speak_progmem(flash_address);  
  tts_configure_current(); 
}

void speak_copyright_note(void) {

  uint8_t voice = VOICE; 
  uint8_t rate = SPEAK_RATE; 
  uint8_t language = LANGUAGE; 
  uint8_t volume = VOLUME; 
   
  VOICE = ISC_TTS_VOICE + 1; 
  SPEAK_RATE = SPEAK_RATE_DEFAULT; 
  LANGUAGE = ISC_TTS_LANGUAGE;  
  VOLUME = VOLUME_DEFAULT; 

  speak_progmem_string_from((PGM_P) &COPYRIGHT);  

  VOICE = voice;
  SPEAK_RATE = rate; 
  LANGUAGE = language; 
  VOLUME = volume; 

  tts_configure_current();  

}

//
//
//

static volatile uint8_t seed_initialized = 0; 

void init_random_seed(void) { 
  srand (TIMER_COUNTER); 
}

void speak_hal9000_quote(void) {

  if (! seed_initialized) {
    init_random_seed(); 
    seed_initialized = 1; 
  }
  
  uint8_t hal_quote = rand() % num_quotes; 
  speak_progmem_string_from((PGM_P) pgm_read_word(&(HAL[hal_quote])));  
}

//
//
//

void sing_daisy(void) {

  tts_configure_dectalk(); 

  tts_speech_start(0); 

  tts_speech_content("[:phone arpa speak on][:rate 200][:n0][dey<650,22>ziy<600,19> dey<650,15>ziy<600,10>gih<200,12>v miy<200,14> yurr<200,15> ae<400,12>nsax<200,15>r duw<750,10> _<400,10>]\n", 1); 

  tts_speech_start(0); 
  tts_speech_content("[:phone arpa speak on][:rate 200][:n0][ay<600,17>m hxae<500,22>f kr ey<650,19>ziy<600,15> ao<200,12>ll fao<200,14>r dhax<200,15> llah<400,17>v ao<200,19>v yu<750,17> _<400,17>]\n", 1); 

  tts_speech_start(0); 
  tts_speech_content("[:phone arpa speak on][:rate 200][:n0][ih<200,19>t wow<200,20>nt biy<200,19> ax<200,17> stay<500,22>llih<200,19>sh mae<350,17>rih<400,15>jh<150,15>]\n", 1); 

  tts_speech_start(0); 
  tts_speech_content("[:phone arpa speak on][:rate 200][:n0][ay<200,17> kae<400,19>nt ax<200,15>fow<400,12>rd ax<200,15> kae<350,12>rih<400,10>jh<150,10>]\n", 1); 

  tts_speech_start(0); 
  tts_speech_content("[:phone arpa speak on][:rate 200][:n0][bah<200,10>t yu<500,15>d lluh<200,19>k swiy<400,17>t ah<200,10>p ao<500,15>n dhax<200,19> siy<200,17>t ao<200,17>v ah<200,19> bay<200,22>six<200,19>kel<200,15> bih<400,17>llt fao<200,10>r tuw<800,15>]\n", 1); 

  tts_configure_current();
  
}

//
// Control Byte Dispatching 
// 

void process_control(uint8_t control_byte) {
  
  switch ( control_byte ) {

      // 255
    case 0xFF : process_reset(); break; 
     
      // 254 - only for documentation: 
    // case 0xFE : STOP_NOW = 1;  break; 
      // 253 
    case 0xFD : disable();  break; 
      // 252
    case 0xFC : blocking_speech();  break; 
      // 251 
    case 0xFB : non_blocking_speech();  break; 

      // 239 
    case 0xEF : native_mode_epson(); break; 
      // 238
    case 0xEE : native_mode_dectalk(); break; 
      // 237 
    case 0xED : trs_mode(); break; 
      // 236 
    case 0xEC : trs_mode_counter_based(); break; 
      // 235 
    case 0xEB : vs100_mode(); break; 
      // 234 
    case 0xEA : confirmations_on(); break;  
      // 233 
    case 0xE9 : confirmations_off(); break;   
      // 232 
    case 0xE8 : english(); break; 
      // 231 
    case 0xE7 : spanish(); break; 
      // 230 
    case 0xE6 : enable_alternate(); break;  
      // 229 
    case 0xE5 : disable_alternate(); break;   
      // 228 
    case 0xE4 : announce_cur_mode(); break; 
      // 227 
    case 0xE3 : speak_copyright_note(); break; 
      // 226 
    case 0xE2 : speak_hal9000_quote(); break; 
      // 225 
    case 0xE1 : sing_daisy(); break; 
      // 224 
    case 0xE0 : speak_version(); break; 

      // 208 - 223 
    case 0xD0 ... 0xDF : set_pitch( control_byte - 0xD0); break; 
      
      // 192 - 207
    case 0xC0 : set_voice_default(); break;
    case 0xC1 ... 0xCD : set_voice( control_byte - 0xC0); break; 

      // 176 - 191 
    case 0xB0 : set_volume_default(); break;
    case 0xB1 ... 0xBF : set_volume( control_byte - 0xB0); break;

      // 160 - 175 
    case 0xA0 : set_rate_default(); break;
    case 0xA1 ... 0xAF : set_rate( control_byte - 0xA0); break;

  }

  return; 

}

//
//
// 

void configure_mode(void) { 

  LEDS_SHOW_MODE; 
  LED_SPEAKING_ON;

  _delay_ms(200); 

  if (LAST_MODE != CUR_MODE) {
    LAST_MODE = CUR_MODE; 
    ALTERNATE = 0; 
  }
  
  switch (CUR_MODE) {
  case TRS_M     : TRS_MODE; break; 
  case VS100_M : VS100_MODE; break; 
  default : NATIVE_MODE; break; 
  }

  window_open = 0; 
  speak_until = 0; 
  last_speak_until = 0; 
  write_cur = 0; 
  read_cur = 0; 
  log_in = 0; 
  ctrl = 0; 
  nibble1 = 0; 
  nibble2 = 0;   

}


//
// Counter 
// 

ISR(TIMER0_COMPA_vect) {  // timer0 overflow interrupt

  TIMER_COUNTER++; 
  if (! BUSY ) {
    LEDS_OFF; 
  }
}

//
//
// 

ISR (INT0_vect) { // WRITE REQUEST on PD2 = INT0 

  // WE ARE AVOIDING ALL TTS FROM THE ISR, BECAUSE OF POTENTIAL DEADLOCKS! 

  if (! DISABLED ) {

    DATA_FROM_TRS80(DATABUS);       

    LED_DATA_ON; 
    BUSY = 1; 
    speech_native_busy; 

    // STOP NOW? Process it! we need a different thread here... 
    if ((DATABUS == 0xFE) && (CUR_MODE != TRS_M) && (CUR_MODE != VS100_M)) { 

      // set the flag 
      // this will cause all TTS-related routines cause to return
      // then the main while loop will have control and initiate TTS_STOP() 

      STOP_NOW = 1; 

    } else { 

      // don't allow control bytes in TRS mode, since printed ASCII characters > 127 may show up
      // also, the VS100 uses bits 6, 7 for pitch control. Would interfere. 

      // NOTE THAT STOP_NOW IS PROCESSED BY THE ISR, NOT HERE: 
      if ((DATABUS & 0b10000000) && (CUR_MODE != TRS_M) && (CUR_MODE != VS100_M)) { 
    
	CONTROL_AVAILABLE = DATABUS; 
      
      } else {

	CONTROL_AVAILABLE = 0; 

	// log in normal data into buffer

	uint8_t index = ( write_cur + 1 ) ; // wraps around 8 bit 

	if (CUR_MODE == TRS_M ) {

	  if ( index != read_cur ) {
	
	    if ( DATABUS == '?' ) {
	    
	      window_open = 1 - window_open; 

	      if (! window_open ) {
		// windows was open, now closed:
		speak_until = write_cur; 
		TIMER_COUNTER = 0;

		if (PHONEME_COUNTER == 1) {		
		  // MAKE IT SPEAK, EVEN WHEN TRS_COUNTER_BASED_SPEAKING == 1 !
		  // TIMER_COUNTER = 255; 
		  buffer[write_cur++] = SINGLE_PHONEME_MARKER; 
		  speak_until++; 
		} 
	      }

	      PHONEME_COUNTER = 0; 
	  
	    } else {
	  	   
	      if ( window_open ) {	  
		// buffer phoneme 
		PHONEME_COUNTER++; 
		buffer[write_cur++] = DATABUS; 
	      }
	    }

	  } else {
	  
	    // buffer overflow

	    blink_led_fast(50); 

	  }

	} else if (CUR_MODE == VS100_M ) {

	  if ( index != read_cur ) {	
	    buffer[write_cur++] = DATABUS; 
	    speak_until = write_cur; 
	    TIMER_COUNTER = 0;
	  } else {
	  
	    // buffer overflow
	    blink_led_fast(50); 

	  }
      
	} else {

	  // DECtalk / EPSON 
      
	  if ( index != read_cur ) { 

	    log_in = 0; 
	    
	    if ( DATABUS == '\\' && ctrl == 0 ) {
	      ctrl = 1; 
	    } else if ( ctrl == 1 ) { 
	      if ( DATABUS == 'x' ) {
		ctrl = 2; 
	      } else {
		// didn't get \x escape, so log in \ and current: 
		buffer[write_cur++] = '\\'; 
	      
		log_in = 1; 
		ctrl = 0; 
	      }
	    } else if ( ctrl == 2 ) { 
	      if ( DATABUS >= '0' && DATABUS <= '9' ) {
		nibble1 = DATABUS - '0'; 
		ctrl = 3; 
	      } else if ( DATABUS >= 'A' && DATABUS <= 'F' ) {
		nibble1 = DATABUS - 'A'; 
		ctrl = 3; 
	      } else {
		// failure to parse \xHEX HEX, ignore! 
		ctrl = 0; 
	      }
	    } else if ( ctrl == 3 ) {
	      if  ( DATABUS >= '0' && DATABUS <= '9' ) {
		nibble2 = DATABUS - '0'; 
		DATABUS = 16*nibble1 + nibble2; 
		ctrl = 0; 
		log_in = 1; 
	      } else if ( DATABUS >= 'A' && DATABUS <= 'F' ) {
		nibble2 = DATABUS - 'A'; 
		DATABUS = 16*nibble1 + nibble2; 
		ctrl = 0; 
		log_in = 1; 
	      } else { 
		ctrl = 0;  
	      }
	    } else {
	      // normal character
	      ctrl = 0; 
	      log_in = 1; 	    
	    }

	    //
	    //
	    //
	  
	    if (log_in) {
	      if ( DATABUS == 13) {
		speak_until = write_cur; 
	      } else {
		buffer[write_cur++] = DATABUS;	    
	      }
	    }

	    //
	    //
	    //

	  }  else {
	  
	    // buffer overflow
	    blink_led_fast(50); 

	  }
	}
      }
    }
  }
}


/* 
ISR (INT0_vect) { // WRITE REQUEST on PD2 = INT0 

  // _delay_us(6); 
  DATA_FROM_TRS80(DATABUS);       
  LED_DATA_ON; 
  
  if (! DISABLED ) {

    BUSY = 1; 

    // don't allow control bytes in TRS mode, since printed ASCII characters > 127 may show up
    // also, the VS100 uses bits 6, 7 for pitch control. Would interfere. 
    if ((DATABUS & 0b10000000) && (CUR_MODE != TRS_M) && (CUR_MODE != VS100_M)) { 
    
      process_control(DATABUS); 

      // don't do this for STOP NOW 
      if ( DATABUS != 0xFE ) {
	
	configure_mode();     

      } else if (STOP_NOW) {
	  
	LEDS_ON; 
	if (SPEAKING_NOW) {
	  tts_stop();       
	}

	LEDS_OFF;      
      }
      
    } else {

      uint8_t index = ( write_cur + 1 ) ; // wraps around 8 bit 

      if (CUR_MODE == TRS_M ) {

	if ( index != read_cur ) {
	
	  if ( DATABUS == '?' ) {
	    
	    window_open = 1 - window_open; 

	    if (! window_open ) {
	      // windows was open, now closed:
	      speak_until = write_cur; 
	      TIMER_COUNTER = 0;

	      if (PHONEME_COUNTER == 1) {		
		// MAKE IT SPEAK, EVEN WHEN TRS_COUNTER_BASED_SPEAKING == 1 !
		// TIMER_COUNTER = 255; 
		buffer[write_cur++] = SINGLE_PHONEME_MARKER; 
		speak_until++; 
	      } 
	    }

	    PHONEME_COUNTER = 0; 
	  
	  } else {
	  	   
	    if ( window_open ) {	  
	      // buffer phoneme 
	      PHONEME_COUNTER++; 
	      buffer[write_cur++] = DATABUS; 
	    }
	  }

	} else {
	  
	  // buffer overflow

	  blink_led_fast(50); 

	}

      } else if (CUR_MODE == VS100_M ) {

	if ( index != read_cur ) {	
	  buffer[write_cur++] = DATABUS; 
	  speak_until = write_cur; 
	  TIMER_COUNTER = 0;
	} else {
	  
	  // buffer overflow

	  blink_led_fast(50); 

	}
      
      } else {

	// DECtalk / EPSON 
      
	if ( index != read_cur ) { 

	  log_in = 0; 
	    
	  if ( DATABUS == '\\' && ctrl == 0 ) {
	    ctrl = 1; 
	  } else if ( ctrl == 1 ) { 
	    if ( DATABUS == 'x' ) {
	      ctrl = 2; 
	    } else {
	      // didn't get \x escape, so log in \ and current: 
	      buffer[write_cur++] = '\\'; 
	      
	      log_in = 1; 
	      ctrl = 0; 
	    }
	  } else if ( ctrl == 2 ) { 
	    if ( DATABUS >= '0' && DATABUS <= '9' ) {
	      nibble1 = DATABUS - '0'; 
	      ctrl = 3; 
	    } else if ( DATABUS >= 'A' && DATABUS <= 'F' ) {
	      nibble1 = DATABUS - 'A'; 
	      ctrl = 3; 
	    } else {
	      // failure to parse \xHEX HEX, ignore! 
	      ctrl = 0; 
	    }
	  } else if ( ctrl == 3 ) {
	    if  ( DATABUS >= '0' && DATABUS <= '9' ) {
	      nibble2 = DATABUS - '0'; 
	      DATABUS = 16*nibble1 + nibble2; 
	      ctrl = 0; 
	      log_in = 1; 
	    } else if ( DATABUS >= 'A' && DATABUS <= 'F' ) {
	      nibble2 = DATABUS - 'A'; 
	      DATABUS = 16*nibble1 + nibble2; 
	      ctrl = 0; 
	      log_in = 1; 
	    } else { 
	      ctrl = 0;  
	    }
	  } else {
	    // normal character
	    ctrl = 0; 
	    log_in = 1; 	    
	  }

	  //
	  //
	  //
	  
	  if (log_in) {
	    if ( DATABUS == 13) {
	      speak_until = write_cur; 
	    } else {
	      buffer[write_cur++] = DATABUS;	    
	    }
	  }

	  //
	  //
	  //

	}  else {
	  
	  // buffer overflow

	  blink_led_fast(50); 

	}
      }
    }
  }
}


ISR (INT1_vect) { // READ REQUEST on PD3 = INT1 
  if (! DISABLED ) {
    if (bit_is_set(IOREQ_PIN, IOREQ_READ)) {
      if ( SPEAKING_NOW || BUSY ) {
	speech_native_busy; 
      } else {
	speech_native_ready; 
      }
    }
  }
}

*/ 


//
//
//


int main(void) {  
  
  MCUSR = 0;
  MCUSR = 0;

  wdt_init();  // soft reset init 
  init_reset_handler();   

  // disable JTAG ! SET IT TWICE REALLY!!!
  MCUCR=0x80;// for the atmega644
  MCUCR=0x80;// for the atmega644

  //
  //
  //

  init_pins();  
  LEDS_OFF;
  blink_led_fast(20);   
  init_allophones();  
  tts_setup(); 

  //
  //
  // 

  CUR_MODE = DIP_CONF_PIN & 0b00000011; // PC0 and PC1 = DIP Switch 
  configure_mode(); 

  //
  //
  //

#ifdef BOOTMESSAGE  
  speech_ready_message(); 
#endif 

  tts_configure_dectalk();

  //
  // diagnostics 
  //

  /* 
     if (bit_is_set(diag, JTRF)) {
     blink_led_slow(1);
     }

     if (bit_is_set(diag, WDRF)) {
     blink_led_slow(2);
     }
  
     if (bit_is_set(diag, BORF)) {
     blink_led_slow(3);
     }

     if (bit_is_set(diag, EXTRF)) {
     blink_led_slow(4);
     }

     if (bit_is_set(diag, EXTRF)) {
     blink_led_slow(5);
     }

     blink_led_fast(100); 
    
  */ 

  //
  //
  //

  // INT0 = PD2 = IOREQ_WRITE EXTERNAL INTERUPT ENABLE 
  // INT1 = PD3 = IOREQ_READ  EXTERNAL INTERUPT ENABLE 
  // RISING EDGE 

  EIMSK = 0; 
  //EICRA = BV(ISC00) | BV(ISC01) | BV(ISC10) | BV(ISC11);  
  // ENABLE 
  //EIMSK = BV(INT0) | BV(INT1); 

  EICRA = BV(ISC00) | BV(ISC01); 
  // ENABLE 
  EIMSK = BV(INT0); 

  //clearBit(EICRA, ISC01); 
  //setBit(EICRA,   ISC00); 
  //setBit(EIMSK, INT0); 

  //
  //
  // 

  sei(); 
  LEDS_OFF;  
  last_speak_until = speak_until;  

  // uint16_t counter = 0; 
  // uint8_t counter2 = 0; 
  // uint8_t armed = 0; 

  configure_timer; 
  start_timer;

  CONTROL_AVAILABLE = 0; 
  
  while (1) {

    speech_native_ready; 
    BUSY = 0; 

    //
    // check for stop 
    //

    if (STOP_NOW) {        

      STOP_NOW = 0; 
      BUSY = 1; 
      speech_native_busy; 
  
      LEDS_ON; 
      tts_stop();         
      _delay_ms(200); 	
      //tts_configure_epson();
      //speak("stop");  
      tts_configure_current();
      LEDS_OFF;      

    } 

    //
    // check for control bytes requested 
    //
    
    else if (CONTROL_AVAILABLE) {

        BUSY = 1; 
        speech_native_busy; 

	process_control(CONTROL_AVAILABLE); 
	configure_mode();     
	
	CONTROL_AVAILABLE = 0; 

    }

    //
    // speaking 
    //
   
    if ( (CUR_MODE == TRS_M   && ! TRS_COUNTER_BASED_SPEAKING && ( speak_until != last_speak_until )) ||
         (CUR_MODE == TRS_M   &&   TRS_COUNTER_BASED_SPEAKING && ( speak_until != last_speak_until ) && ( TIMER_COUNTER > COUNTER_SPEAK_DELAY )) || 
         (CUR_MODE == VS100_M                                 && ( speak_until != last_speak_until ) && ( TIMER_COUNTER > COUNTER_SPEAK_DELAY )) || 
         ((CUR_MODE == DECTALK_M || CUR_MODE == EPSON_M )     && ( speak_until != last_speak_until ))) {

      BUSY = 1; 
      speech_native_busy; 

      last_speak_until = speak_until; 

      LED_SPEAKING_ON; 

      if ( BLOCKING_SPEECH ) {
	LED_BLOCKING_ON;
	z80_halt; 
      } 

      speak_buffer();       

      if ( BLOCKING_SPEECH ) {
	z80_run; 
	LED_BLOCKING_OFF;
      }

      TIMER_COUNTER = 0; 

    }   

    //
    // next while (1) iteration 
    // 

  }

  //
  // end main loop  
  // 

  return 0;  
   
}

