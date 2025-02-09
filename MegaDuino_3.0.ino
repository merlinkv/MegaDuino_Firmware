#define VERSION "MegaDuino 3.0"

//
// Original firmware writen by Andrew Beer, Duncan Edwards, Rafael Molina and others.
//
// MegaDuino Firmware is an adaptation of the MaxDuino firmware by Rafael Molina (http://github.com/rcmolina/maxduino)
// but is specially designed for my MegaDuino project.
// 
// Displays that can be used: LCD 16x2, LCD 20x4, OLED 128x32 & OLED 128x64
//
//Version 3.0 - February 8, 2025
//   Based on the latest OTLA version from rcmolina (https://github.com/rcmolina/MaxDuino/tree/master)
// Version 2.6 - October 2, 2024
//   Added New Logos
// Version 2.5 - May 1, 2023
//   Fixed some bugs on LCD & OLED displays
//   Some works done on File Types & Block IDS
//   Some works done on Menus
// Version 2.0 - September 20, 2022
//    Some bugs fixed when playing Amstrad CPC, Oric, ZX81 & MSX files
//    Better performance on I2C LCD & I2C OLED displays
//    Loading speed improved on Spectrum TZX turbo files, reaching speeds up to 4.500 bds
//    Possibility to select different 8x8 fonts to use with OLED screens
// Version 1.6 - October 5, 2021
//   Some works done to enhance the block rewind & forwarding and on 4B block types
// Version 1.5 - September 11, 2021
//    Some optimizations for Arcon, Oric, Dragon & Camputerx Lynx
//    Corrected block counter for TSX files (MSX)
// Version 1.4 - August 18, 2021
//    Fixed some bugs on ID19 & ID21 Block Types
// Version 1.3 - February 27, 2021
//    Fixed some bugs with MSX CAS files
//    Timer optimized
// Version 1.2 - February 23, 2021
//    Fixed some bugs on LCD screens
// Version 1.2 - January 18, 2021
//    Performed some code debugging tasks
// Version 1.1 - January 14, 2021
//    Added logos & some bug fixes displaying ID Blocks
// Version 1.0 - January 8, 2021
//    Abandoned MaxDuino M versions & started job of MegaDuino Firmware

#include "product_strings.h"
#include "preferences.h"
#include "configs.h"
#include "current_settings.h"
#include "Display.h"
#include "MegaDuino.h"
#include "hwconfig.h"
#include "buttons.h"
#include "sdfat_config.h"
#include <SdFat.h>
#include "TimerCounter.h"
#include "menu.h"
#include "constants.h"
#include "file_utils.h"
#include "buffer.h"
#include "MegDProcessing.h"
#include "CounterPercent.h"
#include "processing_state.h"
#include "casProcessing.h"
#include "pinSetup.h"
#include "USBStorage.h"
#include "power.h"

#ifdef BLOCK_EEPROM_PUT
#include "EEPROM_wrappers.h"
#endif

SdFat sd;                           //Initialise Sd card 
SdBaseFile _tmpdirs[2]; // internal file pointers.  (*currentDir points to either _tmpdirs[0] or _tmpdirs[1] and the other is 'scratch')
SdBaseFile *currentDir = &_tmpdirs[0];  // SD card directory
byte _alt_tmp_dir = 1; // which of the _tmpdirs is scratch (we flip this between 0 and 1)

#ifdef FREERAM
  #define filenameLength 160
  #define nMaxPrevSubDirs 7  
#else 
  #define filenameLength 255
  #define nMaxPrevSubDirs 10  
#endif

char fileName[filenameLength + 1];    //Current filename
char prevSubDir[SCREENSIZE+1];
uint16_t DirFilePos[nMaxPrevSubDirs];  //File Positions in Directory to be restored (also, history of subdirectories)
byte subdir = 0;

#ifndef NO_MOTOR
byte motorState = 1;                //Current motor control state
byte oldMotorState = 1;             //Last motor control state
#endif

byte start = 0;                     //Currently playing flag

bool pauseOn = false;                   //Pause state
uint16_t currentFile;               //File index (per filesystem) of current file, relative to current directory (pointed to by currentDir)
uint16_t maxFile;                   //Total number of files in directory
bool dirEmpty;                      //flag if directory is completely empty
uint16_t oldMinFile = 0;
uint16_t oldMaxFile = 0;

#ifdef SHOW_DIRNAMES
  #define fnameLength  5
  char oldMinFileName[fnameLength];
  char oldMaxFileName[fnameLength];
#endif

bool isDir;                         //Is the current file a directory
unsigned long timeDiff = 0;         //button debounce

#if (SPLASH_SCREEN && TIMEOUT_RESET)
    unsigned long timeDiff_reset = 0;
    byte timeout_reset = TIMEOUT_RESET;
#endif

char PlayBytes[17];

#ifdef BLOCKID_INTO_MEM 
unsigned long blockOffset[maxblock];
byte blockID[maxblock];
#endif

#ifdef BLKBIGSIZE
  word block = 0;
#else
  byte block = 0;
#endif

byte jblks = 1;
byte oldMinBlock = 0;
#ifdef BLOCK_EEPROM_PUT
  byte oldMaxBlock = 99;
#else
  byte oldMaxBlock = 19;
#endif

bool firstBlockPause = false;

#if (SPLASH_SCREEN && TIMEOUT_RESET)
    void(* resetFunc) (void) = 0;//declare reset function at adress 0
    /*void resetFunc() // Restarts program from beginning but does not reset the peripherals and registers
    {
    asm volatile ("  jmp 0");
    }*/
#endif

