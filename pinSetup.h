#ifndef PINSETUP_H_INCLUDED
#define PINSETUP_H_INCLUDED

#include "Arduino.h"

#if defined(__AVR_ATmega2560__)
  #define outputPin           23 
  #define INIT_OUTPORT        DDRA |=  _BV(1)         // El pin23 es el bit1 del PORTA
  #define WRITE_LOW           PORTA &= ~_BV(1)         // El pin23 es el bit1 del PORTA
  #define WRITE_HIGH          PORTA |=  _BV(1)         // El pin23 es el bit1 del PORTA
#endif
#if defined(__arm__) && defined(__STM32F1__)
  #define outputPin           PA9    // this pin is 5V tolerant and PWM output capable
  #define INIT_OUTPORT            pinMode(outputPin,OUTPUT)
  //#define INIT_OUTPORT            pinMode(outputPin,OUTPUT); GPIOA->regs->CRH |=  0x00000030  
  #define WRITE_LOW               digitalWrite(outputPin,LOW)
  //#define WRITE_LOW               GPIOA->regs->ODR &= ~0b0000001000000000
  //#define WRITE_LOW               gpio_write_bit(GPIOA, 9, LOW)
  #define WRITE_HIGH              digitalWrite(outputPin,HIGH)
  //#define WRITE_HIGH              GPIOA->regs->ODR |=  0b0000001000000000
  //#define WRITE_HIGH              gpio_write_bit(GPIOA, 9, HIGH)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
  //General Pin settings
  //Setup buttons with internal pullup

#if defined(__AVR_ATmega2560__)

  const byte chipSelect = 53;          //Sd card chip select pin
  
  #define btnUp         A0            //Up button
  #define btnDown       A1            //Down button
  #define btnPlay       A2            //Play Button
  #define btnStop       A3            //Stop Button
  #define btnRoot       A4            //Return to SD card root
  // #define btnDelete     A5         //Not implemented this button is for an optional function
  #define btnMotor      6             //Motor Sense (connect pin to gnd to play, NC for pause)
#endif

#if defined(__arm__) && defined(__STM32F1__)
//
// Pin definition for Blue Pill boards
//

  #define chipSelect    PB12    //Sd card chip select pin
  #define btnPlay       PA0     //Play Button
  #define btnStop       PA1     //Stop Button
  #define btnUp         PA2     //Up button
  #define btnDown       PA3     //Down button
  #define btnMotor      PA8     //Motor Sense (connect pin to gnd to play, NC for pause)
  #define btnRoot       PA4     //Return to SD card root
#endif

void pinsetup();

#ifdef BUTTON_ADC
// Each button acts as a voltage divider between 10k and the following resistors:
// 0 Ohm  i.e. 100%
// 2.2k Ohm i.e. 82% (10 : 12.2)
// 4.7k Ohm i.e. 68% (10 : 14.7)
// 10k Ohm i.e. 50% (10 : 20)
// 20k Ohm i.e. 33% (10 : 30)

// For a 10-bit ADC, each button is calibrated to the band between this value and the next value above
// (or 1023 for upper limit).
// The bands are intentionally set very wide, and far apart
// However note that ESP ADC is nonlinear and not full-scale, so the resistor
// values must be chosen to avoid ranges at the extreme top (100%) end.
// The resistor values and bands chosen here are compatible with ESP devices

#if defined(ESP32) || defined(ESP8266)
// ESP ADC is nonlinear, and also not full scale, so the values are different!
// because not full scale, a 1k:10k voltage divider (i.e. 90%) is undetectable
// and reads as 1023 still, so resistor values have been altered to create better spacing
#define btnADCPlayLow 1020 // 0 ohm reading 1023 due to saturation
#define btnADCStopLow 900 // 2.2k ohm reading around 960
#define btnADCRootLow 700 // 4.7k ohm reading around 800
#define btnADCDownLow 500 // 10k ohm reading around 590
#define btnADCUpLow 200 // 20k ohm reading around 390
#else
#define btnADCPlayLow 950 // 0 ohm reading around 1000, ideally 1023
#define btnADCStopLow 800 // 2.2k ohm reading around 840
#define btnADCRootLow 600 // 4.7k ohm reading around 695
#define btnADCDownLow 420 // 10k ohm reading around 510
#define btnADCUpLow 200 // 20k ohm reading around 340
#endif

#endif // BUTTON_ADC

#endif // #define PINSETUP_H_INCLUDED
