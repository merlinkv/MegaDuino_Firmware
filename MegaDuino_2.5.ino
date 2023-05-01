#define VERSION "MegaDuino 2.5"

//
// Original firmware writen by Andrew Beer, Duncan Edwards, Rafael Molina and others.
//
// MegaDuino Firmware is an adaptation of the MaxDuino firmware by Rafael Molina 
// but is specially designed for my MegaDuino & MegaDuino STM32 projects.
// 
// Displays that can be used: LCD 16x2, LCD 20x4, OLED 128x32 & OLED 128x64
//
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

#if defined(__AVR_ATmega2560__)
  #include "Mega2560config.h"
#elif defined(__arm__) && defined(__STM32F1__)
  #include "STM32config.h"  
#endif

#include "MegaDuino.h"
#include "hardwarecfg.h"
#include "buttons.h"

#if defined(BLOCK_EEPROM_PUT) || defined(LOAD_EEPROM_LOGO) || defined(RECORD_EEPROM_LOGO) || defined(LOAD_EEPROM_SETTINGS)
#include <EEPROM.h>
#endif

char fline[17];

SdFat sd;                           //Initialise Sd card 
SdBaseFile entry, currentDir, tmpdir;                       //SD card file and directory

#define filenameLength 256

char fileName[filenameLength + 1];    //Current filename
#define nMaxPrevSubDirs 10
char prevSubDir[SCREENSIZE+1];
uint16_t DirFilePos[nMaxPrevSubDirs];  //File Positions in Directory to be restored (also, history of subdirectories)
byte subdir = 0;
unsigned long filesize;             // filesize used for dimensioning AY files

byte scrollPos = 0;                 //Stores scrolling text position
unsigned long scrollTime = millis() + scrollWait;

#ifndef NO_MOTOR
  byte motorState = 1;                //Current motor control state
  byte oldMotorState = 1;             //Last motor control state
#endif

byte start = 0;                     //Currently playing flag

byte pauseOn = 0;                   //Pause state
uint16_t currentFile = 0;           //File index (per filesystem) of current file, relative to current directory (pointed to by currentDir)
uint16_t maxFile = 0;                    //Total number of files in directory
uint16_t oldMinFile = 0;
uint16_t oldMaxFile = 0;
#define fnameLength  5
char oldMinFileName[fnameLength];
char oldMaxFileName[fnameLength];
byte isDir = 0;                     //Is the current file a directory
unsigned long timeDiff = 0;         //button debounce
int Delete = 0;
int numfiles = 0;
int filenum = 1;

#if (SPLASH_SCREEN && TIMEOUT_RESET)
    unsigned long timeDiff_reset = 0;
    byte timeout_reset = TIMEOUT_RESET;
#endif

byte REWIND = 0;                      //Next File, down button pressed
char PlayBytes[17];

unsigned long blockOffset[maxblock];
byte blockID[maxblock];

byte lastbtn=true;

void(* resetFunc) (void) = 0;//declare reset function at adress 0

#if (SPLASH_SCREEN && TIMEOUT_RESET)
    void(* resetFunc) (void) = 0;//declare reset function at adress 0
    /*void resetFunc() // Restarts program from beginning but does not reset the peripherals and registers
    {
    asm volatile ("  jmp 0");
    }*/
#endif

void setup() {

  #include "pinSetup.h"
  pinMode(chipSelect, OUTPUT);      //Setup SD card chipselect pin

  #if defined(LCD16) || defined(LCD20)
    lcd.init();                     //Initialise LCD
    lcd.backlight();
    lcd.clear();
    Banner();
  #endif
  #ifdef OLED1306 
    #if defined(Use_SoftI2CMaster) 
      i2c_init();
    #else
      Wire.begin();
    #endif    
    init_OLED();
    #if (!SPLASH_SCREEN)
      #if defined(LOAD_MEM_LOGO) || defined(LOAD_EEPROM_LOGO)
        delay(1800);              // Show logo
      #endif
      reset_display();           // Clear logo and load saved mode
    #endif
    Banner();
  #endif
  #ifdef SPLASH_SCREEN
    while (!button_any()){
      delay(1800);              // Show logo (OLED) or text (LCD) and remains until a button is pressed.
    }   
    #ifdef OLED1306    
      reset_display();           // Clear logo and load saved mode
    #endif
    Banner();
  #endif
  
  while (!sd.begin(chipSelect,SPI_FULL_SPEED)) {
    NoSDCard();
  }    
  changeDirRoot();
  UniSetup();                       //Setup TZX specific options
  SDCardOK();
  delay(1600);
  #if defined(LCD16) || defined(LCD20)
    lcd.clear();
  #endif
  #ifdef OLED1306
    reset_display();
  #endif     
  getMaxFile();                     //get the total number of files in the directory
  seekFile();            //move to the first file in the directory

  #ifdef LOAD_EEPROM_SETTINGS
    loadEEPROM();
  #endif    
  #if defined(OLED1306) && defined(OSTATUSLINE)
    OledStatusLine();
  #elif defined(LCD20)
    LCDBStatusLine();
  #endif  
}

