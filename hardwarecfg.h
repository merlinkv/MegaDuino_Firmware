#define Use_SoftI2CMaster
//#define Use_SoftWire
#define I2CFAST

#if defined(I2CFAST)
  #define I2C_FASTMODE  1
  #define I2CCLOCK  400000L   //100000L for StandarMode, 400000L for FastMode and 1000000L for FastModePlus
#else
  #define I2C_FASTMODE  0
  #define I2CCLOCK  100000L   //100000L for StandarMode, 400000L for FastMode and 1000000L for FastModePlus
#endif

#if defined(__arm__) && defined(__STM32F1__)
  #ifdef Use_SoftI2CMaster
    #undef Use_SoftI2CMaster
  #endif
  #ifdef Use_SoftWire
    #undef Use_SoftWire
  #endif
#endif

#ifdef TimerOne
  #include <TimerOne.h>
#elif defined(__arm__) && defined(__STM32F1__) 
  //HardwareTimer timer(2); // channel 2
  #include "TimerCounter.h"
 
/* class TimerCounter: public HardwareTimer
{
  public:
    TimerCounter(uint8 timerNum) : HardwareTimer(timerNum) {};
    void setSTM32Period(unsigned long microseconds) __attribute__((always_inline)) {}
};*/
TimerCounter timer(2);

  #include <itoa.h>  
  #define strncpy_P(a, b, n) strncpy((a), (b), (n))
  #define memcmp_P(a, b, n) memcmp((a), (b), (n)) 
  #define strcasecmp_P(a,b) strcasecmp((a), (b)) 
#elif defined(__SAMD21__)
  #include "TimerCounter.h"
  TimerCounter Timer1;
#else
  #include "TimerCounter.h"
  TimerCounter Timer1;              // preinstatiate
  
  unsigned short TimerCounter::pwmPeriod = 0;
  unsigned char TimerCounter::clockSelectBits = 0;
  void (*TimerCounter::isrCallback)() = NULL;
  
  // interrupt service routine that wraps a user defined function supplied by attachInterrupt
  #if defined(__AVR_ATmega4809__) || defined (__AVR_ATmega4808__)
    ISR(TCA0_OVF_vect)
    {
      Timer1.isrCallback();
    /* The interrupt flag has to be cleared manually */
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
    }  
  #else //__AVR_ATmega328P__
    ISR(TIMER1_OVF_vect)
    {
      Timer1.isrCallback();
    }
  #endif
#endif

#include <SdFat.h>

#define scrollSpeed   250           //text scroll delay
#define scrollWait    3000          //Delay before scrolling starts

#ifdef LCD16
  #include "LiquidCrystal_I2C_Soft.h"
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR,16,2); // set the LCD address to 0x27 for a 16 chars and 2 line display
  char indicators[] = {'|', '/', '-',0};
  uint8_t SpecialChar [8]= { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
  #define SCREENSIZE 16
#elif defined(LCD20)
  #include "LiquidCrystal_I2C_Soft.h"
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR,20,4); // set the LCD address to 0x27 for a 20 chars and 4 line display
  char indicators[] = {'|', '/', '-',0};
  uint8_t SpecialChar [8]= { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
  #define SCREENSIZE 20
#elif defined(OLED1306)
  #if defined(Use_SoftI2CMaster) && defined(__AVR_ATmega2560__) 
    #define SDA_PORT PORTD
    #define SDA_PIN 1 
    #define SCL_PORT PORTD
    #define SCL_PIN 0 
    #include <SoftI2CMaster.h>         
  #elif defined(Use_SoftI2CMaster) 
    #define SDA_PORT PORTC
    #define SDA_PIN 4 
    #define SCL_PORT PORTC
    #define SCL_PIN 5 
    #include <SoftI2CMaster.h>        
  #elif defined(Use_SoftWire) && defined(__AVR_ATmega2560__) 
    #define SDA_PORT PORTD
    #define SDA_PIN 1 
    #define SCL_PORT PORTD
    #define SCL_PIN 0 
    #include <SoftWire.h>
    SoftWire Wire = SoftWire();
  #elif defined(Use_SoftWire) 
    #define SDA_PORT PORTC
    #define SDA_PIN 4 
    #define SCL_PORT PORTC
    #define SCL_PIN 5 
    #include <SoftWire.h>
    SoftWire Wire = SoftWire();  
  #else
    #include <Wire.h>
  #endif
  char indicators[] = {'|', '/', '-',92};
  #define SCREENSIZE 16  
#else
  #define SCREENSIZE 16
#endif