void setup() {
  pinsetup();
  pinMode(chipSelect, OUTPUT);      //Setup SD card chipselect pin

  #ifdef LCD16
    lcd.init();                     //Initialise LCD (16x2 type)
    lcd.backlight();
    lcd.clear();
    #if (SPLASH_SCREEN)
      lcd.setCursor(0,0);
      lcd.print(reinterpret_cast <const __FlashStringHelper *>P_PRODUCT_NAME); // Set the text at the initialization for LCD Screen (Line 1)
      lcd.setCursor(0,1); 
      lcd.print(reinterpret_cast <const __FlashStringHelper *>P_VERSION); // Set the text at the initialization for LCD Screen (Line 2)
    #endif   
  #endif

  #ifdef LCD20
    lcd.init();                     //Initialise LCD (20x4 type)
    lcd.backlight();
    lcd.clear();
    #if (SPLASH_SCREEN)
      lcd.setCursor(0,0);
      lcd.print(reinterpret_cast <const __FlashStringHelper *>P_PRODUCT_NAME); // Set the text at the initialization for LCD Screen (Line 1)
      lcd.setCursor(0,1); 
      lcd.print(reinterpret_cast <const __FlashStringHelper *>P_VERSION); // Set the text at the initialization for LCD Screen (Line 2)
    #endif   
  #endif
    
  #ifdef OLED1306
    init_OLED();
    #if (!SPLASH_SCREEN)
      #if defined(LOAD_MEM_LOGO) || defined(LOAD_EEPROM_LOGO)
        delay(2500);             // Show logo
      #endif
      reset_display();           // Clear logo and load saved mode
    #endif
  #endif

  setup_buttons();
 
  #ifdef SPLASH_SCREEN
    while (!button_any()){
      delay(100);                // Show logo (OLED) or text (LCD) and remains until a button is pressed.
    }   
    #ifdef OLED1306    
      reset_display();           // Clear logo and load saved mode

      printtextF(P_PRODUCT_NAME, 0);
      printtextF(P_VERSION, lineaxy);
      while (button_any()){
        delay(100);              // Show version while button is still pressed (let go to continue)
      }   
      reset_display();           // Clear screen
    #endif
  #endif

  while (!sd.begin(chipSelect, SD_SPI_CLOCK_SPEED)) {
    //Start SD card and check it's working
    //printtextF(PSTR("No SD Card"),0);
    NoSDCard();
    delay(1500);
    #ifdef SOFT_POWER_OFF
    check_power_off_key();
    #endif
  }    

  #ifdef USB_STORAGE_ENABLED
  usb_detach();
  delay(1500);
  usb_retach();
  setup_usb_storage();
  #endif

  changeDirRoot();
  UniSetup();                       //Setup TZX specific options
  SDCardOK();
  delay(1500);
  #if defined(LCD16) || defined(LCD20)
    lcd.clear();
  #endif
  #ifdef OLED1306    
    reset_display();
    Banner();
    delay(1500);
    reset_display();
  #endif    
  printtextF(PSTR("Reset..."),0);
  delay(1500);
  
  #if defined(LCD16) || defined(LCD20)
    lcd.clear();
  #endif

  getMaxFile();                     //get the total number of files in the directory

  seekFile();            //move to the first file in the directory

  #ifdef LOAD_EEPROM_SETTINGS
    loadEEPROM();
  #endif  

  #if defined(OLED1306) && defined(OSTATUSLINE)
    OledStatusLine();
  #elif defined(LCD20) && defined(OSTATUSLINE)
    LCDStatusLine();
  #endif
}

extern unsigned long soft_poweroff_timer;