void loop(void) {
  if(start==1)
  {
    //TZXLoop only runs if a file is playing, and keeps the buffer full.
    uniLoop();
  } else {
  //  digitalWrite(outputPin, LOW);    //Keep output LOW while no file is playing.
    WRITE_LOW;    
  }
  
  if((millis()>=scrollTime) && start==0 && (strlen(fileName)> SCREENSIZE)) {
    //Filename scrolling only runs if no file is playing to prevent I2C writes 
    //conflicting with the playback Interrupt
    scrollTime = millis()+scrollSpeed;
    scrollText(fileName);
    scrollPos +=1;
    if(scrollPos>strlen(fileName)) {
      scrollPos=0;
      scrollTime=millis()+scrollWait;
      scrollText(fileName);
    }
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
      
     if(button_play()) {
        //Handle Play/Pause button
        if(start==0) {
          //If no file is play, start playback
          playFile();
          #ifndef NO_MOTOR
            if (mselectMask == 1){  
            //oldMotorState = !motorState;  //Start in pause if Motor Control is selected
              oldMotorState = 0;
            }
          #endif
          delay(200);
        } else {
          //If a file is playing, pause or unpause the file                  
          #if defined(LCD16)
            String filename2 = fileName;
            lcd.setCursor(0,0); lcd.print(filename2.substring(0,16));
          #elif defined(LCD20)
            String filename2 = fileName;
            lcd.setCursor(0,0); lcd.print(filename2.substring(0,20));
          #elif defined(OLED1306)
            if (pauseOn == 0){
              printtextF(PSTR("Paused"),1);
            } else {
                printtextF(PSTR("Playing"),1);
                setXY(0,2); sendStr((unsigned char *)"----------------");
            }
          #else            
            if (pauseOn == 0) {
              printtextF(PSTR("Paused"),1);
              jblks =1; 
              firstBlockPause = true;
            } else {
              printtextF(PSTR("Playing"),1);
              firstBlockPause = false; 
            }
          #endif
          #if defined(LCD16            )
            if (currpct <100) {              
              itoa(newpct,PlayBytes,10);strcat_P(PlayBytes,PSTR("%"));lcd.setCursor(9,1);lcd.print(PlayBytes);
            }
            PlayBytes[0]=PlayBytes[1]=PlayBytes[2]='0';
            #ifndef cntrMSS
              if ((lcdsegs %1000) <10) itoa(lcdsegs%10,PlayBytes+2,10);
              else 
                 if ((lcdsegs %1000) <100)itoa(lcdsegs%1000,PlayBytes+1,10);
                 else 
                    itoa(lcdsegs%1000,PlayBytes,10);
            #endif
            #ifdef cntrMSS
              if ((lcdsegs %600) <10) itoa(lcdsegs%10,PlayBytes+2,10);
              else 
                 if ((lcdsegs %600) <60)itoa(lcdsegs%600,PlayBytes+1,10);
                 else 
                    itoa(lcdsegs%600 /60 *100 + lcdsegs%60,PlayBytes,10);
            #endif  
            lcd.setCursor(13,1);lcd.print(PlayBytes); 
          #elif defined(LCD20)
            if (currpct <100) {              
              itoa(newpct,PlayBytes,10);strcat_P(PlayBytes,PSTR("%"));lcd.setCursor(11,1);lcd.print(PlayBytes);
            }
            PlayBytes[0]=PlayBytes[1]=PlayBytes[2]='0';
            #ifndef cntrMSS
              if ((lcdsegs %1000) <10) itoa(lcdsegs%10,PlayBytes+2,10);
              else 
                 if ((lcdsegs %1000) <100)itoa(lcdsegs%1000,PlayBytes+1,10);
                 else 
                    itoa(lcdsegs%1000,PlayBytes,10);
            #endif
            #ifdef cntrMSS
              if ((lcdsegs %600) <10) itoa(lcdsegs%10,PlayBytes+2,10);
              else 
                 if ((lcdsegs %600) <60)itoa(lcdsegs%600,PlayBytes+1,10);
                 else 
                    itoa(lcdsegs%600 /60 *100 + lcdsegs%60,PlayBytes,10);
            #endif  
            lcd.setCursor(17,1);lcd.print(PlayBytes); 
          #elif defined(OLED1306)
            if (currpct <100) {                         
              itoa(newpct,PlayBytes,10);strcat_P(PlayBytes,PSTR("%"));setXY (8,1);sendStr((unsigned char *)(PlayBytes));
            } else {                          // Block number must me printed after REW
                setXY(10,3);
                sendChar(48+(block)/10);
                sendChar(48+(block)%10);                            
            }
            PlayBytes[0]=PlayBytes[1]=PlayBytes[2]='0';
            #ifndef cntrMSS
              if ((lcdsegs %1000) <10) itoa(lcdsegs%10,PlayBytes+2,10);
              else 
                 if ((lcdsegs %1000) <100)itoa(lcdsegs%1000,PlayBytes+1,10);
                 else 
                    itoa(lcdsegs%1000,PlayBytes,10);
            #endif
            #ifdef cntrMSS
              if ((lcdsegs %600) <10) itoa(lcdsegs%10,PlayBytes+2,10);
              else 
                 if ((lcdsegs %600) <60)itoa(lcdsegs%600,PlayBytes+1,10);
                 else 
                    itoa(lcdsegs%600 /60 *100 + lcdsegs%60,PlayBytes,10);
            #endif                 
            setXY(13,1); sendStr((unsigned char *)(PlayBytes));
          #endif
          pauseOn = !pauseOn;
        }
        debounce(button_play);
      }

  #ifdef ONPAUSE_POLCHG
    if(button_root() && start==1 && pauseOn==1
      #ifdef btnRoot_AS_PIVOT   
        && button_stop()
      #endif
    ){  
    TSXCONTROLzxpolarityUEFSWITCHPARITY = !TSXCONTROLzxpolarityUEFSWITCHPARITY;
    #if defined(OLED1306)
      printtextF(PSTR("                "),1); 
      OledStatusLine();
    #elif defined(LCD16)
      printtextF(PSTR("                "),1);
    #elif defined(LCD20)
      printtextF(PSTR("                    "),1);
      LCDBStatusLine();
    #endif
    debounce(button_root);
    }
  #endif

  #ifdef btnRoot_AS_PIVOT
    lastbtn=false;     
    if(button_root() && start==0 && !lastbtn) {                                          // show min-max dir
      #ifdef SHOW_DIRPOS
        #if defined(LCD16) && !defined(SHOW_STATUS_LCD) && !defined(SHOW_DIRNAMES)
          char len=0;
          lcd.setCursor(0,0); 
          lcd.print(itoa(oldMinFile,input,10)); lcd.print('<'); len += strlen(input) + 1;
          lcd.print(itoa(currentFile,input,10)); lcd.print('<'); len += strlen(input) + 1;
          lcd.print(itoa(oldMaxFile,input,10)); len += strlen(input); 
          for(char x=len;x<16;x++) lcd.print(' '); 
        #endif
        #if defined(LCD16) && defined(SHOW_STATUS_LCD)        
          lcd.setCursor(0,0);
          lcd.print(BAUDRATE);
          lcd.print(' ');
          if(mselectMask==1) lcd.print(F(" M:ON"));
          else lcd.print(F("m:off"));
          lcd.print(' ');
          if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) lcd.print(F(" %^ON"));
          else lcd.print(F("%^off"));         
        #endif 
        #if defined(LCD16) && defined(SHOW_DIRNAMES)
          str4cpy(input,fileName);
          GetFileName(oldMinFile); str4cpy(oldMinFileName,fileName);
          GetFileName(oldMaxFile); str4cpy(oldMaxFileName,fileName);
          GetFileName(currentFile); 
          lcd.setCursor(0,0);
          lcd.print(oldMinFileName);lcd.print(' ');lcd.print('<');
          lcd.print((char *)input);lcd.print('<');lcd.print(' ');
          lcd.print(oldMaxFileName);                  
        #endif
        #if defined(LCD20) && !defined(SHOW_STATUS_LCD) && !defined(SHOW_DIRNAMES)
          char len=0;
          lcd.setCursor(0,0); 
          lcd.print(itoa(oldMinFile,input,10)); lcd.print('<'); len += strlen(input) + 1;
          lcd.print(itoa(currentFile,input,10)); lcd.print('<'); len += strlen(input) + 1;
          lcd.print(itoa(oldMaxFile,input,10)); len += strlen(input); 
          for(char x=len;x<20;x++) lcd.print(' '); 
        #endif
        #if defined(LCD20) && defined(SHOW_STATUS_LCD)        
          lcd.setCursor(0,0);
          lcd.print(BAUDRATE);
          lcd.print(' ');
          if(mselectMask==1) lcd.print(F(" M:ON"));
          else lcd.print(F("M:OFF"));
          lcd.print(' ');
          if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) lcd.print(F(" %^ON"));
          else lcd.print(F("%^OFF"));         
        #endif 
        #if defined(LCD20) && defined(SHOW_DIRNAMES)
          str4cpy(input,fileName);
          GetFileName(oldMinFile); str4cpy(oldMinFileName,fileName);
          GetFileName(oldMaxFile); str4cpy(oldMaxFileName,fileName);
          GetFileName(currentFile); 
          lcd.setCursor(0,0);
          lcd.print(oldMinFileName);lcd.print(' ');lcd.print('<');
          lcd.print((char *)input);lcd.print('<');lcd.print(' ');
          lcd.print(oldMaxFileName);                  
        #endif
        #if defined(OLED1306) && !defined(SHOW_DIRNAMES)
          char len=0;
          setXY(0,0);
          sendStr(itoa(oldMinFile,input,10));sendChar('<');len += strlen(input) + 1;
          sendStr(itoa(currentFile,input,10));sendChar('<'); len += strlen(input) + 1;
          sendStr(itoa(oldMaxFile,input,10)); len += strlen(input);                   
          for(char x=len;x<16;x++) sendChar(' ');                       
        #endif
        #if defined(OLED1306) && defined(SHOW_DIRNAMES)
          str4cpy(input,fileName);
          GetFileName(oldMinFile); str4cpy(oldMinFileName,fileName);
          GetFileName(oldMaxFile); str4cpy(oldMaxFileName,fileName);
          GetFileName(currentFile); 
          setXY(0,0);
          sendStr(oldMinFileName);sendChar(' ');sendChar('<');
          sendStr((char *)input);sendChar('<');sendChar(' ');
          sendStr(oldMaxFileName);
        #endif
      #endif        
      while(button_root() && !lastbtn) {
        lastbtn = 1;
        checkLastButton();           
      }        
      printtext(PlayBytes,0);
      }
      #if defined(LCD16) || defined(LCD20) && defined(SHOW_BLOCKPOS_LCD)
        if(button_root() && start==1 && pauseOn==1 && !lastbtn) {                                          // show min-max block
          if defined(LCD16)
            lcd.setCursor(11,0);
          if defined(LCD20)
            lcd.setCursor(13,0);
            if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) lcd.print(F(" %^ON"));
          else lcd.print(F("%^OFF"));            
          while(button_root() && start==1 && !lastbtn) {
          lastbtn = 1;
          checkLastButton();           
          }
          lcd.setCursor(11,0);
          lcd.print(' ');
          lcd.print(' ');
          lcd.print(PlayBytes);        
         }
      #endif
  #endif

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
      #if defined(LCD16) || defined(LCD20)
        lcd.setCursor(0,0);
        printtext(PlayBytes,1);          
        printtextF(PSTR(""),1);
      #elif defined(OLED1306)
        printtext(PlayBytes,0);
        printtextF(PSTR(""),lineaxy);
      #endif
      scrollPos=0;
      scrollText(fileName);    
    #else
      #ifdef RECORD_EEPROM_LOGO
        init_OLED();
        delay(1500);              // Show logo
        reset_display();           // Clear logo and load saved mode
        printtextF(PSTR("Reset.."),0);
        delay(500);
        PlayBytes[0]='\0'; 
        strcat_P(PlayBytes,PSTR(VERSION));
        printtext(PlayBytes,1);
        #if defined(OSTATUSLINE)
          OledStatusLine();
        #endif
      #else             
        subdir=0;
        changeDirRoot();
        getMaxFile();
        currentFile=0;
        seekFile();
      #endif         
    #endif
    debounce(button_root);
    ltoa(filesize,PlayBytes,10);printtext(strcat_P(PlayBytes,PSTR(" bytes")),1);      
  }

  if(button_stop() && start==1
    #ifdef btnRoot_AS_PIVOT
      && !button_root()
    #endif
    ){      
    stopFile();
    debounce(button_stop);
    #if defined(LCD16)
      lcd.setCursor(0,1); lcd.print("                ");  
    #elif defined(LCD20)
      lcd.setCursor(0,1); lcd.print("                    ");  
      LCDBStatusLine();
    #elif defined(OLED1306)
      OledStatusLine();
    #endif     
    ltoa(filesize,PlayBytes,10);printtext(strcat_P(PlayBytes,PSTR(" bytes")),1);    
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
    if(button_up() && start==1 && pauseOn==1
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
  #endif

  #if defined(BLOCKMODE) && defined(btnRoot_AS_PIVOT)
    if(button_up() && start==1 && pauseOn==1 && button_root()) {  // up block half-interval search
    if (block >oldMinBlock) {
    oldMaxBlock = block;
    block = oldMinBlock + (oldMaxBlock - oldMinBlock)/2;
    }
    GetAndPlayBlock();    
    debounce(button_up);
    }
  #endif

  if(button_up() && start==0
    #ifdef btnRoot_AS_PIVOT
      && !button_root()
    #endif
    ){                         // up dir sequential search                                           
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif
    scrollTime=millis()+scrollWait;
    scrollPos=0;
    upFile();
    debouncemax(button_up);
  }

  #ifdef btnRoot_AS_PIVOT
    if(button_up() && start==0 && button_root()) {      // up dir half-interval search
      #if (SPLASH_SCREEN && TIMEOUT_RESET)
        timeout_reset = TIMEOUT_RESET;
      #endif
      //Move up a file in the directory
      scrollTime=millis()+scrollWait;
      scrollPos=0;
      upHalfSearchFile();
      debounce(button_up);
    }
  #endif

  #if defined(BLOCKMODE) && defined(BLKSJUMPwithROOT)
    if(button_root() && start==1 && pauseOn==1){      // change blocks to jump 
    if (jblks==BM_BLKSJUMP) jblks=1; else jblks=BM_BLKSJUMP;
    #if defined(LCD16)
      lcd.setCursor(15,0); if (jblks==BM_BLKSJUMP) lcd.print(F("^")); else lcd.print(F("\'"));
    #elif defined(LCD20)
      lcd.setCursor(19,0); if (jblks==BM_BLKSJUMP) lcd.print(F("^")); else lcd.print(F("\'"));
    #elif defined(OLED1306)
      #ifdef XY2
        if (jblks==BM_BLKSJUMP) sendStrXY("^",15,3); else sendStrXY("\'",15,3);  
      #else
        setXY(15,3);if (jblks==BM_BLKSJUMP) sendChar('^'); else sendChar('\'');  
      #endif
    #endif      
    debounce(button_root);
    }
  #endif

  #ifdef BLOCKMODE
    if(button_down() && start==1 && pauseOn==1
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
    if(button_down() && start==1 && pauseOn==1 && button_root()) {     // down block half-interval search
    if (block <oldMaxBlock) {
      pldMinBlock = block;
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
    scrollTime=millis()+scrollWait;
    scrollPos=0;
    downFile();
    debouncemax(button_down);
    }
  
  #ifdef btnRoot_AS_PIVOT
    if(button_down() && start==0 && button_root()) {              // down dir half-interval search
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif
    scrollTime=millis()+scrollWait;
    scrollPos=0;
    downHalfSearchFile();
    debounce(button_down);
    }
  #endif

  #ifndef NO_MOTOR
    if(start==1 && (oldMotorState!=motorState) && mselectMask==1 ) {  
      if(motorState==1 && pauseOn==0){
      #if defined(LCD16)
        lcd.setCursor(0,1); lcd.print("---PRESS PLAY---");    
      #elif defined(LCD20)
        lcd.setCursor(0,1); lcd.print("-----PRESS PLAY-----");    
      #elif defined(OLED1306)
        #ifdef XY
          setXY(0,0); printtext(fileName,0);
          setXY(0,2); sendStr((unsigned char *)"---PRESS PLAY---");
        #endif
        #ifdef XY2
          sendStrXY("PAUSED ",0,0);
        #endif
      #endif 
      scrollPos=0;
      pauseOn=1;
      }
      if(motorState==0 && pauseOn==1) {
        #if defined(LCD16)
          String filename2 = fileName;
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print(filename2.substring(0,16));
          lcd.setCursor(0,1); lcd.print("Playing         ");
          lcd.setCursor(9,1); lcd.print(newpct); lcd.print("%");
          lcd.setCursor(14,1); lcd.print("000");
          if (lcdsegs % CNTRBASE != 0){itoa(lcdsegs%CNTRBASE,PlayBytes,10);lcd.setCursor(15,1);lcd.print(PlayBytes);} // // recálculo de decenas ... 10,20,30,40,50,60,70,80,90,110,120,..
          else 
          if (lcdsegs % (CNTRBASE*10) != 0) {itoa(lcdsegs%(CNTRBASE*10)/CNTRBASE*100,PlayBytes,10);lcd.setCursor(14,1);lcd.print(PlayBytes);} // recálculo de centenas ... 100,200,300,400,500,600,700,800,900,1100, ...
          lcdsegs++;                
        #elif defined(LCD20)
          String filename2 = fileName;
          lcd.setCursor(0,0); lcd.print("                    ");
          lcd.setCursor(0,0); lcd.print(filename2.substring(0,20));
          lcd.setCursor(0,1); lcd.print("Playing             ");
          lcd.setCursor(11,1); lcd.print(newpct); lcd.print("%");
          lcd.setCursor(17,1); lcd.print("000");
          if (lcdsegs % CNTRBASE != 0){itoa(lcdsegs%CNTRBASE,PlayBytes,10);lcd.setCursor(18,1);lcd.print(PlayBytes);} // recálculo de decenas ... 10,20,30,40,50,60,70,80,90,110,120,..
          else 
          if (lcdsegs % (CNTRBASE*10) != 0) {itoa(lcdsegs%(CNTRBASE*10)/CNTRBASE*100,PlayBytes,10);lcd.setCursor(17,1);lcd.print(PlayBytes);} // recálculo de centenas ... 100,200,300,400,500,600,700,800,900,1100, ...
          lcdsegs++;
        #elif defined(OLED1306)
          #ifdef XY
            setXY(0,0);
            printtext(fileName,0);
            setXY(0,1); sendStr((unsigned char *)"Playing");
            setXY(0,2); sendStr((unsigned char *)"----------------");
          #endif
          #ifdef XY2
            sendStrXY("Playing",0,0);
          #endif
        #endif 
        scrollPos=0;
        pauseOn = 0;       
        }
      oldMotorState=motorState;
      }
// Borrado de archivos
      if(button_delete() && start==1 && Delete==0) {
        stopFile();
        printtext(PlayBytes,1);
        debounce(button_delete);
      }
      if(button_delete() && start==0 && Delete==0) {
        getMaxFile();
        if (maxFile==0) {
          ShowDirEmpty();
          #ifdef OLED1306
            OledStatusLine();
          #endif
          #ifdef LCD20
            LCDBStatusLine();
          #endif
        } else {
          #if defined(LCD16)
            lcd.setCursor(0,0);  lcd.print("Delete File?    ");
            lcd.setCursor(0,1);  lcd.print(" Del Y - Stop N ");         
          #elif defined(LCD20)
            lcd.setCursor(0,1); lcd.print("Delete File?        ");
            lcd.setCursor(0,2); lcd.print("Y Press Delete again");
            lcd.setCursor(0,3); lcd.print("N Press Stop        ");
          #elif defined(OLED1306)
            setXY(0,0);
            printtext(fileName,0);
            OLED1306TXTDelete();       
          #endif
        Delete = 1;
        }
        debounce(button_delete);
      }     
      if(button_stop() && start==0 && Delete==1) {
        getMaxFile();
        if (maxFile==0) {
          #ifdef OLED1306
            clear_display();
            ShowDirEmpty();       
            OledStatusLine();
          #endif
          #ifdef LCD16
            lcd.clear();
            ShowDirEmpty();
          #endif       
          #ifdef LCD20
            lcd.clear();
            ShowDirEmpty();
            LCDBStatusLine();
          #endif
        } else {
          #ifdef LCD16
            lcd.clear();
          #endif
          #ifdef LCD20
            lcd.clear();
            LCDBStatusLine();
          #endif
          #ifdef OLED1306
           clear_display();
           OledStatusLine();
          #endif
          scrollPos=0;
          scrollText(fileName);
          printtext(PlayBytes,1);
        } 
        Delete = 0;
        debounce(button_stop);
      }
      if(button_root() && start==0 && Delete==1) {
        Delete = 0;
        debounce(btnRoot);
        resetFunc();
      }
      if(button_delete() && start==0 && Delete==1) {
        if(isDir==1) {
          #ifdef OLED1306
            clear_display();
          #endif
          #ifdef LCD20
            lcd.clear();
            LCDBStatusLine();         
          #endif
          CantDelDir();
          delay(1500);
          #ifdef OLED1306
            clear_display();
            OledStatusLine();
          #endif
          #ifdef LCD16
            lcd.clear();
          #endif       
          #ifdef LCD20
            lcd.clear();
            LCDBStatusLine();         
          #endif       
          seekFile();
        } else {
        getMaxFile();
          if (maxFile==0) {
            #ifdef OLED1306
              clear_display();
              ShowDirEmpty();
              OledStatusLine();
            #endif
            #ifdef LCD16
              lcd.clear();
              ShowDirEmpty();
            #endif         
            #ifdef LCD20
              lcd.clear();
              ShowDirEmpty();
              LCDBStatusLine();
            #endif
          } else {
            #ifdef LCD16
              lcd.clear();
              lcd.print("File Deleted    ");
            #endif
            #ifdef LCD20
              lcd.clear();
              lcd.print("File Deleted        ");
              LCDBStatusLine();
            #endif
            #ifdef OLED1306
              clear_display();
               sendStrXY("File Deleted ",0,0);
              OledStatusLine(); 
          #endif
            delay(1000);
            !sd.remove(fileName);
//          REWIND=0;  
            if (maxFile == numfiles) {
            seekFile();
            } else {
              seekFile();
//            downFile();
            }
          CheckDirEmpty();
          }
        }
        Delete = 0;
        debounce(button_delete);
      }
  // Fin de borrado de archivos y Reset
    #endif
  }  
}

void upFile() {    
  // move up a file in the directory.
  // find prior index, using currentFile.
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
    if (entry.open(&currentDir, currentFile, O_RDONLY))
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
  oldMinFile = 0;
  oldMaxFile = maxFile;
  currentFile++;
  if (currentFile>maxFile) { currentFile=0; }
  seekFile();
}

void upHalfSearchFile() {    
  //move up to half-pos between oldMinFile and currentFile

  if (currentFile >oldMinFile) {
    oldMaxFile = currentFile;
/*
    #ifdef SHOW_DIRNAMES
      str4cpy(oldMaxFileName,fileName);
    #endif
*/
    currentFile = oldMinFile + (oldMaxFile - oldMinFile)/2;
    seekFile();
  }
}

void downHalfSearchFile() {    
  //move down to half-pos between currentFile amd oldMaxFile

  if (currentFile <oldMaxFile) {
    oldMinFile = currentFile;
/*    
    #ifdef SHOW_DIRNAMES
      str4cpy(oldMinFileName,fileName);
    #endif
*/    
    currentFile = oldMinFile + 1+ (oldMaxFile - oldMinFile)/2;
    seekFile();
  } 
}

void seekFile() {    
  //move to a set position in the directory, store the filename, and display the name on screen.
  entry.close(); // precautionary, and seems harmless if entry is already closed
  
  while (!entry.open(&currentDir, currentFile, O_RDONLY))
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
  #ifdef AYPLAY
    ayblklen = filesize + 3;  // add 3 file header, data byte and chksum byte to file length
  #endif
  if(entry.isDir() || !strcmp(fileName, "ROOT")) { isDir=1; } else { isDir=0; }
  entry.close();
  PlayBytes[0]='\0'; 
  if (isDir==1) {
    if (subdir >0)strcpy(PlayBytes,prevSubDir);
//    else strcat_P(PlayBytes,PSTR(VERSION));
  } else {
    ltoa(filesize,PlayBytes,10);strcat_P(PlayBytes,PSTR(" bytes"));
  }
  printtext(PlayBytes,1);
  scrollPos=0;
  scrollText(fileName);
}

void stopFile() {
  //TZXStop();
  TZXStop();
  if(start==1){
    printtextF(PSTR("Stopped"),0);
    //lcd_clearline(0);
    //lcd.print(F("Stopped"));
    #ifdef P8544
      lcd.gotoRc(3,38);
      lcd.bitmap(Stop, 1, 6);
    #endif
    start=0;
  }
}

void playFile() {
  if(isDir==1) {
    changeDir();
  }
  else if (fileName[0] == '\0') 
  {
    #if defined(LCD16) || defined(LCD20)
      printtextF(PSTR("No File Selected"),1);
    #endif      
    #ifdef OLED1306
      printtextF(PSTR("No File Selected"),lineaxy);
    #endif

  }
  else
  {
    #ifdef LCD16
      String filename2 = fileName;
      lcd.setCursor(0,0); lcd.print(filename2.substring(0,16));
      lcd.setCursor(0,1); lcd.print("Playing         ");
    #endif
    #ifdef LCD20
      String filename2 = fileName;
      lcd.setCursor(0,0); lcd.print(filename2.substring(0,20));
      lcd.setCursor(0,1); lcd.print("Playing             ");
    #endif
    #ifdef OLED1306
      setXY(0,0);
      printtext(fileName,0);
      printtextF(PSTR("Playing"),0);
      setXY(0,2);
      sendStr((unsigned char *)"----------------");
    #endif 
    scrollPos=0;
    pauseOn = 0;
    scrollText(fileName);
    currpct=100;
    lcdsegs=0;
    UniPlay();
    start=1;       
  }    
}

void getMaxFile() {    
  // gets the total files in the current directory and stores the number in maxFile
  // and also gets the file index of the last file found in this directory
  currentDir.rewind();
  maxFile = 0;
  while(entry.openNext(&currentDir, O_RDONLY)) {
    maxFile = currentDir.curPosition()/32-1;
    entry.close();
  }
  currentDir.rewind(); // precautionary but I think might be unnecessary since we're using currentFile everywhere else now
  oldMaxFile = maxFile;
}

void changeDir() {    
  // change directory (to whatever is currently the selected fileName)
  // if fileName="ROOT" then return to the root directory
                      
  if(!strcmp(fileName, "ROOT"))
  {
    subdir=0;    
    changeDirRoot();
  } else {
     //if (subdir >0) entry.cwd()->getName(prevSubDir[subdir-1],filenameLength); // Antes de cambiar
     if (subdir < nMaxPrevSubDirs) {
      DirFilePos[subdir] = currentFile;
      subdir++;
      // but, also, stash the current filename as the parent (prevSubDir) for display
      // (but we only keep one prevSubDir, we no longer need to store them all)
      fileName[SCREENSIZE] = '\0';
      strcpy(prevSubDir, fileName);
    }
    tmpdir.open(&currentDir, currentFile, O_RDONLY);
    currentDir = tmpdir;
    tmpdir.close();
  }
  getMaxFile();
  currentFile=0;
  oldMinFile=0;
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
      tmpdir.open(&currentDir, DirFilePos[i], O_RDONLY);
      currentDir = tmpdir;
      tmpdir.close();
    }
    // get the filename of the last directory we opened, because this is now the prevSubDir for display
    currentDir.getName(fileName, filenameLength);
    fileName[SCREENSIZE] = '\0'; // copy only the piece we need. small cheat here - we terminate the string where we want it to end...
    strcpy(prevSubDir, fileName);
  }
   
  getMaxFile();
  currentFile = this_directory; // select the directory we were in, as the current file in the parent
  oldMinFile=0;
  seekFile(); // don't forget that this will put the real filename back into fileName 
}

