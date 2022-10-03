#ifndef PINSETUP_H_INCLUDED
#define PINSETUP_H_INCLUDED

#ifdef __AVR_ATmega2560__
  #define outputPin           23 
  #define INIT_OUTPORT        DDRA |=  _BV(1)          // El pin23 es el bit1 del PORTA
  #define WRITE_LOW           PORTA &= ~_BV(1)         // El pin23 es el bit1 del PORTA
  #define WRITE_HIGH          PORTA |=  _BV(1)         // El pin23 es el bit1 del PORTA

#elif defined(__arm__) && defined(__STM32F1__)
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

#ifdef __AVR_ATmega2560__

  const byte chipSelect = 53;          //Sd card chip select pin
  
  #define btnUp         A0            //Up button
  #define btnDown       A1            //Down button
  #define btnPlay       A2            //Play Button
  #define btnStop       A3            //Stop Button
  #define btnRoot       A4            //Return to SD card root
  #define btnDelete     A5            //Reset MegaDuino or Delete File selected by switch
  #define btnMotor      6             //Motor Sense (connect pin to gnd to play, NC for pause)
#elif defined(__arm__) && defined(__STM32F1__)
  // Pin definition for Blue Pill boards
  #define chipSelect    PB12            //Sd card chip select pin
  
  #define btnPlay       PA0             //Play Button
  #define btnStop       PA1             //Stop Button
  #define btnUp         PA2             //Up button
  #define btnDown       PA3             //Down button
  #define btnMotor      PA8             //Motor Sense (connect pin to gnd to play, NC for pause)
  #define btnRoot       PA4             //Return to SD card root
#endif

#ifdef __AVR_ATmega2560__
  PORTF |= _BV(0);    // Analog pin A0
  PORTF |= _BV(1);    // Analog pin A1
  PORTF |= _BV(2);    // Analog pin A2
  PORTF |= _BV(3);    // Analog pin A3
  PORTF |= _BV(4);    // Analog pin A4
  PORTF |= _BV(5);    // Analog pin A5
  PORTH |= _BV(3);    // Digital pin 6
#elif defined(__arm__) && defined(__STM32F1__)
  //General Pin settings
  //Setup buttons with internal pullup 
  pinMode(btnPlay,INPUT_PULLUP);
  digitalWrite(btnPlay,HIGH);
  pinMode(btnStop,INPUT_PULLUP);
  digitalWrite(btnStop,HIGH);
  pinMode(btnUp,INPUT_PULLUP);
  digitalWrite(btnUp,HIGH);
  pinMode(btnDown,INPUT_PULLUP);
  digitalWrite(btnDown,HIGH);
  pinMode(btnMotor, INPUT_PULLUP);
  digitalWrite(btnMotor,HIGH);
  pinMode(btnRoot, INPUT_PULLUP);
  digitalWrite(btnRoot, HIGH); 
  pinMode(btnDelete, INPUT_PULLUP);
  digitalWrite(btnDelete, HIGH); 
#endif

#ifdef BUTTON_ADC
  // for a 10-bit ADC, each button is calibrated to the band between this value and the next value above (or 1023 for upper limit)
  // The bands are intentionally set very wide, and far apart
  // Each button acts as a voltage divider between 10k and the following resistors:
  #define btnADCPlayLow 980 // 0 ohm
  #define btnADCStopLow 900 // 1k ohm
  #define btnADCRootLow 700 // 2.4k ohm
  #define btnADCDownLow 400 // 10k ohm
  #define btnADCUpLow 200 // 20k ohm
#endif

#endif // #define PINSETUP_H_INCLUDED