void loop(void) {
  if(start==1)
  {
    //TZXLoop only runs if a file is playing, and keeps the buffer full.
    UniLoop();
  } else {
    WRITE_LOW;    
  }
  
  if(start==0 && (strlen(fileName)> SCREENSIZE)) {
    //Filename scrolling only runs if no file is playing to prevent I2C writes 
    //conflicting with the playback Interrupt
    scrollText(fileName, isDir);
  }

  #ifndef NO_MOTOR
    motorState=digitalRead(btnMotor);
  #endif
  
  #if (SPLASH_SCREEN && TIMEOUT_RESET)
    if (millis() - timeDiff_reset > 1000) //check timeout reset every second
    {
      timeDiff_reset = millis(); // get current millisecond count
      if (start==0)
      {
        timeout_reset--;
        if (timeout_reset==0)
        {
          timeout_reset = TIMEOUT_RESET;
          resetFunc();
        }
      }
      else
      {
        timeout_reset = TIMEOUT_RESET;
      }    
    }
  #endif
    
  if (millis() - timeDiff > 50) {   // check switch every 50ms 
    timeDiff = millis();           // get current millisecond count

  #ifdef SOFT_POWER_OFF
    if(start==0)
    {
      check_power_off_key();
    }
    else
    {
      clear_power_off();
    }
  #endif

    if(button_play()) {
      //Handle Play/Pause button
      if(start==0) {
        //If no file is play, start playback
        playFile();
        #ifndef NO_MOTOR
        if (mselectMask){  
          //Start in pause if Motor Control is selected
          oldMotorState = 0;
        }
        #endif
        delay(50);
        
      } else {
        //If a file is playing, pause or unpause the file                  
        if (!pauseOn) {
          printtext2F(PSTR("Paused  "),0);
          jblks =1; 
          firstBlockPause = true;
        } else  {
          printtext2F(PSTR("Playing      "),0);
          currpct=100;
          firstBlockPause = false;      
        }
        pauseOn = !pauseOn;
      }
      
      debounce(button_play);
    }

  #ifdef ONPAUSE_POLCHG
    if(button_root() && start==1 && pauseOn 
      #ifdef btnRoot_AS_PIVOT   
        && button_stop()
      #endif
      ){             // change polarity
      // change tsx speed control/zx polarity/uefTurboMode
      TSXCONTROLzxpolarityUEFSWITCHPARITY = !TSXCONTROLzxpolarityUEFSWITCHPARITY;
      #if defined(OLED1306) && defined(OSTATUSLINE) 
        OledStatusLine();
      #elif defined(LCD20) && defined(OSTATUSLINE)
        LCDStatusLine();        
      #endif 
      debounce(button_root);
    }
  #endif

  #ifdef btnRoot_AS_PIVOT
    lastbtn=false;     
    if(button_root() && start==0 && !lastbtn) {                                          // show min-max dir
      
      #ifdef SHOW_DIRPOS
        #if defined(LCD16)
          #if !defined(SHOW_STATUS_LCD) && !defined(SHOW_DIRNAMES)
            char len=0;
            lcd.setCursor(0,0); 
            lcd.print(utoa(oldMinFile,input,10));
            lcd.print('<');
            len += strlen(input) + 1;
            lcd.print(utoa(currentFile,input,10));
            lcd.print('<');
            len += strlen(input) + 1;
            lcd.print(utoa(oldMaxFile,input,10));
            len += strlen(input); 
            for(char x=len;x<16;x++) {
              lcd.print(' '); 
            }
          #elif defined(SHOW_STATUS_LCD)        
            lcd.setCursor(0,0);
            lcd.print(BAUDRATE);
            lcd.print(' ');
            if(mselectMask) lcd.print(F(" M:ON"));
            else lcd.print(F("M:OFF"));
            lcd.print(' ');
            if (TSXCONTROLzxpolarityUEFSWITCHPARITY) lcd.print(F(" %^ON"));
            else lcd.print(F("%^OFF"));         
          #elif defined(SHOW_DIRNAMES)
            str4cpy(input,fileName);
            GetFileName(oldMinFile);
            str4cpy(oldMinFileName,fileName);
            GetFileName(oldMaxFile);
            str4cpy(oldMaxFileName,fileName);
            GetFileName(currentFile); 
            lcd.setCursor(0,0);
            lcd.print(oldMinFileName);
            lcd.print(' ');
            lcd.print('<');
            lcd.print((char *)input);
            lcd.print('<');
            lcd.print(' ');
            lcd.print(oldMaxFileName);                  
          #endif
        #endif // defined(LCD16)

        #if defined(LCD20)
          #if !defined(SHOW_STATUS_LCD) && !defined(SHOW_DIRNAMES)
            char len=0;
            lcd.setCursor(0,0); 
            lcd.print(utoa(oldMinFile,input,10));
            lcd.print('<');
            len += strlen(input) + 1;
            lcd.print(utoa(currentFile,input,10));
            lcd.print('<');
            len += strlen(input) + 1;
            lcd.print(utoa(oldMaxFile,input,10));
            len += strlen(input); 
            for(char x=len;x<20;x++) {
              lcd.print(' '); 
            }
          #elif defined(SHOW_STATUS_LCD)        
            lcd.setCursor(0,0);
            lcd.print(BAUDRATE);
            lcd.print(' ');
            if(mselectMask) lcd.print(F(" M:ON"));
            else lcd.print(F("M:OFF"));
            lcd.print(' ');
            if (TSXCONTROLzxpolarityUEFSWITCHPARITY) lcd.print(F(" %^ON"));
            else lcd.print(F("%^OFF"));         
          #elif defined(SHOW_DIRNAMES)
            str4cpy(input,fileName);
            GetFileName(oldMinFile);
            str4cpy(oldMinFileName,fileName);
            GetFileName(oldMaxFile);
            str4cpy(oldMaxFileName,fileName);
            GetFileName(currentFile); 
            lcd.setCursor(0,0);
            lcd.print(oldMinFileName);
            lcd.print(' ');
            lcd.print('<');
            lcd.print((char *)input);
            lcd.print('<');
            lcd.print(' ');
            lcd.print(oldMaxFileName);                  
          #endif
        #endif // defined(LCD20)

        #if defined(OLED1306)
          #if !defined(SHOW_DIRNAMES)
            char len=0;
            setXY(0,0);
            sendStr(utoa(oldMinFile,input,10));
            sendChar('<');
            len += strlen(input) + 1;
            sendStr(utoa(currentFile,input,10));
            sendChar('<');
            len += strlen(input) + 1;
            sendStr(utoa(oldMaxFile,input,10));
            len += strlen(input);
            for(char x=len;x<16;x++) {
              sendChar(' ');
            }
          #elif defined(SHOW_DIRNAMES)
            str4cpy(input,fileName);
            GetFileName(oldMinFile); str4cpy(oldMinFileName,fileName);
            GetFileName(oldMaxFile); str4cpy(oldMaxFileName,fileName);
            GetFileName(currentFile); 
            
            setXY(0,0);
            sendStr(oldMinFileName);sendChar(' ');sendChar('<');
            sendStr((char *)input);sendChar('<');sendChar(' ');
            sendStr(oldMaxFileName);
              
          #endif
        #endif // defined(OLED1306)
      #endif // SHOW_DIRPOS
        
      while(button_root() && !lastbtn) {
        //prevent button repeats by waiting until the button is released.
        lastbtn = 1;
        checkLastButton();           
      }        
      printtext(PlayBytes,0);
    }
      
    #if defined(LCD16) && defined(SHOW_BLOCKPOS_LCD)
      if(button_root() && start==1 && pauseOn && !lastbtn) {                                          // show min-max block
        lcd.setCursor(11,0);
        if (TSXCONTROLzxpolarityUEFSWITCHPARITY) {
          lcd.print(F(" %^ON"));
        } else {
          lcd.print(F("%^OFF"));
        }
        while(button_root() && start==1 && !lastbtn) {
          //prevent button repeats by waiting until the button is released.
          lastbtn = 1;
          checkLastButton();           
        }
        lcd.setCursor(11,0);
        lcd.print(' ');
        lcd.print(' ');
        lcd.print(PlayBytes);        
      }
    #endif

    #if defined(LCD20) && defined(SHOW_BLOCKPOS_LCD)
      if(button_root() && start==1 && pauseOn && !lastbtn) {                                          // show min-max block
        lcd.setCursor(13,0);
        if (TSXCONTROLzxpolarityUEFSWITCHPARITY) {
          lcd.print(F(" %^ON"));
        } else {
          lcd.print(F("%^OFF"));
        }
        while(button_root() && start==1 && !lastbtn) {
          //prevent button repeats by waiting until the button is released.
          lastbtn = 1;
          checkLastButton();           
        }
        lcd.setCursor(11,0);
        lcd.print(' ');
        lcd.print(' ');
        lcd.print(PlayBytes);        
      }
    #endif

  #endif // btnRoot_AS_PIVOT

  if(button_root() && start==0
    #ifdef btnRoot_AS_PIVOT
      && button_stop()
    #endif        
    ){                   // go menu
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif

    #if defined(Use_MENU) && !defined(RECORD_EEPROM_LOGO)
      menuMode();
      printtext(PlayBytes,0);
      #if defined(LCD16) || defined (LCD20)
        printtextF(PSTR(""),1);
      #endif      
      #ifdef OLED1306
        printtextF(PSTR(""),lineaxy);
      #endif
      scrollText(fileName, isDir, 0);
    #elif defined(RECORD_EEPROM_LOGO)
      init_OLED();
      delay(1500);              // Show logo
      reset_display();           // Clear logo and load saved mode
      printtextF(PSTR("Reset..."),0);
      delay(500);
      strcpy_P(PlayBytes, P_PRODUCT_NAME);
      printtextF(P_PRODUCT_NAME, 0);
      #if defined(OLED1306) && defined(OSTATUSLINE)
        OledStatusLine();
      #elif defined(LCD20) && defined(OSTATUSLINE)
        LCDStatusLine();
      #endif
    #else             
      subdir=0;
      changeDirRoot();
      getMaxFile();
      seekFile();
    #endif         

    debounce(button_root);
  }

  if(button_stop() && start==1
                        #ifdef btnRoot_AS_PIVOT
                                && !button_root()
                        #endif
                              ){      

    stopFile();
    debounce(button_stop);
  }

  if(button_stop() && start==0 && subdir >0) {               // back subdir
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
        timeout_reset = TIMEOUT_RESET;
    #endif     
    changeDirParent();

    debounce(button_stop);   
  }
     
  #ifdef BLOCKMODE
    if(button_up() && start==1 && pauseOn
      #ifdef btnRoot_AS_PIVOT
        && !button_root()
      #endif
      ){             //  up block sequential search                                                                 
      firstBlockPause = false;
      #ifdef BLOCKID_INTO_MEM
        oldMinBlock = 0;
        oldMaxBlock = maxblock;
        if (block > 0) block--;
        else block = maxblock;      
      #endif
      #if defined(BLOCK_EEPROM_PUT)
        oldMinBlock = 0;
        oldMaxBlock = 99;
        if (block > 0) block--;
        else block = 99;
      #endif
      #if defined(BLOCKID_NOMEM_SEARCH)
        if (block > jblks) block=block-jblks;
        else block = 0;
      #endif        

      GetAndPlayBlock();       
      debouncemax(button_up);
    }

    #if defined(btnRoot_AS_PIVOT)
      if(button_up() && start==1 && pauseOn && button_root()) {  // up block half-interval search
        if (block >oldMinBlock) {
          oldMaxBlock = block;
          block = oldMinBlock + (oldMaxBlock - oldMinBlock)/2;
        }

        GetAndPlayBlock();    
        debounce(button_up);
      }
    #endif
  #endif // BLOCKMODE

  if(button_up() && start==0
                        #ifdef btnRoot_AS_PIVOT
                              && !button_root()
                        #endif
                              ){                         // up dir sequential search                                           

    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif
    //Move up a file in the directory
    scrollTextReset();
    upFile();
    debouncemax(button_up);
  }

  #ifdef btnRoot_AS_PIVOT
    if(button_up() && start==0 && button_root()) {      // up dir half-interval search
      #if (SPLASH_SCREEN && TIMEOUT_RESET)
        timeout_reset = TIMEOUT_RESET;
      #endif
      //Move up a file in the directory
      scrollTextReset();
      upHalfSearchFile();
      debounce(button_up);
    }
  #endif

  #if defined(BLOCKMODE) && defined(BLKSJUMPwithROOT)
    if(button_root() && start==1 && pauseOn) {      // change blocks to jump 
      if (jblks==BM_BLKSJUMP) {
        jblks=1;
      } else {
        jblks=BM_BLKSJUMP;
      }

      #ifdef LCD16
        lcd.setCursor(15,0);
        if (jblks==BM_BLKSJUMP) {
          lcd.print(F("^"));
        } else {
          lcd.print(F("\'"));
        }
      #endif

      #ifdef LCD20
        lcd.setCursor(19,0);
        if (jblks==BM_BLKSJUMP) {
          lcd.print(F("^"));
        } else {
          lcd.print(F("\'"));
        }
      #endif      

      #ifdef OLED1306
        #ifdef XY2
          if (jblks==BM_BLKSJUMP) {
            sendStrXY("^",15,0);
          } else {
            sendStrXY("\'",15,0);
          }
        #else
          setXY(15,0);
          if (jblks==BM_BLKSJUMP) {
            sendChar('^');
          } else {
            sendChar('\'');
          }
        #endif
      #endif      
      debounce(button_root);
    }
  #endif

  #ifdef BLOCKMODE
    if(button_down() && start==1 && pauseOn
                                          #ifdef btnRoot_AS_PIVOT
                                                && !button_root()
                                          #endif
                                                ){      // down block sequential search                                                           

      #ifdef BLOCKID_INTO_MEM
        oldMinBlock = 0;
        oldMaxBlock = maxblock;
        if (firstBlockPause) {
          firstBlockPause = false;
          if (block > 0) block--;
          else block = maxblock;
        } else {
          if (block < maxblock) block++;
          else block = 0;       
        }             
      #endif
      #if defined(BLOCK_EEPROM_PUT)
        oldMinBlock = 0;
        oldMaxBlock = 99;
        if (firstBlockPause) {
          firstBlockPause = false;
          if (block > 0) block--;
          else block = 99;
        } else {
          if (block < 99) block++;
          else block = 0;       
        }         
      #endif
      #if defined(BLOCKID_NOMEM_SEARCH)
        if (firstBlockPause) {
          firstBlockPause = false;
          if (block > 0) block--;
        } else {
          block = block + jblks;
        }         
      #endif

      GetAndPlayBlock();    
      debouncemax(button_down);
    }
  #endif

  #if defined(BLOCKMODE) && defined(btnRoot_AS_PIVOT)
    if(button_down() && start==1 && pauseOn && button_root()) {     // down block half-interval search
      if (block <oldMaxBlock) {
        oldMinBlock = block;
        block = oldMinBlock + 1+ (oldMaxBlock - oldMinBlock)/2;
      } 
      GetAndPlayBlock();    
      debounce(button_down);
    }
  #endif

  if(button_down() && start==0
                        #ifdef btnRoot_AS_PIVOT
                                && !button_root()
                        #endif
                              ){                    // down dir sequential search                                             
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif
    //Move down a file in the directory
    scrollTextReset();
    downFile();
    debouncemax(button_down);
  }

  #ifdef btnRoot_AS_PIVOT
    if(button_down() && start==0 && button_root()) {              // down dir half-interval search
      #if (SPLASH_SCREEN && TIMEOUT_RESET)
        timeout_reset = TIMEOUT_RESET;
      #endif
      //Move down a file in the directory
      scrollTextReset();
      downHalfSearchFile();
      debounce(button_down);
    }
  #endif

  #ifndef NO_MOTOR
    if(start==1 && (oldMotorState!=motorState) && mselectMask) {  
      //if file is playing and motor control is on then handle current motor state
      //Motor control works by pulling the btnMotor pin to ground to play, and NC to stop
      if(motorState==1 && !pauseOn) {
        printtext2F(PSTR("PAUSED "),0);
        pauseOn = true;
      } 
      if(motorState==0 && pauseOn) {
        printtext2F(PSTR("Playing "),0);
        pauseOn = false;
      }
      scrollText(fileName, isDir, 0);
      oldMotorState=motorState;
    }
  #endif
  }
}