void changeDirRoot()
{
  currentDir.close();
  currentDir.open("/", O_RDONLY);                    //set SD to root directory
}

void scrollText(char* text){
  #ifdef LCD16
  //Text scrolling routine.  Setup for 16x2 screen so will only display 16 chars
    if(scrollPos<0) scrollPos=0;
    char outtext[17];
    if(isDir) { outtext[0]= 0x3E; 
      for(int i=1;i<16;i++) {
        int p=i+scrollPos-1;
        if(p<strlen(text)) {
          outtext[i]=text[p];
        } else {
          outtext[i]='\0';
        }
      }
    } else { 
      for(int i=0;i<16;i++) {
        int p=i+scrollPos;
        if(p<strlen(text)) {
          outtext[i]=text[p];
        } else {
          outtext[i]='\0';
        }
      }
    }
    outtext[16]='\0';
    printtext(outtext,0);
  #endif

  #ifdef LCD20
    //Text scrolling routine.  Setup for 20x4 screen so will only display 20 chars
    if(scrollPos<0) scrollPos=0;
    char outtext[21];
    if(isDir) { outtext[0]= 0x3E; 
      for(int i=1;i<20;i++) {
        int p=i+scrollPos-1;
        if(p<strlen(text)) {
          outtext[i]=text[p];
        } else {
          outtext[i]='\0';
        }
      }
    } else { 
      for(int i=0; i<20; i++) {
        int p=i+scrollPos;
        if(p<strlen(text)) {
          outtext[i]=text[p];
        } else {
          outtext[i]='\0';
        }
      }
    }
    outtext[20]='\0';
    printtext(outtext,0);
  #endif

  #ifdef OLED1306
  //Text scrolling routine.
  if(scrollPos<0) scrollPos=0;
  char outtext[17];
  if(isDir) { outtext[0]= 0x3E; 
    for(int i=1;i<16;i++)
    {
      int p=i+scrollPos-1;
      if(p<strlen(text)) 
      {
        outtext[i]=text[p];
      } else {
        outtext[i]='\0';
      }
    }
  } else { 
    for(int i=0;i<16;i++)
    {
      int p=i+scrollPos;
      if(p<strlen(text)) 
        {
          outtext[i]=text[p];
        } else {
          outtext[i]='\0';
        }
      }
    }
    outtext[16]='\0';
    printtext(outtext,0);  
  #endif
}

