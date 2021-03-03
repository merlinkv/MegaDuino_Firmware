
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

 void menuMode()
 { 
  byte menuItem=0;
  byte subItem=0;
  byte updateScreen=true;
  
  while(digitalRead(btnStop)==HIGH || lastbtn)
  {
    if(updateScreen) {
      #ifdef OLED1306
        setXY(0,0); sendStr((unsigned char *)"Menu            ");
        setXY(0,2); sendStr((unsigned char *)"----------------");
        switch(menuItem) {
        case 0:
          setXY(0,1); sendStr((unsigned char *)"Baud Rate?      ");
        break;
        case 1:
          setXY(0,1); sendStr((unsigned char *)"Motor Ctrl?     ");
        break;        
        case 2:
          setXY(0,1); sendStr((unsigned char *)"TSXCzxpUEF?     ");
        break;
        case 3:
          setXY(0,1); sendStr((unsigned char *)"SkipBLK 2A?     ");
        break;        
        }
      #endif
//        printtextF(PSTR("Menu"),0); - Ejemplo version original      
      #ifdef LCD16
        lcd.setCursor(0,1); lcd.print("                ");
        switch(menuItem) {
        case 0:
          lcd.setCursor(0,0); lcd.print("Baud Rate?    ");
        break;
        case 1:
          lcd.setCursor(0,0); lcd.print("Motor Ctrl?   ");
        break;        
        case 2:
          lcd.setCursor(0,0); lcd.print("TSXCzxpUEFSW? ");        
        break;
        case 3:
          lcd.setCursor(0,0); lcd.print("Skip BLK:2A?  ");        
        break;
        }
      #endif
      #ifdef LCD20
        lcd.setCursor(0,1); lcd.print("                    ");
        switch(menuItem) {
        case 0:
          lcd.setCursor(0,0); lcd.print("Baud Rate?          ");
        break;
        case 1:
          lcd.setCursor(0,0); lcd.print("Motor Ctrl?         ");
        break;        
        case 2:
          lcd.setCursor(0,0); lcd.print("TSXCzxpUEFSW?       ");        
        break;
        case 3:
          lcd.setCursor(0,0); lcd.print("Skip BLK:2A?        ");        
        break;
        }
      #endif
      updateScreen=false;
      }
    
    if(digitalRead(btnDown)==LOW && !lastbtn){
      #ifdef MenuBLK2A
        if(menuItem<3) menuItem+=1;
      #endif
      #ifndef MenuBLK2A
        if(menuItem<2) menuItem+=1;      
      #endif
      lastbtn=true;
      updateScreen=true;
    }
    if(digitalRead(btnUp)==LOW && !lastbtn) {
      if(menuItem>0) menuItem+=-1;
      lastbtn=true;
      updateScreen=true;
    }
    if(digitalRead(btnPlay)==LOW && !lastbtn) {
      switch(menuItem){
        case 0:
          subItem=0;
          updateScreen=true;
          lastbtn=true;
          while(digitalRead(btnStop)==HIGH || lastbtn) {
            if(updateScreen) {
//              printtextF(PSTR("Baud Rate"),0);
              #ifdef OLED1306
                setXY(0,2);
                sendStr((unsigned char *)"----------------");
                switch(subItem) {
                case 0:
                  setXY(0,1);
                  sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1);
                  sendStr((unsigned char *)"1200 ");
                  if(BAUDRATE==1200) {
                    setXY(11,1);
                    sendStr((unsigned char *)"1200*");
                  }
                break;
                case 1:
                  setXY(0,1);
                  sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1);
                  sendStr((unsigned char *)"2400 ");
                  if(BAUDRATE==2400) {
                    setXY(11,1);
                    sendStr((unsigned char *)"2400*");
                  }
                break;
                case 2:
                  setXY(0,1);
                  sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1);
                  sendStr((unsigned char *)"3600 ");
                  if(BAUDRATE==3600) {
                    setXY(11,1);
                    sendStr((unsigned char *)"3600*");
                  }
                break;
                case 3:
                  setXY(0,1);
                  sendStr((unsigned char *)"Baud Rate:  ");
                  setXY(11,1);
                  sendStr((unsigned char *)"3850 ");
                  if(BAUDRATE==3850) {
                    setXY(11,1);
                    sendStr((unsigned char *)"3850*");
                  }
                break;
              }
              #else
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
              #endif
              updateScreen=false;
            }
                    
            if(digitalRead(btnDown)==LOW && !lastbtn){
              if(subItem<3) subItem+=1;
              lastbtn=true;
              updateScreen=true;
            }
            if(digitalRead(btnUp)==LOW && !lastbtn) {
              if(subItem>0) subItem+=-1;
              lastbtn=true;
              updateScreen=true;
            }
            if(digitalRead(btnPlay)==LOW && !lastbtn) {
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
              #ifdef LCD20
                LCDBStatusLine();
              #endif
              #ifdef OLED1306 
                OledStatusLine();
              #endif
              lastbtn=true;
            }
            checkLastButton();
          }
          lastbtn=true;
          updateScreen=true;
        break;

        case 1:
          subItem=0;
          updateScreen=true;
          lastbtn=true;
          while(digitalRead(btnStop)==HIGH || lastbtn) {
            if(updateScreen) {
              #ifdef OLED1306
                setXY(0,1);
                sendStr((unsigned char *)"Motor Ctrl:  ");
              #else
                printtextF(PSTR("Motor Ctrl"),0);
              #endif
              if(mselectMask==0) {
                #ifdef OLED1306
                  setXY(12,1);  
                  sendStr((unsigned char *)"OFF*");
                #else                
                  printtextF(PSTR("OFF *"),lineaxy);
                #endif
              }
              if(mselectMask==1) {
                #ifdef OLED1306
                  setXY(12,1);  
                  sendStr((unsigned char *)"ON* ");
                #else                
                  printtextF(PSTR("ON *"),lineaxy);
                #endif
              }              
              updateScreen=false;
            }
            if(digitalRead(btnPlay)==LOW && !lastbtn) {
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

        case 2:
          subItem=0;
          updateScreen=true;
          lastbtn=true;
          while(digitalRead(btnStop)==HIGH || lastbtn) {
            if(updateScreen) {
              #ifdef OLED1306
                setXY(0,1);
                sendStr((unsigned char *)"TSXCzxpSW:   ");
              #else
                printtextF(PSTR("TSXCzxpolUEFSW"),0);
              #endif
              if(TSXCONTROLzxpolarityUEFSWITCHPARITY==0) {
                #ifdef OLED1306
                  setXY(12,1);  
                  sendStr((unsigned char *)"OFF*");
                #else                
                  printtextF(PSTR("OFF *"),lineaxy);
                #endif
              }
              if(TSXCONTROLzxpolarityUEFSWITCHPARITY==1) {
                #ifdef OLED1306
                  setXY(12,1);  
                  sendStr((unsigned char *)"ON* ");
                #else                
                  printtextF(PSTR("ON *"),lineaxy);
                #endif
              }              
              updateScreen=false;
            }
            if(digitalRead(btnPlay)==LOW && !lastbtn) {
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
        case 3:
          subItem=0;
          updateScreen=true;
          lastbtn=true;
          while(digitalRead(btnStop)==HIGH || lastbtn) {
            if(updateScreen) {
              #ifdef OLED1306
                setXY(0,1);
                sendStr((unsigned char *)"SkipBLK 2A:  ");
              #else
                printtextF(PSTR("Skip BLK:2A"),0);
              #endif
              if(skip2A==0) {
                #ifdef OLED1306
                  setXY(12,1);  
                  sendStr((unsigned char *)"OFF*");
                #else                
                  printtextF(PSTR("OFF *"),lineaxy);
                #endif
              }
              if(skip2A==1) {
                #ifdef OLED1306
                  setXY(12,1);  
                  sendStr((unsigned char *)"ON* ");
                #else                
                  printtextF(PSTR("ON *"),lineaxy);
                #endif
              }              
              updateScreen=false;
            }
            if(digitalRead(btnPlay)==LOW && !lastbtn) {
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
        break;
    #endif
      }
    }
    checkLastButton();
  }
  updateEEPROM();
  #ifdef OLED1306
    setXY(0,2); sendStr((unsigned char *)"----------------");    
  #endif
//  #ifdef LCD20
//    LCDBStatusLine();  
//  #endif   
  debounce(btnStop);
 }

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

  if(mselectMask) settings |=128;
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


  if(bitRead(settings,7)) {
    mselectMask=1;
  } else {
    mselectMask=0;
  }
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
//  setBaud();
  UniSetup();
 
 }

void checkLastButton()
{
  if(digitalRead(btnDown) && digitalRead(btnUp) && digitalRead(btnPlay) && digitalRead(btnStop)) lastbtn=false; 
        //    setXY(0,0);
        //  sendChar(lastbtn+'0');
  delay(50);
}