void upFile() {    
  // move up a file in the directory.
  // find prior index, using currentFile.
  if (dirEmpty) return;
  oldMinFile = 0;
  oldMaxFile = maxFile;
  while(currentFile!=0)
  {
    currentFile--;
    // currentFile might not point to a valid entry (since not all values are used)
    // and we might need to go back a bit further
    // Do this here, so that we only need this logic in one place
    // and so we can make seekFile dumber
    entry.close();
    if (entry.open(currentDir, currentFile, O_RDONLY))
    {
      entry.close();
      break;
    }
  }

  if(currentFile==0)
  {
    // seek up wrapped - should now be reset to point to the last file
    currentFile = maxFile;
  }
  seekFile();
}

void downFile() {    
  //move down a file in the directory
  if (dirEmpty) return;
  oldMinFile = 0;
  oldMaxFile = maxFile;
  currentFile++;
  if (currentFile>maxFile) { currentFile=0; }
  seekFile();
}

void upHalfSearchFile() {    
  //move up to half-pos between oldMinFile and currentFile
  if (dirEmpty) return;

  if (currentFile >oldMinFile) {
    oldMaxFile = currentFile;
    currentFile = oldMinFile + (oldMaxFile - oldMinFile)/2;
    seekFile();
  }
}

void downHalfSearchFile() {    
  //move down to half-pos between currentFile amd oldMaxFile
  if (dirEmpty) return;

  if (currentFile <oldMaxFile) {
    oldMinFile = currentFile;
    currentFile = oldMinFile + 1+ (oldMaxFile - oldMinFile)/2;
    seekFile();
  } 
}