void printtext2F(const char* text, int l) {  //Print text to screen. 
  #if defined(LCD16) || defined(LCD20)
    lcd.setCursor(0,l);
    char x = 0;
    while (char ch=pgm_read_byte(text+x)) {
      lcd.print(ch);
      x++;
    }
  #endif
  #ifdef OLED1306
    #ifdef XY2
      strncpy_P(fline, text, 16);
      sendStrXY(fline,0,l);
     #endif
     #ifdef XY 
      setXY(0,l);
      char x = 0;
      while (char ch=pgm_read_byte(text+x)) {
        sendChar(ch);
        x++;
      }
     #endif
  #endif
}

void printtextF(const char* text, int l) {  //Print text to screen. 
  #ifdef LCD16
    lcd.setCursor(0,1);
    char x = 0;
    while (char ch=pgm_read_byte(text+x)) {
      lcd.print(ch);
      x++;
    }
    for(x; x<16; x++) lcd.print(' ');  
  #endif
  #ifdef LCD20
    lcd.setCursor(0,l);
    char x = 0;
    while (char ch=pgm_read_byte(text+x)) {
      lcd.print(ch);
      x++;
    }
    for(x; x<20; x++) lcd.print(' ');  
  #endif
 #ifdef OLED1306
     #ifdef XY2
      strncpy_P(fline, text, 16);
      for(int i=strlen(fline);i<16;i++) fline[i]=0x20;
      sendStrXY(fline,0,1);
     #endif
     #ifdef XY 
      setXY(0,1);
      char x = 0;
      while (char ch=pgm_read_byte(text+x)) {
        sendChar(ch);
        x++;
      }
      for(x; x<16; x++) sendChar(' ');
     #endif
  #endif
}

