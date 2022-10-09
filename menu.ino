#include "buttons.h"

#if defined(__arm__) && defined(__STM32F1__)

  uint8_t EEPROM_get(uint16_t address, byte *data) {
    if (EEPROM.init()==EEPROM_OK) {
      *data = (byte)(EEPROM.read(address) & 0xff);  
      return true;  
    } else 
      return false;
  }
  

  uint8_t EEPROM_put(uint16_t address, byte data) {
    if (EEPROM.init()==EEPROM_OK) {
      EEPROM.write(address, (uint16_t) data); 
      return true;    
    } else
      return false;
  }
  #endif

enum MenuItems{
  BAUD_RATE,
#ifndef NO_MOTOR
  MOTOR_CTL,
#endif
  TSX_POL_UEFSW,
#ifdef MenuBLK2A
  BLK2A,
#endif
  _Num_Menu_Items
};

 void menuMode()
 { 
  byte menuItem=0;
  byte subItem=0;
  byte updateScreen=true;
  
  while(!button_stop() || lastbtn)
  {
    if(updateScreen) {
      #ifdef OLED1306
        setXY(0,0); sendStr((unsigned char *)"Menu            ");
        setXY(0,2); sendStr((unsigned char *)"----------------");
        switch(menuItem) {
        case MenuItems::BAUD_RATE:
          setXY(0,1); sendStr((unsigned char *)"Baud Rate?      ");
        break;
        #ifndef NO_MOTOR
          case MenuItems::MOTOR_CTL:
            setXY(0,1); sendStr((unsigned char *)"Motor Ctrl?     ");
          break;
        #endif        
          case MenuItems::TSX_POL_UEFSW:
            setXY(0,1); sendStr((unsigned char *)"TSXCzxpSW?      ");
          break;
        #ifdef MenuBLK2A
          case MenuItems::BLK2A:
            setXY(0,1); sendStr((unsigned char *)"SkipBLK 2A?     ");
          break;
        #endif        
        }
      #elif defined(LCD16)
        lcd.setCursor(0,1); lcd.print("                ");
        switch(menuItem) {
          case MenuItems::BAUD_RATE:        
            lcd.setCursor(0,0); lcd.print("Baud Rate?    ");
          break;
        #ifndef NO_MOTOR
          case MenuItems::MOTOR_CTL:
            lcd.setCursor(0,0); lcd.print("Motor Ctrl?   ");
          break; 
        #endif       
          case MenuItems::TSX_POL_UEFSW:
            lcd.setCursor(0,0); lcd.print("TSXCzxpUEFSW? ");        
          break;
        #ifdef MenuBLK2A          
          case MenuItems::BLK2A:
            lcd.setCursor(0,0); lcd.print("Skip BLK:2A?  ");        
          break;
        #endif
        }
      #elif defined(LCD20)
        lcd.setCursor(0,1); lcd.print("--------------------");
        switch(menuItem) {
          case MenuItems::BAUD_RATE:        
            lcd.setCursor(0,0); lcd.print("Baud Rate?          ");
          break;
        #ifndef NO_MOTOR
          case MenuItems::MOTOR_CTL:
            lcd.setCursor(0,0); lcd.print("Motor Ctrl?         ");
          break; 
        #endif       
          case MenuItems::TSX_POL_UEFSW:
            lcd.setCursor(0,0); lcd.print("TSXCzxpUEFSW?       ");        
          break;
        #ifdef MenuBLK2A          
          case MenuItems::BLK2A:
            lcd.setCursor(0,0); lcd.print("Skip BLK:2A?        ");        
          break;
        #endif
        }
      #endif
      updateScreen=false;
    }
    if(button_down() && !lastbtn){
      if(menuItem<MenuItems::_Num_Menu_Items-1) menuItem+=1;
      lastbtn=true;
      updateScreen=true;
    }
    if(button_up() && !lastbtn) {
      if(menuItem>0) menuItem+=-1;
      lastbtn=true;
      updateScreen=true;
    }
    if(button_play() && !lastbtn) {
      switch(menuItem){
        case MenuItems::BAUD_RATE:
//          subItem=0;
          updateScreen=true;
          lastbtn=true;
          while(!button_stop() || lastbtn) {
            if(updateScreen) {
//              printtextF(PSTR("Baud Rate"),0);
              #ifdef OLED1306
                setXY(0,2); sendStr((unsigned char *)"----------------");
                switch(subItem) {
                case 0:
                  setXY(0,1); sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1); sendStr((unsigned char *)"1200 ");
                  if(BAUDRATE==1200) {
                    setXY(11,1); sendStr((unsigned char *)"1200*");
                  }
                break;
                case 1:
                  setXY(0,1); sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1); sendStr((unsigned char *)"2400 ");
                  if(BAUDRATE==2400) {
                    setXY(11,1); sendStr((unsigned char *)"2400*");
                  }
                break;
                case 2:
                  setXY(0,1); sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1); sendStr((unsigned char *)"3600 ");
                  if(BAUDRATE==3600) {
                    setXY(11,1); sendStr((unsigned char *)"3600*");
                  }
                break;
                case 3:
                  setXY(0,1); sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1); sendStr((unsigned char *)"3850 ");
                  if(BAUDRATE==3850) {
                    setXY(11,1); sendStr((unsigned char *)"3850*");
                  }
                break;
              }
            #elif defined(LCD16)
              printtextF(PSTR("Baud Rate"),0);
              switch(subItem) {
                case 0:                                  
                  printtextF(PSTR("1200"),lineaxy);
                  if(BAUDRATE==1200) printtextF(PSTR("1200 *"),lineaxy);
                break;
                case 1:        
                  printtextF(PSTR("2400"),lineaxy);
                  if(BAUDRATE==2400) printtextF(PSTR("2400 *"),lineaxy);
                break;
                case 2:                  
                  printtextF(PSTR("3600"),lineaxy);
                  if(BAUDRATE==3600) printtextF(PSTR("3600 *"),lineaxy);
                break;                  
                case 3:                  
                  printtextF(PSTR("3850"),lineaxy);
                  if(BAUDRATE==3850) printtextF(PSTR("3850 *"),lineaxy);
                break;                
              }
            #elif defined(LCD20)
              printtextF(PSTR("Baud Rate:          "),0);
              printtextF(PSTR("--------------------"),1);              
              switch(subItem) {
                case 0:
                  printtextF(PSTR("Baud Rate:     1200 "),0);
                  if(BAUDRATE==1200) printtextF(PSTR("Baud Rate:     1200*"),0);
                break;
                case 1:
                  printtextF(PSTR("Baud Rate:     2400 "),0);
                  if(BAUDRATE==2400) printtextF(PSTR("Baud Rate:     2400*"),0);
                break;
                case 2:
                  printtextF(PSTR("Baud Rate:     3600 "),0);
                  if(BAUDRATE==3600) printtextF(PSTR("Baud Rate:     3600*"),0);
                break;
                case 3:
                  printtextF(PSTR("Baud Rate:     3850 "),0);
                  if(BAUDRATE==3850) printtextF(PSTR("Baud Rate:     3850*"),0);
                break;
              }
            #endif
            updateScreen=false;
            }
                    
            if(button_down() && !lastbtn){
              if(subItem<3) subItem+=1;
              lastbtn=true;
              updateScreen=true;
            }
            if(button_up() && !lastbtn) {
              if(subItem>0) subItem+=-1;
              lastbtn=true;
              updateScreen=true;
            }
            if(button_play() && !lastbtn) {
              switch(subItem) {
                case 0:
                  BAUDRATE=1200;
                break;
                case 1:
                  BAUDRATE=2400;
                break;
                case 2:
                  BAUDRATE=3600;
                break;                
                case 3:
                  BAUDRATE=3850;
                break;
              }
              updateScreen=true;
              #if defined(OLED1306) && defined(OSTATUSLINE) 
                OledStatusLine();
              #endif
              #ifdef LCD20
                LCDBStatusLine();
              #endif              
              lastbtn=true;
            }
            checkLastButton();
          }
          lastbtn=true;
          updateScreen=true;
        break;

   #ifndef NO_MOTOR
     case MenuItems::MOTOR_CTL:
       subItem=0;
       updateScreen=true;
       lastbtn=true;
       while(!button_stop() || lastbtn) {
         if(updateScreen) {
           #ifdef OLED1306
             setXY(0,1); sendStr((unsigned char *)"Motor Ctrl:  ");
           #elif defined(LCD16)
             printtextF(PSTR("Motor Ctrl:"),0);
           #elif defined(LCD20)
              printtextF(PSTR("Motor Ctrl:         "),0);
              printtextF(PSTR("--------------------"),1);                    
           #endif
           if(mselectMask==0) {
             #ifdef OLED1306
               setXY(12,1); sendStr((unsigned char *)"OFF*");
             #elif defined(LCD16)
               printtextF(PSTR("OFF *"),lineaxy);
             #elif defined(LCD20)
              printtextF(PSTR("Motor Ctrl:     OFF*"),0);               
             #endif
           }
           if(mselectMask==1) {
             #ifdef OLED1306
               setXY(12,1); sendStr((unsigned char *)"ON*");
             #elif defined(LCD16)
               printtextF(PSTR("OFF *"),lineaxy);
             #elif defined(LCD20)
               printtextF(PSTR("Motor Ctrl:      ON*"),0);               
             #endif
           }              
           updateScreen=false;
         }
         if(button_play() && !lastbtn) {
           mselectMask= !mselectMask;
           lastbtn=true;
           updateScreen=true;
           #ifdef LCD20
             LCDBStatusLine();
           #endif              
           #ifdef OLED1306 
             OledStatusLine();
           #endif              
         }
         checkLastButton();
       }
       lastbtn=true;
       updateScreen=true;
     break;
   #endif

     case MenuItems::TSX_POL_UEFSW:
       subItem=0;
       updateScreen=true;
       lastbtn=true;
       while(!button_stop() || lastbtn) {
         if(updateScreen) {
           #ifdef OLED1306
             setXY(0,1); sendStr((unsigned char *)"TSXCzxpSW:   ");
           #elif defined(LCD16)
             printtextF(PSTR("TSXCzxpolUEFSW"),0);
           #elif defined(LCD20)
              printtextF(PSTR("TZXCzxpUEFSW:       "),0);
              printtextF(PSTR("--------------------"),1);            
           #endif
           if(TSXCONTROLzxpolarityUEFSWITCHPARITY==0) {
             #ifdef OLED1306
               setXY(12,1); sendStr((unsigned char *)"OFF*");
             #elif defined(LCD16)             
               printtextF(PSTR("OFF *"),lineaxy);
             #elif defined(LCD20)
               printtextF(PSTR("TZXCzxpUEFSW:   OFF*"),0);                      
             #endif
           }
           if(TSXCONTROLzxpolarityUEFSWITCHPARITY==1) {
             #ifdef OLED1306
               setXY(12,1); sendStr((unsigned char *)"ON* ");
             #elif defined(LCD16)      
               printtextF(PSTR("ON *"),lineaxy);
             #elif defined(LCD20)
               printtextF(PSTR("TZXCzxpUEFSW:    ON*"),0);             
             #endif
           }              
           updateScreen=false;
         }
         if(button_play() && !lastbtn) {
           TSXCONTROLzxpolarityUEFSWITCHPARITY = !TSXCONTROLzxpolarityUEFSWITCHPARITY;
           lastbtn=true;
           updateScreen=true;
           #ifdef LCD20
             LCDBStatusLine();
           #endif              
           #ifdef OLED1306 
            OledStatusLine();
           #endif
         }
         checkLastButton();
       }
       lastbtn=true;
       updateScreen=true;
     break;
          
   #ifdef MenuBLK2A
     case MenuItems::BLK2A:
       subItem=0;
       updateScreen=true;
       lastbtn=true;
       while(!button_stop() || lastbtn) {
         if(updateScreen) {
           #ifdef OLED1306
             setXY(0,1); sendStr((unsigned char *)"SkipBLK 2A:  ");
           #elif defined(LCD16)
             printtextF(PSTR("Skip BLK:2A"),0);
           #elif defined(LCD20)
              printtextF(PSTR("SkipBLK 2A:         "),0);
              printtextF(PSTR("--------------------"),1);               
           #endif
           if(skip2A==0) {
             #ifdef OLED1306
               setXY(12,1); sendStr((unsigned char *)"OFF*");
             #elif defined(LCD16)                
               printtextF(PSTR("OFF *"),lineaxy);
             #elif defined(LCD20)
               printtextF(PSTR("SkipBLK 2A:     OFF*"),0);
             #endif
           }
           if(skip2A==1) {
             #ifdef OLED1306
               setXY(12,1); sendStr((unsigned char *)"ON* ");
             #elif defined(LCD16)               
               printtextF(PSTR("ON *"),lineaxy);
             #elif defined(LCD20)
               printtextF(PSTR("SkipBLK 2A:      ON*"),0);             
             #endif
           }              
           updateScreen=false;
         }
         if(button_play() && !lastbtn) {
           skip2A = !skip2A;
           lastbtn=true;
           updateScreen=true;
           #ifdef LCD20
             LCDBStatusLine();
           #endif              
           #ifdef OLED1306 
             OledStatusLine();
           #endif
         } 
         checkLastButton();
       }
       lastbtn=true;
       updateScreen=true;
     #endif     
    }
  }
  checkLastButton();
  }
  #ifdef LOAD_EEPROM_SETTINGS
    updateEEPROM();
  #endif

  debounce(button_stop);
 }