void seekFile() {    
  //move to a set position in the directory, store the filename, and display the name on screen.
  entry.close(); // precautionary, and seems harmless if entry is already closed
  if (dirEmpty)
  {
    strcpy_P(fileName, PSTR("[EMPTY]"));
  }
  else
  {
    while (!entry.open(currentDir, currentFile, O_RDONLY) && currentFile<maxFile)
    {
      // if entry.open fails, when given an index, sdfat 2.1.2 automatically adjust curPosition to the next good entry
      // (which means that we can just retry calling open, passing in curPosition)
      // but sdfat 1.1.4 does not automatically adjust curPosition, so we cannot do that trick.
      // We cannot call openNext, because (with sdfat 2.1.2) the position has ALREADY been adjusted, so you will actually
      // miss a file.
      // so need to do this manually (to support both library versions), via incrementing currentFile manually
      currentFile++;
    }

    entry.getName(fileName,filenameLength);
    filesize = entry.fileSize();
    if(entry.isDir() || !strcmp(fileName, "ROOT")) { isDir=1; } else { isDir=0; }
    entry.close();
  }

  if (isDir==1) {
    if (subdir >0)strcpy(PlayBytes,prevSubDir);
    else strcpy_P(PlayBytes, P_PRODUCT_NAME);
   
  } else {
    ultoa(filesize,PlayBytes,10);
    strcat_P(PlayBytes,PSTR(" bytes"));
  }

  printtext(PlayBytes,0);
  scrollText(fileName, isDir, 0);
}