void printtext(char* text, int l) {  //Print text to screen. 
  #ifdef LCD16
    lcd.setCursor(0,l);
    char ch;
    const char len = strlen(text);
    for(char x=0;x<16;x++) {
        if(x<len)  ch=text[x];
        else  ch=0x20;
        lcd.print(ch); 
    }
  #endif
  #ifdef LCD20
    lcd.setCursor(0,l);
    char ch;
    const char len = strlen(text);
    for(char x=0;x<20;x++) {
        if(x<len)  ch=text[x];
        else  ch=0x20;
        lcd.print(ch); 
    }
  #endif
  #ifdef OLED1306
    #ifdef XY2
      for(int i=0;i<16;i++)
      {
        if(i<strlen(text))  fline[i]=text[i];
        else  fline[i]=0x20;
      }    
      sendStrXY(fline,0,l);
    #endif
    #ifdef XY
      setXY(0,l); 
      char ch;
      const char len = strlen(text);
       for(char x=0;x<16;x++)
      {
        if(x<len)  ch=text[x];
        else  ch=0x20;
        sendChar(ch);
      }       
    #endif
  #endif
}

#ifdef OLED1306
  void OledStatusLine() {
    #ifdef XY
      setXY(0,2); sendStr((unsigned char *)"----------------");    
      // setXY(0,2); sendStr((unsigned char*)"                ");
      setXY(0,3); sendStr((unsigned char*)"ID:   BLK:  ");
      #ifdef OLED1306_128_64
        setXY(0,4);sendStr("Baud ---> ");
        setXY(0,5);sendStr("Motor --> ");
        setXY(0,6);sendStr("TSXCzx -> ");
        setXY(0,7);sendStr("Skip2A -> ");
        setXY(10,4);
        itoa(BAUDRATE,(char *)input,10);sendStr(input);
        setXY(10,5);
        if(mselectMask==1) sendStr((unsigned char *)"ON ");
        else sendStr((unsigned char *)"OFF");    
        setXY(10,6); 
        if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) sendStr((unsigned char *)"ON ");
        else sendStr((unsigned char *)"OFF");
        setXY(10,7); 
        if (skip2A == 1) sendStr((unsigned char *)"ON ");
        else sendStr((unsigned char *)"OFF");     
      #else
        setXY(0,7);
        //sendChar(48+BAUDRATE/1000); sendChar(48+(BAUDRATE/100)%10);sendChar(48+(BAUDRATE/10)%10);sendChar(48+BAUDRATE%10);
        itoa(BAUDRATE,(char *)input,10);sendStr(input);
        setXY(5,7);
        if(mselectMask==1) sendStr((unsigned char *)" M:ON");
        else sendStr((unsigned char *)"M:OFF");    
        setXY(11,7); 
        if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) sendStr((unsigned char *)" %^ON");
        else sendStr((unsigned char *)"%^OFF");
      #endif
    #endif
    #ifdef XY2                        // Y with double value
      #ifdef OLED1306_128_64          // 8 rows supported
        sendStrXY("ID:   BLK:  ",0,3);
        itoa(BAUDRATE,input,10);sendStrXY(input,10,4);
        if(mselectMask==1) sendStrXY("ON ",10,5);
        else sendStrXY("OFF",10,5);  
        if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) sendStrXY("ON ",10,6);
        else sendStrXY("OFF",10,6);           
      #else
      #endif
    #endif      
  }
