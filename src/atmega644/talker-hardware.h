//
// PINS 
//

#define SPI_PORT  PORTB

#define MOSI PB5
#define MISO PB6
#define SCK  PB7
#define CS   PB3

//
// EPSON CONTROL (SAME PINB/PORTB AS SPI) 
// 

#define TTS_INPUT  PINB
#define TTS_OUTPUT PORTB
#define TTS_DDR    DDRB

#define S1V30120_RDY PB0
#define S1V30120_RST PB2
#define S1V30120_CS  PB3

// All OUTPUT, TTS ClickBoard control, EXCEPT RDY INPUT 
// ~WAIT -> Z80, INPUT = RUN, OUTPUT and 0 = HALT 
// RDY ~WAIT RST CS * MOSI MISO SCK 
// 0 = INPUT, 1 = OUTPUT
#define CONFIGURE_TTS_INOUT DDRB  = 0b10111100 

//
// Z80 ~WAIT
// 

#define _Z80_READY PB1 
#define z80_run  DDRB = 0b10111100 
#define z80_halt DDRB = 0b10111110, clearBit(PORTB, _Z80_READY) 

//
// TRS80 Databus IO 
// 

#define TO_TRS80_INPUT  PORTD // PD7, PD6 
#define TO_TRS80_BIT7   PD7
#define TO_TRS80_BIT6   PD6 
#define NATIVE_SBY      PD7 // tell TRS80 that Epson chip is busy 
#define NATIVE_OTHERBIT PD6 // no purpose yet, future bit

#define MODE_PORT PORTD 
#define MODE_SEL0 PD4
#define MODE_SEL1 PD5 

#define NATIVE_MODE clearBit(MODE_PORT, MODE_SEL0); clearBit(MODE_PORT, MODE_SEL1)
#define TRS_MODE      setBit(MODE_PORT, MODE_SEL0); clearBit(MODE_PORT, MODE_SEL1)
#define VS100_MODE  clearBit(MODE_PORT, MODE_SEL0);   setBit(MODE_PORT, MODE_SEL1) 

// x x SPEECH_WRITE SPEECH_READ MODE_SEL0(OUTPUT) MODE_SEL1(OUTPUT) I6(OUTPUT) I7(OUTPUT) 
// 0 = INPUT, 1 = OUTPUT
#define CONFIGURE_TO_TRS80_INPUT     DDRD  = 0b11110000 

#define FROM_TRS80_OUTPUT PINA // -> ATMega Input 
// all input, no pullups 
#define CONFIGURE_FROM_TRS80_OUTPUT  DDRA  = 0b00000000; PORTA  = 0b00000000 

//
// Master DATABUS MACRO 
// 

#define DATA_TO_TRS80(arg)    TO_TRS80_INPUT = arg 
#define DATA_FROM_TRS80(arg)  arg = FROM_TRS80_OUTPUT 

//
// 
//

#define speech_native_ready   setBit(TO_TRS80_INPUT, NATIVE_SBY); setBit(TO_TRS80_INPUT, NATIVE_OTHERBIT)
#define speech_native_busy  clearBit(TO_TRS80_INPUT, NATIVE_SBY); setBit(TO_TRS80_INPUT, NATIVE_OTHERBIT)

//
// TRS80 Speech Data Write - From Address Decoder! Trigger  
// Now, this IOREQ can also be a Memory Address from Screen for TRS80 Voice Synth Mode
// 

#define IOREQ_PIN    PIND
#define IOREQ_WRITE  PD2 
#define IOREQ_READ   PD3 

//
// Soft Reset Button and LEDs 
// PC6 = D22 
// 

#define SOFT_RESET_INT        PCINT22 
#define SOFT_RESET_INT_VEC    PCINT2_vect
#define SOFT_RESET_PIN        PINC
#define SOFT_RESET_PIN_NUMBER PC6

// Enable Pullup Resistor for Reset! And CONF!   
#define CONFIGURE_RESET    PORTC |= 0b01000011 

//
// LEDs 
// 

#define LED_PORT     PORTC

#define LED_DATA           PC3
#define LED_BLOCKING       PC4 
#define LED_SPEAKING       PC5 
#define LED_SINGLE_PHONEME PC4

// double use for MODE DISPLAY 
#define LED_MODE0    PC4
#define LED_MODE1    PC5 

//
// 0 = INPUT, 1 = OUTPUT, * = Don't Care 
// PC7 PC6 PC5 PC4 PC3 PC2 PC1 PC0 = * Reset LED3 LED2 LED1 * CONF2 CONF1 
//

#define CONFIGURE_LEDS DDRC = 0b00111000;

//
// DIP Switch for Start Mode Config 
//  

#define DIP_CONF_PIN PINC 
#define DIP_CONF1    PC0
#define DIP_CONF2    PC1 