void stopFile() {
  UniStop();
  if(start==1){
    printtextF(PSTR("Stopped"),0);
    #ifdef P8544
      lcd.gotoRc(3,38);
      lcd.bitmap(Stop, 1, 6);
    #endif
    start=0;
  }
}

void playFile() {
  if(isDir==1) {
    //If selected file is a directory move into directory
    changeDir();
  }
  else if (!dirEmpty) 
  {
    printtextF(PSTR("Playing"),0);
    pauseOn = false;
    scrollText(fileName, isDir, 0);
    currpct=100;
    lcdsegs=0;
    UniPlay();
    start=1;       
  }    
}

void getMaxFile() {    
  // gets the total files in the current directory and stores the number in maxFile
  // and also gets the file index of the last file found in this directory
  currentDir->rewind();
  maxFile = 0;
  dirEmpty=true;
  while(entry.openNext(currentDir, O_RDONLY)) {
    maxFile = currentDir->curPosition()/32-1;
    dirEmpty=false;
    entry.close();
  }
  currentDir->rewind(); // precautionary but I think might be unnecessary since we're using currentFile everywhere else now
  oldMinFile = 0;
  oldMaxFile = maxFile;
  currentFile = 0;
}

void changeDir() {    
  // change directory (to whatever is currently the selected fileName)
  // if fileName="ROOT" then return to the root directory
  if (dirEmpty) {
    // do nothing because you haven't selected a valid file yet and the directory is empty
    return;
  }
  else if(!strcmp(fileName, "ROOT"))
  {
    subdir=0;    
    changeDirRoot();
  }
  else
  {
    if (subdir < nMaxPrevSubDirs) {
      DirFilePos[subdir] = currentFile;
      subdir++;
      // but, also, stash the current filename as the parent (prevSubDir) for display
      // (but we only keep one prevSubDir, we no longer need to store them all)
      fileName[SCREENSIZE] = '\0';
      strcpy(prevSubDir, fileName);
    }
    _tmpdirs[_alt_tmp_dir].open(currentDir, currentFile, O_RDONLY);
    // flip the dir pointers so currentDir is now the newly opened dir and tmpdir points to the spare
    currentDir = &_tmpdirs[_alt_tmp_dir];
    _alt_tmp_dir = 1-_alt_tmp_dir;
    _tmpdirs[_alt_tmp_dir].close(); // closes the old dir
  }
  getMaxFile();
  seekFile();
}

void changeDirParent()
{
  // change up to parent directory.
  // get to parent folder via re-navigating the same sequence of directories, starting at root
  // (slow-ish but, honestly, not very slow. and requires very little memory to keep a deep stack of parents
  // since you only need to store one file index, not a whole filename, for each directory level)
  // Note to self : does sdfat now support any better way of doing this e.g. is there a link back to parent like ".."  ?
  subdir--;
  uint16_t this_directory=DirFilePos[subdir]; // remember what directory we are in currently

  changeDirRoot();
  if(subdir>0)
  {
    for(int i=0; i<subdir; i++)
    {
      _tmpdirs[_alt_tmp_dir].open(currentDir, DirFilePos[i], O_RDONLY);
      // flip the dir pointers so currentDir is now the newly opened dir and tmpdir points to the spare
      currentDir = &_tmpdirs[_alt_tmp_dir];
      _alt_tmp_dir = 1-_alt_tmp_dir;
      _tmpdirs[_alt_tmp_dir].close(); // closes the old dir
    }
    // get the filename of the last directory we opened, because this is now the prevSubDir for display
    currentDir->getName(fileName, filenameLength);
    fileName[SCREENSIZE] = '\0'; // copy only the piece we need. small cheat here - we terminate the string where we want it to end...
    strcpy(prevSubDir, fileName);
  }
   
  getMaxFile();
  currentFile = this_directory; // select the directory we were in, as the current file in the parent
  seekFile(); // don't forget that this will put the real filename back into fileName 
}

void changeDirRoot()
{
  currentDir->close();
  currentDir->open("/", O_RDONLY);                    //set SD to root directory
}