#endif

void SetPlayBlock()
  {
  printtextF(PSTR(" "),0);
  #ifdef LCD16 ||LCD20
    lcd.setCursor(0,0);
    lcd.print(F("BLK:"));
    lcd.print(block);lcd.print(' ');
    lcd.print(F("ID:"));lcd.print(currentID,HEX); // Block ID en hex
  #endif
  #ifdef OLED1306
    #ifdef XY2
      utoa(block, (char *)input, 10);
      #if defined(OLEDBLKMATCH)              
      if (block <10) {input[1]=input[0];input[0]='0';input[2]=0;}
      if (block < 100) sendStrXY((char *)input,14,4);
      else sendStrXY((char *)(input+1),14,4);
    #endif                         
    sendStrXY("BLK:",0,0);
    sendStrXY((char *)input,4,0);//sendChar(' ');
    if (block < 100) sendStrXY(" ID:", 6,0);
    else sendStrXY(" ID:", 7,0);
    if (currentID/16 < 10) input[0]=48+currentID/16;
    else input[0]=55+currentID/16;
    if (currentID%16 < 10) input[1]=48+currentID%16;
    else input[1]=55+currentID%16;
    input[2]=0;
    if (block < 100) sendStrXY((char *)input,10,0);
    else sendStrXY((char *)input,11,0);
  #else
    utoa(block, (char *)input, 10);
    #if defined(OLEDBLKMATCH)              
      setXY(14,2);
      if (block <10) sendChar('0');
      if (block < 100) sendStr((char *)input);
      else sendStr((char *)(input+1));
    #endif                           
    setXY(0,0);
    sendStr("BLK:");
    sendStr((char *)input);//sendChar(' ');
    sendStr(" ID:");
    if (currentID/16 < 10) input[0]=48+currentID/16;
    else input[0]=55+currentID/16;
    if (currentID%16 < 10) input[1]=48+currentID%16;
    else input[1]=55+currentID%16;
    input[2]=0;sendStr((char *)input);
  #endif
  #endif      
  currpct=100; 
  lcdsegs=0;       
  currentBit=0;                               // fallo reproducción de .tap tras .tzx
  pass=0;
  if (!casduino) {
    currentBlockTask = READPARAM;               //First block task is to read in parameters
  #if defined(__AVR__)
    Timer1.setPeriod(1000);                     //set 1ms wait at start of a file.
  #elif defined(__arm__) && defined(__STM32F1__) 
    timer.setSTM32Period(1000);
  #else
    #error unknown timer
  #endif
  }
}