#ifdef LOAD_EEPROM_SETTINGS
 void updateEEPROM()
 {
  /* Setting Byte: 
   *  bit 0: 1200
   *  bit 1: 2400
   *  bit 2: 3600
   *  bit 3: 3850
   *  bit 4: n/a
   *  bit 5: BLK_2A control
   *  bit 6: TSXCONTROLzxpolarityUEFSWITCHPARITY
   *  bit 7: Motor control
   */
  byte settings=0;

  switch(BAUDRATE) {
    case 1200:
    settings |=1;
    break;
    case 2400:
    settings |=2;
    break;
    case 3600:
    settings |=4;  
    break;      
    case 3850:
    settings |=8;
    break;
  }

  #ifndef NO_MOTOR
  if(mselectMask) settings |=128;
  #endif

  if(TSXCONTROLzxpolarityUEFSWITCHPARITY) settings |=64;
  
  #ifdef MenuBLK2A
    if(skip2A) settings |=32;
  #endif

  #if defined(__AVR__)
    EEPROM.put(EEPROM_CONFIG_BYTEPOS,settings);
  #elif defined(__arm__) && defined(__STM32F1__)
    EEPROM_put(EEPROM_CONFIG_BYTEPOS,settings);
  #endif      
  setBaud();
 }

 void loadEEPROM()
 {
  byte settings=0;
  #if defined(__AVR__)
    EEPROM.get(EEPROM_CONFIG_BYTEPOS,settings);
  #elif defined(__arm__) && defined(__STM32F1__)
    EEPROM_get(EEPROM_CONFIG_BYTEPOS,&settings);
  #endif
      
  if(!settings) return;
  
  #ifndef NO_MOTOR
  if(bitRead(settings,7)) {
    mselectMask=1;
  } else {
    mselectMask=0;
  }
  #endif

  if(bitRead(settings,6)) {
    TSXCONTROLzxpolarityUEFSWITCHPARITY=1;
  } else {
    TSXCONTROLzxpolarityUEFSWITCHPARITY=0;
  }
  
  #ifdef MenuBLK2A
  if(bitRead(settings,5)) {
    skip2A=1;
  } else {
    skip2A=0;
  }   
  #endif
  
  if(bitRead(settings,0)) {
    BAUDRATE=1200;
  }
  if(bitRead(settings,1)) {
    BAUDRATE=2400;
  }
  if(bitRead(settings,2)) {
    BAUDRATE=3600;  
  }
  if(bitRead(settings,3)) {
    BAUDRATE=3850;  
  }
 }
#endif

void checkLastButton()
{
  if(!button_down() && !button_up() && !button_play() && !button_stop()) lastbtn=false; 
        //    setXY(0,0);
        //  sendChar(lastbtn+'0');
  delay(50);
}