void SetPlayBlock()
{
  printtextF(PSTR(" "),0);

  #ifdef LCD16
    lcd.setCursor(0,0);
    lcd.print(F("BLK:"));
    lcd.print(block);
    lcd.print(F(" ID:"));
    lcd.print(currentID,HEX); // Block ID en hex
  #endif

  #ifdef LCD20
    lcd.setCursor(0,0);
    lcd.print(F("BLK:"));
    lcd.print(block);
    lcd.print(F(" ID:"));
    lcd.print(currentID,HEX); // Block ID en hex
  #endif  

  #if defined(OLED1306)
    #if defined(XY2)
      utoa(block, (char *)input, 10);
      #if defined(OLEDBLKMATCH)              
        if (block<10) {
          input[1]=input[0];
          input[0]='0';
          input[2]=0;
        }
        if (block < 100) {
          sendStrXY((char *)input,14,4);
        } else {
          sendStrXY((char *)(input+1),14,4);
        }
      #endif                         

      sendStrXY("BLK:",0,0);
      sendStrXY((char *)input,4,0);
        
      if (block < 100) {
        sendStrXY(" ID:", 6,0);
      } else {
        sendStrXY(" ID:", 7,0);
      }
          
      input[0]=pgm_read_byte(HEX_CHAR+(currentID>>4));
      input[1]=pgm_read_byte(HEX_CHAR+(currentID&0x0f));
      input[2]=0;
      if (block < 100) {
        sendStrXY((char *)input,10,0);
      } else {
        sendStrXY((char *)input,11,0);
      }
          
    #else // defined(XY2)

      utoa(block, (char *)input, 10);
      #if defined(OLEDBLKMATCH)              
        setXY(14,2);
        if (block <10) {
          sendChar('0');
        }
        if (block < 100) {
          sendStr((char *)input);
        } else {
          sendStr((char *)(input+1));
        }
      #endif                           
      setXY(0,0);
      sendStr("BLK:");
      sendStr((char *)input);//sendChar(' ');
      sendStr(" ID:");

      input[0]=pgm_read_byte(HEX_CHAR+(currentID>>4));
      input[1]=pgm_read_byte(HEX_CHAR+(currentID&0x0f));
      input[2]=0;
      sendStr((char *)input);
    #endif
  #endif // defined(OLED1306)

  clearBuffer();
  currpct=100; 
  lcdsegs=0;       
  currentBit=0;                               // fallo reproducción de .tap tras .tzx

#ifdef Use_CAS
  if (casduino==CASDUINO_FILETYPE::NONE) // not a CAS / DRAGON file
#endif
  {
    currentBlockTask = BLOCKTASK::READPARAM;               //First block task is to read in parameters
    Timer.setPeriod(1000);
  }
}

void GetAndPlayBlock()
{
  #ifdef BLOCKID_INTO_MEM
    bytesRead=blockOffset[block%maxblock];
    currentID=blockID[block%maxblock];   
  #endif
  #ifdef BLOCK_EEPROM_PUT
    EEPROM_get(BLOCK_EEPROM_START+5*block, bytesRead);
    EEPROM_get(BLOCK_EEPROM_START+4+5*block, currentID);
  #endif
  #ifdef BLOCKID_NOMEM_SEARCH 
    unsigned long oldbytesRead=0;
    bytesRead=0;
    if (currentID!=BLOCKID::TAP) bytesRead=10;   //TZX with blocks skip TZXHeader

    #ifdef BLKBIGSIZE
      unsigned int i = 0;
    #else
      byte i = 0;
    #endif      

    while (i<= block) {
      if(ReadByte()) {
        oldbytesRead=bytesRead-1;
        if (currentID!=BLOCKID::TAP) currentID = outByte;  //TZX with blocks GETID
        if (currentID==BLOCKID::TAP) bytesRead--;
      }
      else {
        block = i-1;
        break;
      }
        
      switch(currentID) {
        case BLOCKID::ID10:
                    bytesRead+=2;     //Pause                
                    if(ReadWord()) bytesRead += outWord; //Length of data that follow
                    #if defined(OLEDBLKMATCH)
                      i++;
                    #endif
                    break;

        case BLOCKID::ID11:
                    bytesRead+=15; //lPilot,lSynch1,lSynch2,lZ,lO,lP,LB,Pause
                    if(ReadLong()) bytesRead += outLong;
                    #if defined(OLEDBLKMATCH)
                      i++;
                    #endif                      
                    break;

        case BLOCKID::ID12:
                    bytesRead+=4;
                    break;

        case BLOCKID::ID13:
                    if(ReadByte()) bytesRead += (long(outByte) * 2);
                    break;

        case BLOCKID::ID14:
                    bytesRead+=7;
                    if(ReadLong()) bytesRead += outLong;
                    /*
                      #if defined(OLEDBLKMATCH) //&& defined(BLOCKID14_IN)
                        i++;
                      #endif 
                    */                   
                    break;

        case BLOCKID::ID15:
                    bytesRead+=5;
                    if(ReadLong()) bytesRead += outLong;
                    #if defined(OLEDBLKMATCH)
                      i++;
                    #endif                     
                    break;

        case BLOCKID::ID19:
                    if(ReadDword()) bytesRead += outLong;
                    #if defined(OLEDBLKMATCH) //&& defined(BLOCKID19_IN)
                      i++;
                    #endif          
                    break;

        case BLOCKID::ID20:
                    bytesRead+=2;
                    break;

        case BLOCKID::ID21:
                    if(ReadByte()) bytesRead += outByte;
                    #if defined(OLEDBLKMATCH) && defined(BLOCKID21_IN)
                      i++;
                    #endif          
                    break;

        case BLOCKID::ID22:
                    break;

        case BLOCKID::ID24:
                    bytesRead+=2;
                    break;

        case BLOCKID::ID25:
                    break;

        case BLOCKID::ID2A:
                    bytesRead+=4;
                    break;

        case BLOCKID::ID2B:
                    bytesRead+=5;
                    break;

        case BLOCKID::ID30:
                    if (ReadByte()) bytesRead += outByte;                                            
                    break;

        case BLOCKID::ID31:
                    bytesRead+=1;         
                    if(ReadByte()) bytesRead += outByte; 
                    break;

        case BLOCKID::ID32:
                    if(ReadWord()) bytesRead += outWord;
                    break;

        case BLOCKID::ID33:
                    if(ReadByte()) bytesRead += (long(outByte) * 3);
                    break;

        case BLOCKID::ID35:
                    bytesRead += 0x10;
                    if(ReadDword()) bytesRead += outLong;
                    break;

        case BLOCKID::ID4B:
                    if(ReadDword()) bytesRead += outLong;
                    #if defined(OLEDBLKMATCH)
                      i++;
                    #endif          
                    break;

        case BLOCKID::TAP:
                     if(ReadWord()) bytesRead += outWord;
                    #if defined(OLEDBLKMATCH) && defined(BLOCKTAP_IN)
                      i++;
                    #endif           
                    break;
      }
      #if not defined(OLEDBLKMATCH)
        i++;
      #endif  
    }

    bytesRead=oldbytesRead;
  #endif   

  if (currentID==BLOCKID::TAP) {
    currentTask=TASK::PROCESSID;
  }else {
    currentTask=TASK::GETID;    //Get new TZX Block
    if(ReadByte()) {
      //TZX with blocks GETID
      currentID = outByte;
      currentTask=TASK::PROCESSID;
    }
  }
   
  SetPlayBlock(); 
}