void GetAndPlayBlock()
{
   #ifdef BLOCKID_INTO_MEM
      bytesRead=blockOffset[block%maxblock];
      currentID=blockID[block%maxblock];   
   #endif
   #ifdef BLOCK_EEPROM_PUT
      #if defined(__AVR__)
        EEPROM.get(BLOCK_EEPROM_START+5*block, bytesRead);
        EEPROM.get(BLOCK_EEPROM_START+4+5*block, currentID);
      #elif defined(__arm__) && defined(__STM32F1__)
        EEPROM_get(BLOCK_EEPROM_START+5*block, &bytesRead);
        EEPROM_get(BLOCK_EEPROM_START+4+5*block, &currentID);
      #endif      
   #endif
   #ifdef BLOCKID_NOMEM_SEARCH 
      //block=1; //forcing block number for debugging
      
      unsigned long oldbytesRead;           //TAP
      bytesRead=0;                          //TAP 
      if (currentID!=TAP) bytesRead=10;   //TZX with blocks skip TZXHeader

      //int i=0;
      #ifdef BLKBIGSIZE
        int i = 0;
      #else
        byte i = 0;
      #endif      
      while (i<= block) {

        //if (currentID!=TAP) if(ReadByte(bytesRead)==1) currentID = outByte;  //TZX with blocks GETID
        if(ReadByte(bytesRead)==1){
          oldbytesRead = bytesRead-1;
          if (currentID!=TAP) currentID = outByte;  //TZX with blocks GETID
          if (currentID==TAP) bytesRead--;
        }
        else {block = i-1;break;}
        
        switch(currentID) {
          case ID10:  bytesRead+=2;     //Pause                
                      if(ReadWord(bytesRead)==2) bytesRead += outWord; //Length of data that follow
                      #if defined(OLEDBLKMATCH)
                        i++;
                      #endif
                      break;
          case ID11:  bytesRead+=15; //lPilot,lSynch1,lSynch2,lZ,lO,lP,LB,Pause
                      if(ReadLong(bytesRead)==3) bytesRead += outLong;
                      #if defined(OLEDBLKMATCH)
                        i++;
                      #endif                      
                      break;
          case ID12:  bytesRead+=4;
                      break;
          case ID13: if(ReadByte(bytesRead)==1) bytesRead += (long(outByte) * 2);
                      break;
          case ID14:  bytesRead+=7;
                      if(ReadLong(bytesRead)==3) bytesRead += outLong;
                      break;
          case ID15:  bytesRead+=5;
                      if(ReadLong(bytesRead)==3) bytesRead += outLong; 
                      break;
          case ID19:  if(ReadDword(bytesRead)==4) bytesRead += outLong;
                      #if defined(OLEDBLKMATCH) //&& defined(BLOCKID19_IN)
                        i++;
                      #endif          
                      break;                      
          case ID20:  bytesRead+=2;
                      break;
          case ID21:  if(ReadByte(bytesRead)==1) bytesRead += outByte;
                      #if defined(OLEDBLKMATCH) && defined(BLOCKID21_IN)
                        i++;
                      #endif          
                      break;
          case ID22:  
                      //#if defined(OLEDBLKMATCH) && defined(BLOCKID21_IN)
                      //  i++;
                      //#endif          
                      break;
          case ID24:  bytesRead+=2;
                      break;                                                                                
          case ID25:
                      break;
          case ID2A:  bytesRead+=4;
                      break;
          case ID2B:  bytesRead+=5;
                      break;
          case ID30:  if (ReadByte(bytesRead)==1) bytesRead += outByte;                                            
                      break;
          case ID31:  bytesRead+=1;         
                      if(ReadByte(bytesRead)==1) bytesRead += outByte; 
                      break;
          case ID32:  if(ReadWord(bytesRead)==2) bytesRead += outWord;
                      break;
          case ID33:  if(ReadByte(bytesRead)==1) bytesRead += (long(outByte) * 3);
                      break;
          case ID35:  bytesRead += 0x10;
                      if(ReadDword(bytesRead)==4) bytesRead += outLong;
                      break;
          case ID4B:  if(ReadDword(bytesRead)==4) bytesRead += outLong;
                      #if defined(OLEDBLKMATCH)
                        i++;
                      #endif          
                      break;
          case TAP:   if(ReadWord(bytesRead)==2) bytesRead += outWord;
                      #if defined(OLEDBLKMATCH) && defined(BLOCKTAP_IN)
                        i++;
                      #endif           
                      break;
        }
        #if not defined(OLEDBLKMATCH)
          i++;
        #endif  
      }

      //bytesRead=142; //forcing bytesRead for debugging      
      //ltoa(bytesRead,PlayBytes,16);printtext(PlayBytes,lineaxy);
              
   #endif   

   bytesRead= oldbytesRead;
   if (currentID==TAP) currentTask=PROCESSID;
   else {
    currentTask=GETID;    //Get new TZX Block
    if(ReadByte(bytesRead)==1) {currentID = outByte;currentTask=PROCESSID;}  //TZX with blocks GETID
   }
   
   SetPlayBlock(); 
}

