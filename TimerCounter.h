/*
 *  Interrupt and PWM utilities for 16 bit Timer1 on ATmega168/328
 *  Original code by Jesse Tane for http://labs.ideo.com August 2008
 */

#define TIMER1_RESOLUTION 65536UL  // Timer1 is 16 bit

// Placing nearly all the code in this .h file allows the functions to be
// inlined by the compiler.  In the very common case with constant values
// the compiler will perform all calculations and simply write constants
// to the hardware registers (for example, setPeriod).

#if  defined(__arm__) && defined(__STM32F1__)
  //clase derivada
  class TimerCounter:public HardwareTimer
    {
    public:
      TimerCounter(uint8 timerNum) : HardwareTimer(timerNum) {};
      void setSTM32Period(unsigned long microseconds) __attribute__((always_inline)) {
      
/*    if (microseconds < 65536/36) {
      this->setPrescaleFactor(F_CPU/1000000/36);
      this->setOverflow(microseconds*36);
    }else
    if (microseconds < 65536/18) {
      this->setPrescaleFactor(F_CPU/1000000/18);
      this->setOverflow(microseconds*18);
    }else */
      if (microseconds < 65536/24) {
        this->setPrescaleFactor(F_CPU/1000000/24);
        this->setOverflow(microseconds*24);
      }else    
/*    if (microseconds < 65536/16) {
      this->setPrescaleFactor(F_CPU/1000000/16);
      this->setOverflow(microseconds*16);
    }else */
        if (microseconds < 65536/8) {
          this->setPrescaleFactor(F_CPU/1000000/8);
          this->setOverflow(microseconds*8);
      }else
        if (microseconds < 65536/4) {
          this->setPrescaleFactor(F_CPU/1000000/4);
          this->setOverflow(microseconds*4);
      }else
        if (microseconds < 65536/2) {
          this->setPrescaleFactor(F_CPU/1000000/2);
          this->setOverflow(microseconds*2);
      }else
        if (microseconds < 65536) {
          this->setPrescaleFactor(F_CPU/1000000);
          this->setOverflow(microseconds);
      }else
        if (microseconds < 65536*2) {
          this->setPrescaleFactor(F_CPU/1000000*2);
          this->setOverflow(microseconds/2);
      }else
        if (microseconds < 65536*4) {
          this->setPrescaleFactor(F_CPU/1000000*4);
          this->setOverflow(microseconds/4);
      }else
        if (microseconds < 65536*8) {
          this->setPrescaleFactor(F_CPU/1000000*8);
          this->setOverflow(microseconds/8);
      }else
/*    if (microseconds < 65536*16) {
      this->setPrescaleFactor(F_CPU/1000000*16);
      this->setOverflow(microseconds/16);
    }else
    if (microseconds < 65536*32) {
      this->setPrescaleFactor(F_CPU/1000000*32);
      this->setOverflow(microseconds/32);
    }else */
        if (microseconds < 65536*64) {
          this->setPrescaleFactor(F_CPU/1000000*64);
          this->setOverflow(microseconds/64);
      }else
/*    if (microseconds < 65536*128) {
      this->setPrescaleFactor(F_CPU/1000000*128);
      this->setOverflow(microseconds/128);
    }else
    if (microseconds < 65536*256) {
      this->setPrescaleFactor(F_CPU/1000000*256);
      this->setOverflow(microseconds/256);
    }else */
        if (microseconds < 65536*512) {                                    
          this->setPrescaleFactor(F_CPU/1000000*512);
          this->setOverflow(microseconds/512);
        } else {                           
          this->setPrescaleFactor(F_CPU/1000000*512);
          this->setOverflow(65535);      
        }
        this->refresh();
    }
  };
#endif