void str4cpy(char *dest, const char *src)
{
  byte x=0;
  while (*(src+x) && (x<4)) {
    dest[x] = src[x];
    x++;
  }
  for(; x<4; x++) dest[x]=' ';
  dest[4]=0; 
}

void GetFileName(uint16_t pos)
{
  entry.close(); // precautionary, and seems harmless if entry is already closed
  if (entry.open(currentDir, pos, O_RDONLY))
  {
    entry.getName(fileName,filenameLength);
  }
  entry.close();
}

void block_mem_oled()
{
  #ifdef BLOCKID_INTO_MEM
    blockOffset[block%maxblock] = bytesRead;
    blockID[block%maxblock] = currentID;
  #endif
  #ifdef BLOCK_EEPROM_PUT
    EEPROM_put(BLOCK_EEPROM_START+5*block, bytesRead);
    EEPROM_put(BLOCK_EEPROM_START+4+5*block, currentID);
  #endif

  #if defined(OLED1306) && defined(OLEDPRINTBLOCK) 
    #ifdef XY
      setXY(3,3);
      sendChar(pgm_read_byte(HEX_CHAR+(currentID>>4)));sendChar(pgm_read_byte(HEX_CHAR+(currentID&0x0f)));
      setXY(10,3);
      if ((block%10) == 0) sendChar('0'+(block/10)%10);  
      setXY(11,3);
      sendChar('0'+block%10);
    #endif
    #if defined(XY2) && not defined(OLED1306_128_64)
      setXY(9,1);
      sendChar(pgm_read_byte(HEX_CHAR+(currentID>>4)));sendChar(pgm_read_byte(HEX_CHAR+(currentID&0x0f)));
      setXY(12,1);
      if ((block%10) == 0) sendChar('0'+(block/10)%10);
      setXY(13,1);sendChar('0'+block%10);
    #endif
    #if defined(XY2) && defined(OLED1306_128_64)
      #ifdef XY2force
        input[0]=pgm_read_byte(HEX_CHAR+(currentID>>4));
        input[1]=pgm_read_byte(HEX_CHAR+(currentID&0x0f));
        input[2]=0;
        sendStrXY((char *)input,7,4);
        if ((block%10) == 0) {
          utoa((block/10)%10,(char *)input,10);
          sendStrXY((char *)input,14,4);
        }
        input[0]='0'+block%10;
        input[1]=0;
        sendStrXY((char *)input,15,4);
      #else                      
        setXY(7,4);
        sendChar(pgm_read_byte(HEX_CHAR+(currentID>>4)));sendChar(pgm_read_byte(HEX_CHAR+(currentID&0x0f)));
        setXY(14,4);
        if ((block%10) == 0) sendChar('0'+(block/10)%10);
        setXY(15,4);
        sendChar('0'+block%10);
      #endif
    #endif                    
  #endif

  #if defined(BLOCKID_INTO_MEM)
    if (block < maxblock) block++;
    else block = 0;
  #endif
  #if defined(BLOCK_EEPROM_PUT) 
    if (block < 99) block++;
    else block = 0; 
  #endif
  #if defined(BLOCKID_NOMEM_SEARCH) 
    block++;
  #endif             
}

void NoSDCard() {
  #ifdef OLED1306
    setXY(0,1); sendStr((unsigned char *)"----------------");
    setXY(0,2); sendStr((unsigned char *)"   No SD Card   ");
    setXY(0,3); sendStr((unsigned char *)"----------------");
    setXY(0,4); sendStr((unsigned char *)"   Insert  SD   ");
    setXY(0,5); sendStr((unsigned char *)"----------------");        
  #endif
  #ifdef LCD16
    lcd.setCursor(0,0); lcd.print("   No SD Card   ");
    lcd.setCursor(0,1); lcd.print("   Insert  SD   ");    
  #endif
  #ifdef LCD20
    lcd.setCursor(0,0); lcd.print("--------------------");
    lcd.setCursor(0,1); lcd.print("     No SD Card     ");
    lcd.setCursor(0,2); lcd.print(" Please, Insert SD. ");
    lcd.setCursor(0,3); lcd.print("--------------------");        
  #endif  
}

void SDCardOK() {
  #ifdef OLED1306
    clear_display();
    setXY(0,2); sendStr((unsigned char *)"----------------");
    setXY(0,3); sendStr((unsigned char *)"  SD Card - OK  ");
    setXY(0,4); sendStr((unsigned char *)"----------------");
  #endif
  #ifdef LCD16
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("  SD Card - OK  ");
  #endif
  #ifdef LCD20
    lcd.clear();
    printtextF(PSTR("--------------------"),0);    
    printtextF(PSTR("    SD Card - OK    "),1);
    printtextF(PSTR("--------------------"),2);
  #endif  
}

void Banner() {
  #ifdef OLED1306
    setXY(0,2);
    sendStr((unsigned char *)"----------------");
    setXY(1,3);
    sendStr((unsigned char *)VERSION);
    setXY(0,4);
    sendStr((unsigned char *)"----------------");
  #endif
  #ifdef LCD16
    lcd.setCursor(1,0); lcd.print(F(VERSION));
    lcd.clear();
  #endif
  #ifdef LCD20
    lcd.setCursor(1,0); lcd.print(F(VERSION));
    lcd.clear();
  #endif  
}