void str4cpy (char *dest, char *src)
{
  char x =0;
  while (*(src+x) && (x<4)) {
       dest[x] = src[x];
       x++;
  }
  for(x; x<4; x++) dest[x]=' ';
  dest[4]=0; 
}

void GetFileName(uint16_t pos)
{
  entry.close(); // precautionary, and seems harmless if entry is already closed
  if (entry.open(&currentDir, pos, O_RDONLY))
  {
    entry.getName(fileName,filenameLength);
  }
  entry.close();
}

void Banner() {
  #ifdef OLED1306
    setXY(0,2);
    sendStr((unsigned char *)"----------------");
    setXY(1,3);
    sendStr((unsigned char *)VERSION);
    setXY(0,4);
    sendStr((unsigned char *)"----------------");
    delay(1500);
    reset_display();
  #elif defined(LCD16)
    lcd.setCursor(1,0); lcd.print(F(VERSION));
    delay(1800);
    lcd.clear();
  #elif defined(LCD20)
    lcd.setCursor(0,0); lcd.print("--------------------");
    lcd.setCursor(3,1); lcd.print(F(VERSION));
    lcd.setCursor(0,2); lcd.print("--------------------");
    delay(1800);
    lcd.clear();
  #endif
}

void NoSDCard() {
  #ifdef OLED1306
    setXY(0,1); sendStr((unsigned char *)"----------------");
    setXY(0,2); sendStr((unsigned char *)"   No SD Card   ");
    setXY(0,3); sendStr((unsigned char *)"----------------");
    setXY(0,4); sendStr((unsigned char *)"   Insert  SD   ");
    setXY(0,5); sendStr((unsigned char *)"----------------");        
  #elif defined(LCD16)
    lcd.setCursor(0,0); lcd.print("   No SD Card   ");
    lcd.setCursor(0,1); lcd.print("   Insert  SD   ");    
  #elif defined(LCD20)
    printtextF(PSTR("--------------------"),0);  
    printtextF(PSTR("     No SD Card     "),1);
    printtextF(PSTR(" Please, insert SD. "),2);    
    printtextF(PSTR("--------------------"),3);
  #endif
}

void SDCardOK() {
  #ifdef OLED1306
    clear_display();
    setXY(0,2); sendStr((unsigned char *)"----------------");
    setXY(0,3); sendStr((unsigned char *)"  SD Card - OK  ");
    setXY(0,4); sendStr((unsigned char *)"----------------");
  #elif defined(LCD16)
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("  SD Card - OK  ");
  #elif defined(LCD20)
    lcd.clear();
    printtextF(PSTR("--------------------"),0);    
    printtextF(PSTR("    SD Card - OK    "),1);
    printtextF(PSTR("--------------------"),2);
  #endif
}

void LCDBStatusLine() {
  #ifdef LCD20
    printtextF(PSTR("Baud:      Motor:   "),2);
    printtextF(PSTR("TSXC:      Skip2:   "),3);
    lcd.setCursor(5,2);
    lcd.print(BAUDRATE);
    lcd.setCursor(17,2);
    if(mselectMask==0)
      lcd.print("OFF");
    else
      lcd.print("ON");
      lcd.setCursor(5,3);
    if(TSXCONTROLzxpolarityUEFSWITCHPARITY==0)
      lcd.print("OFF");
    else
      lcd.print("ON");
      lcd.setCursor(17,3);
    if(skip2A==0)
      lcd.print("OFF");
    else
      lcd.print("ON");
  #endif
}

void CheckDirEmpty() {
  if (isDir==1 && Delete==1 && start==0 && pauseOn==0) {
    CantDelDir();
    delay(1500);
    #ifdef OLED1306
      OledStatusLine();
    #elif defined(LCD20)
      LCDBStatusLine();
    #endif    
    REWIND=1;
    seekFile();
  }
  if (isDir==0 && Delete==1 && start==0 && pauseOn==0) {
    getMaxFile();
    if (maxFile==0) {
      #ifdef OLED1306
        clear_display();
        ShowDirEmpty();
        OledStatusLine();
      #elif defined(LCD16)
        lcd.clear();
        ShowDirEmpty();
      #elif defined(LCD20)
        lcd.clear();
        ShowDirEmpty();
        LCDBStatusLine();
      #endif       
    }  
  }
  if (isDir==0 && Delete==0 && start==0 && pauseOn==0) {
    getMaxFile();
    if (maxFile==0) {
      #ifdef OLED1306
        clear_display();
        ShowDirEmpty();
        OledStatusLine();
      #endif
      #ifdef LCD16
        lcd.clear();
        ShowDirEmpty();
      #endif        
      #ifdef LCD20
        lcd.clear();
        ShowDirEmpty();
        LCDBStatusLine();
      #endif       
    }
  }
}

void OLED1306TXTDelete() {
  #ifdef OLED1306
    sendStrXY("----------------",0,1);         
    sendStrXY("  Delete File?  ",0,2);
    sendStrXY("----------------",0,3);
    sendStrXY("Y Press DL again",0,4);
    sendStrXY("N Press Stop    ",0,5);
    sendStrXY("----------------",0,6);
    sendStrXY("Root RESET Board",0,7);
  #endif
}

void ShowDirEmpty() {
  #ifdef OLED1306  
    sendStrXY("Directory Empty ",0,0);
    sendStrXY("STOP to go Back ",0,1);
  #elif defined(LCD16)
    lcd.setCursor(0,0); lcd.print("Directory Empty ");
    lcd.setCursor(0,1); lcd.print("STOP to go Back ");
  #elif defined(LCD20)
    lcd.setCursor(0,0); lcd.print("Directory Empty     ");
    lcd.setCursor(0,1); lcd.print("STOP to go Back     ");
  #endif
}

void CantDelDir() {
  #ifdef OLED1306
    sendStrXY("Can't Delete DIR",0,0);
    sendStrXY("Going Back      ",0,1);
  #elif defined(LCD16)
    lcd.setCursor(0,0); lcd.print("Can't Delete DIR");
    lcd.setCursor(0,1); lcd.print("Going Back      ");
  #elif defined(LCD20)
    lcd.setCursor(0,0); lcd.print("Can't Delete DIR    ");
    lcd.setCursor(0,1); lcd.print("Going Back          ");
  #endif
}

void RewindSD() {
  #ifdef SDFat
    entry.cwd()->rewind();
  #else
  switch(subdir) {
    case 0:
      strcat(fileName,"/");
    break;
    case 1:          
      strcat(strcat(fileName,"/"),prevSubDir[0]);
    break;
    case 2:
      default:
      strcat(strcat(strcat(strcat(fileName,"/"),prevSubDir[0]),"/"),prevSubDir[1]);
    break;        
  }
//  cwdentry = SD.open(fileName);
//  cwdentry.rewindDirectory();
  #endif
}
