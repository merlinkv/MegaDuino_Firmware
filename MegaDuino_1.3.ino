#define VERSION "MegaDuino 1.3"
//
// Original firmware writen by Andrew Beer, Duncan Edwards, Rafael Molina and others.
//
// MegaDuino Firmware is an adaptation of the MaxDuino firmware by Rafael Molina but is specially designed for my MegaDuino & MegaDuino STM32 projects
//
//
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

#define SDFat

#ifdef TimerOne
  #include <TimerOne.h>
#elif defined(__arm__) && defined(__STM32F1__)
  HardwareTimer timer(2); // channer 2  
  #include "itoa.h"  
  #define strncpy_P(a, b, n) strncpy((a), (b), (n))
  #define memcmp_P(a, b, n) memcmp((a), (b), (n))  
#else
  #include "TimerCounter.h"
  TimerCounter Timer1;              // preinstatiate
  
  unsigned short TimerCounter::pwmPeriod = 0;
  unsigned char TimerCounter::clockSelectBits = 0;
  void (*TimerCounter::isrCallback)() = NULL;
  
  // interrupt service routine that wraps a user defined function supplied by attachInterrupt
  ISR(TIMER1_OVF_vect)
  {
    Timer1.isrCallback();
  }
#endif

#ifdef SDFat
  #include <SdFat.h>
#else
  #include <SD.h>
  #define SdFile File
  #define SdFat SDClass
  #define chdir open
  #define openNext openNextFile
  #define isDir() isDirectory()
  #define fileSize size
  #define seekSet seek
  File cwdentry;
#endif

#include <EEPROM.h>

#if defined(__AVR_ATmega2560__)
  #include "MEGA2560config.h"
#else
  #include "STM32config.h"  
#endif  
#include "MegaDuino.h"

#ifdef LCD16
  #include <Wire.h> 
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR,16,2); // set the LCD address to 0x27 for a 16 chars and 2 line display
  char indicators[] = {'|', '/', '-',0};
  uint8_t SpecialChar [8]= { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
  #define SCREENSIZE 16
#endif

#ifdef LCD20
  #include <Wire.h> 
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR,20,4); // set the LCD address to 0x27 for a 20 chars and 4 line display
  char indicators[] = {'|', '/', '-',0};
  uint8_t SpecialChar [8]= { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
  #define SCREENSIZE 20
  #define maxblock 10
#endif

#ifdef OLED1306 
  #include <Wire.h> 
  char indicators[] = {'|', '/', '-',92};
  #define SCREENSIZE 16
#endif

char fline[17];

SdFat sd;                           //Initialise Sd card 
SdFile entry;                       //SD card file

#define subdirLength     22         // hasta 62 no incrementa el consumo de RAM
#define filenameLength   4*subdirLength  //Maximum length for scrolling filename, hasta 255 no incrementa consumo RAM

char fileName[filenameLength + 1];    //Current filename
char sfileName[13];                   //Short filename variable
char prevSubDir[3][subdirLength];    // Subir a la EPROM ¿?
int DirFilePos[3];                   //File Positios in Directory to be restored
byte subdir = 0;
unsigned long filesize;             // filesize used for dimensioning AY files

#if defined(__AVR_ATmega2560__)
  const byte chipSelect = 53;         // Sd card chip select pin
  #define btnUp         A0            // Up button
  #define btnDown       A1            // Down button
  #define btnPlay       A2            // Play Button
  #define btnStop       A3            // Stop Button
  #define btnRoot       A4            // Return to SD card root
  #define btnDelete     A5            // Two options: Delete files or reset the board
  #define btnMotor      6             // Motor Sense (connect pin to gnd to play, NC for pause)
#endif

#if defined(__arm__) && defined(__STM32F1__)
  #define chipSelect    PB12           // Sd card chip select pin
  #define btnUp         PA3            // Up button
  #define btnDown       PA2            // Down button
  #define btnPlay       PA1            // Play Button
  #define btnStop       PA0            // Stop Button
  #define btnRoot       PB9            // Return to SD card root
  #define btnDelete     PA4            // Two options: Delete files or reset the board
  #define btnMotor      PA8            // Motor Sense (connect pin to gnd to play, NC for pause)
#endif

#define scrollSpeed   250           //text scroll delay
#define scrollWait    3000          //Delay before scrolling starts

byte scrollPos = 0;                 //Stores scrolling text position
unsigned long scrollTime = millis() + scrollWait;


byte motorState = 1;                //Current motor control state
byte oldMotorState = 1;             //Last motor control state
byte start = 0;                     //Currently playing flag

byte pauseOn = 0;                   //Pause state
int currentFile = 1;                //Current position in directory
int maxFile = 0;                    //Total number of files in directory
int oldMinFile = 1;
int oldMaxFile = 0;
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
char PlayBytes[subdirLength];

unsigned long blockOffset[maxblock];
byte blockID[maxblock];

byte lastbtn=true;

//#if (SPLASH_SCREEN && TIMEOUT_RESET)
void(* resetFunc) (void) = 0;//declare reset function at adress 0
//#endif

void setup() {
  #ifdef LCD16
    lcd.begin();                     //Initialise LCD (16x2 type)    
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0,0);
    Banner();
  #endif
  #ifdef LCD20
    lcd.begin();
    lcd.backlight();
    lcd.clear();
    Banner();
    lcd.createChar(0, SpecialChar);
  #endif  
  #ifdef OLED1306 
    Wire.begin();
    init_OLED();
    delay(2000);              // Show logo
    reset_display();           // Clear logo and load saved mode
    Banner();
  #endif

  //General Pin settings
  //Setup buttons with internal pullup

  #if defined(__AVR_ATmega2560__)
    PORTF |= _BV(0); // Analog pin A0
    PORTF |= _BV(1); // Analog pin A1
    PORTF |= _BV(2); // Analog pin A2
    PORTF |= _BV(3); // Analog pin A3
    PORTF |= _BV(4); // Analog pin A4
    PORTF |= _BV(5); // Analog pin A5
    PORTH |= _BV(3); // Digital pin 6
  #endif

  #if defined(__arm__) && defined(__STM32F1__)
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
  #ifdef SPLASH_SCREEN
      while (digitalRead(btnPlay) == HIGH & 
             digitalRead(btnStop) == HIGH &
             digitalRead(btnUp)   == HIGH &
             digitalRead(btnDown) == HIGH &
             digitalRead(btnRoot) == HIGH &
             digitalRead(btnDelete) == HIGH)
      {
        delay(100);              // Show logo (OLED) or text (LCD) and remains until a button is pressed.
      }   
      #ifdef OLED1306    
          reset_display();           // Clear logo and load saved mode
      #endif
  #endif
  
  pinMode(chipSelect, OUTPUT);      //Setup SD card chipselect pin

  #ifdef SDFat
    while (!sd.begin(chipSelect,SPI_FULL_SPEED)) {   
      NoSDCard();
    }
  #else
    while (!SD.begin(chipSelect)) {   
      NoSDCard();
    }
  #endif         

  UniSetup();                       //Setup TZX specific options
  delay(1000);
  #ifdef OLED1306
    clear_display();
    setXY(0,0); sendStr((unsigned char *)"----------------");
    setXY(0,1); sendStr((unsigned char *)"  SD Card - OK  ");
    setXY(0,2); sendStr((unsigned char *)"----------------");
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
  delay(2000);
  loadEEPROM();
  #ifdef OLED1306
    reset_display();
  #endif
  #ifdef LCD16 || LCD20
    lcd.clear();
  #endif
  #ifdef LCD20
    LCDBStatusLine();
  #endif
  #ifdef OLED1306
    OledStatusLine();
  #endif  
  getMaxFile();                     //get the total number of files in the directory
  seekFile(currentFile);            //move to the first file in the directory
}

void loop(void) {
  if(start==1)
  {
    uniLoop();
  } else {
    WRITE_LOW;    
  }
  if((millis()>=scrollTime) && start==0 && (strlen(fileName)> SCREENSIZE)) {
    scrollTime = millis()+scrollSpeed;
    scrollText(fileName,0);
    scrollPos +=1;
    if(scrollPos>strlen(fileName)) {
      scrollPos=0;
      scrollTime=millis()+scrollWait;
      scrollText(fileName,0);
    }
  }
  motorState=digitalRead(btnMotor);
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
      
     if(digitalRead(btnPlay) == LOW) {
        //Handle Play/Pause button
        if(start==0) {
          //If no file is play, start playback
          playFile();
          if (mselectMask == 1){  
            //oldMotorState = !motorState;  //Start in pause if Motor Control is selected
              oldMotorState = 0;
          }
          delay(200);
        } else {
          //If a file is playing, pause or unpause the file                  
          #ifdef LCD16
            String filename2 = fileName;
            lcd.setCursor(0,0); lcd.print("                ");
            lcd.setCursor(0,0); lcd.print(filename2.substring(0,16));
          #endif
          #ifdef LCD20
            String filename2 = fileName;
            lcd.setCursor(0,0); lcd.print("                    ");
            lcd.setCursor(0,0); lcd.print(filename2.substring(0,20));
          #endif          
          #ifdef OLED1306
            if (pauseOn == 0){
              printtextF(PSTR("Paused"),1);
            } else {
                printtextF(PSTR("Playing"),1);
                setXY(0,2);
                sendStr((unsigned char *)"----------------");
            }
          #else            
            if (pauseOn == 0) {
              printtextF(PSTR("Paused"),1); 
              firstBlockPause = true;
            } else {
              printtextF(PSTR("Playing"),1);
              firstBlockPause = false; 
            }
          #endif
          #ifdef LCD16            
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
          #endif
          #ifdef LCD20
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
          #endif
          #ifdef OLED1306
            //sprintf(PlayBytes,"Paused % 3d%%  %03d",newpct,lcdsegs%1000); sendStrXY(PlayBytes,0,0);
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
            setXY(13,1);
            sendStr((unsigned char *)(PlayBytes));
          #endif
          pauseOn = !pauseOn;
       }
       CheckDirEmpty();
       debounce(btnPlay);         
     }

#ifdef ONPAUSE_POLCHG
  if(digitalRead(btnRoot)==LOW && start==1 && pauseOn==1
    #ifdef btnRoot_AS_PIVOT
      && digitalRead(btnStop)==LOW   
    #endif
    ){
    // change tsx speed control/zx polarity/uefTurboMode
    TSXCONTROLzxpolarityUEFSWITCHPARITY = !TSXCONTROLzxpolarityUEFSWITCHPARITY;
    #ifdef OLED1306
      printtextF(PSTR("                "),1); 
      OledStatusLine();
    #endif
    #ifdef LCD16
      printtextF(PSTR("                "),1);
    #endif       
    #ifdef LCD20
      printtextF(PSTR("                    "),1);
      LCDBStatusLine();
    #endif
    debounce(btnRoot);  
  }
#endif

#ifdef btnRoot_AS_PIVOT
     //checkLastButton();
     //if(digitalRead(btnDown) && digitalRead(btnUp) && digitalRead(btnPlay) && digitalRead(btnStop)) lastbtn=false;
     lastbtn=false;     
     if(digitalRead(btnRoot)==LOW && start==0 && !lastbtn) {                                          // show min-max dir
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
           else lcd.print(F("M:OFF"));
           lcd.print(' ');
           if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) lcd.print(F(" %^ON"));
           else lcd.print(F("%^OFF"));         
        #endif 
        #if defined(LCD16) && defined(SHOW_DIRNAMES)
          str4cpy(input,fileName);
          GetFileName(oldMinFile); str4cpy(oldMinFileName,fileName);
          //GetFileName(currentFile); str4cpy(input,fileName); 
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
          //GetFileName(currentFile); str4cpy(input,fileName); 
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
          sendStr(input);sendChar('<');sendChar(' ');
          sendStr(oldMaxFileName);
           
        #endif
      #endif        
        while(digitalRead(btnRoot)==LOW && !lastbtn) {
           lastbtn = 1;
           checkLastButton();           
        }        
        printtext(PlayBytes,0);
     }
     #if defined(LCD16) && defined(SHOW_BLOCKPOS_LCD)
       if(digitalRead(btnRoot)==LOW && start==1 && pauseOn==1 && !lastbtn) {                                          // show min-max block
        lcd.setCursor(11,0);
         if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) lcd.print(F(" %^ON"));
        else lcd.print(F("%^OFF"));  
        while(digitalRead(btnRoot)==LOW && start==1 && !lastbtn) {
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
       if(digitalRead(btnRoot)==LOW && start==1 && pauseOn==1 && !lastbtn) {                                          // show min-max block
        lcd.setCursor(13,0);
         if (TSXCONTROLzxpolarityUEFSWITCHPARITY == 1) lcd.print(F(" %^ON"));
        else lcd.print(F("%^OFF"));  
        while(digitalRead(btnRoot)==LOW && start==1 && !lastbtn) {
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

if(digitalRead(btnRoot)==LOW && start==0
  #ifdef btnRoot_AS_PIVOT
    && digitalRead(btnStop)==LOW
  #endif        
  ){
  #if (SPLASH_SCREEN && TIMEOUT_RESET)
    timeout_reset = TIMEOUT_RESET;
  #endif
  #if defined(Use_MENU) && !defined(RECORD_EEPROM_LOGO)
    menuMode();
    #ifdef LCD16 || LCD20
      lcd.setCursor(0,0);
      printtext(PlayBytes,1);          
      printtextF(PSTR(""),1);
    #endif
    #ifdef OLED1306
      printtext(PlayBytes,0);
      printtextF(PSTR(""),lineaxy);
    #endif
    scrollPos=0;
    scrollText(fileName,0);     
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
      OledStatusLine();
      delay(2000)
    #else             
      subdir=0;
      prevSubDir[0][0]='\0';
      prevSubDir[1][0]='\0';
      prevSubDir[2][0]='\0';
      sd.chdir("/");
      getMaxFile();
      currentFile=1;
      seekFile(currentFile);
    #endif         
  #endif
  debounce(btnRoot);  
}

if(digitalRead(btnStop)==LOW && start==1
  #ifdef btnRoot_AS_PIVOT
    && digitalRead(btnRoot)
  #endif
  ){      
  stopFile();
  debounce(btnStop);
  #ifdef LCD16
    lcd.setCursor(0,1); lcd.print("                ");  
  #endif  
  #ifdef LCD20
    lcd.setCursor(0,1); lcd.print("                    ");  
    LCDBStatusLine();
  #endif
  #ifdef OLED1306
    OledStatusLine();
  #endif     
  ltoa(filesize,PlayBytes,10);printtext(strcat_P(PlayBytes,PSTR(" bytes")),1);  
  }
  if(digitalRead(btnStop)==LOW && start==0 && subdir >0) {                                         // back subdir
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif     
    fileName[0]='\0';
    subdir--;
    prevSubDir[subdir][0]='\0';     
    switch(subdir){
      case 0:
        sd.chdir("/");
      break;
      case 1:
        sd.chdir(strcat(strcat(fileName,"/"),prevSubDir[0]));
      break;
      case 2:
        default:
          subdir = 2;
          sd.chdir(strcat(strcat(strcat(strcat(fileName,"/"),prevSubDir[0]),"/"),prevSubDir[1]));
      break;
    }
       //Return to prev Dir of the SD card.
       //sd.chdir(fileName,true);
       //sd.chdir("/CDT");       
       //printtext(prevDir,0); //debug back dir
       
    getMaxFile();
       //currentFile=1;
    currentFile=DirFilePos[subdir];
    oldMinFile =1;   // Check and activate when new space for OLED
    REWIND=1;
    seekFile(currentFile);
    DirFilePos[subdir]=0;
    debounce(btnStop);   
  }     

#ifdef BLOCKMODE
  if(digitalRead(btnUp)==LOW && start==1 && pauseOn==1             //  up block sequential search
    #ifdef btnRoot_AS_PIVOT
      && digitalRead(btnRoot)
    #endif
  ){
    firstBlockPause = false;
    #ifdef BLOCKID_INTO_MEM
      oldMinBlock = 0;
      oldMaxBlock = maxblock;
      if (block > 0) block--;
      else block = maxblock;      
    #endif
    #ifdef BLOCK_EEPROM_PUT
      oldMinBlock = 0;
      oldMaxBlock = 99;
      if (block > 0) block--;
      else block = 99;
    #endif
    GetAndPlayBlock();       
    debounce(btnUp);         
  }
#endif    

#if defined(BLOCKMODE) && defined(btnRoot_AS_PIVOT)
  if(digitalRead(btnUp)==LOW && start==1 && pauseOn==1 && digitalRead(btnRoot)==LOW) {         // up block half-interval search
    if (block >oldMinBlock) {
      oldMaxBlock = block;
      block = oldMinBlock + (oldMaxBlock - oldMinBlock)/2;
    }
    GetAndPlayBlock();    
    debounce(btnUp);         
  }
#endif

if(digitalRead(btnUp)==LOW && start==0   // up dir sequential search
  #ifdef btnRoot_AS_PIVOT
    && digitalRead(btnRoot)
  #endif
  ){                         // up dir sequential search   
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif
       //Move up a file in the directory
    scrollTime=millis()+scrollWait;
    scrollPos=0;
    upFile();
    debouncemax(btnUp);   
   }

#ifdef btnRoot_AS_PIVOT
     if(digitalRead(btnUp)==LOW && start==0 && digitalRead(btnRoot)==LOW) {                     // up dir half-interval search
       #if (SPLASH_SCREEN && TIMEOUT_RESET)
            timeout_reset = TIMEOUT_RESET;
       #endif
       //Move up a file in the directory
       scrollTime=millis()+scrollWait;
       scrollPos=0;
       upHalfSearchFile();
       debounce(btnUp);       
     }
#endif

#ifdef BLOCKMODE
  if(digitalRead(btnDown)==LOW && start==1 && pauseOn==1 // down block sequential search
    #ifdef btnRoot_AS_PIVOT
      && digitalRead(btnRoot)
    #endif
    ){      // down block sequential search  
      #ifdef BLOCKID_INTO_MEM
        oldMinBlock = 0;
        oldMaxBlock = maxblock;
        if (block < maxblock) block++;
        else block = 0;       
      #endif
      #ifdef BLOCK_EEPROM_PUT
        oldMinBlock = 0;
        oldMaxBlock = 99;
        if (block < 99) block++;
        else block = 0;
      #endif
      GetAndPlayBlock();    
      debounce(btnDown);                  
    }
#endif
#if defined(BLOCKMODE) && defined(btnRoot_AS_PIVOT)
     if(digitalRead(btnDown)==LOW && start==1 && pauseOn==1 && digitalRead(btnRoot)==LOW) {     // down block half-interval search
        if (block <oldMaxBlock) {
          oldMinBlock = block;
          block = oldMinBlock + 1+ (oldMaxBlock - oldMinBlock)/2;
        } 
       GetAndPlayBlock();    
       debounce(btnDown);                  
     }
#endif

if(digitalRead(btnDown)==LOW && start==0
  #ifdef btnRoot_AS_PIVOT
    && digitalRead(btnRoot)
  #endif
  ){                    // down dir sequential search     
    #if (SPLASH_SCREEN && TIMEOUT_RESET)
      timeout_reset = TIMEOUT_RESET;
    #endif
    //Move down a file in the directory
    scrollTime=millis()+scrollWait;
    scrollPos=0;
    downFile();
    debouncemax(btnDown);
    }

#ifdef btnRoot_AS_PIVOT
     if(digitalRead(btnDown)==LOW && start==0 && digitalRead(btnRoot)==LOW) {              // down dir half-interval search
       #if (SPLASH_SCREEN && TIMEOUT_RESET)
            timeout_reset = TIMEOUT_RESET;
       #endif
       //Move down a file in the directory
       scrollTime=millis()+scrollWait;
       scrollPos=0;
       downHalfSearchFile();
       debounce(btnDown);      
     }
#endif
     if(start==1 && (!oldMotorState==motorState) && mselectMask==1 ) {  
       //if file is playing and motor control is on then handle current motor state
       //Motor control works by pulling the btnMotor pin to ground to play, and NC to stop
       if(motorState==1 && pauseOn==0){
         #ifdef LCD16
           lcd.setCursor(0,1); lcd.print("---PRESS PLAY---");    
         #endif
         #ifdef LCD20
           lcd.setCursor(0,1); lcd.print("-----PRESS PLAY-----");    
         #endif  
         #ifdef OLED1306
           #ifdef XY
             setXY(0,0); printtext(fileName,0);
             setXY(0,2); sendStr((unsigned char *)"---PRESS PLAY---");
           #endif
           #ifdef XY2
             sendStrXY("PAUSED ",0,0);
           #endif
         #endif 
         scrollPos=0;
         //scrollText(fileName);
         pauseOn=1;
       }
       if(motorState==0 && pauseOn==1) {
         //printtextF(PSTR("PLAYing"),0);
         #ifdef LCD16
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
         #endif 
         #ifdef LCD20
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
         #endif
         #ifdef OLED1306
            #ifdef XY
              setXY(0,0);
              printtext(fileName,0);
              setXY(0,1);
              sendStr((unsigned char *)"Playing");
              setXY(0,2);
              sendStr((unsigned char *)"----------------");
            #endif
            #ifdef XY2
              sendStrXY("Playing",0,0);
            #endif
         #endif 
         scrollPos=0;
//         scrollText(fileName);
         pauseOn = 0;       
       }
       oldMotorState=motorState;
     }
      
   }
// Borrado de archivos y Reset

   if(digitalRead(btnDelete)==LOW && start==1 && Delete==0) {
     stopFile();
     printtext(PlayBytes,1);
     debounce(btnDelete);
   }
   if(digitalRead(btnDelete)==LOW && start==0 && Delete==0) {
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
       #ifdef LCD16
         lcd.setCursor(0,0);  lcd.print("Delete File?    ");
         lcd.setCursor(0,1);  lcd.print(" Del Y - Stop N ");         
       #endif
       #ifdef LCD20
         lcd.setCursor(0,1); lcd.print("Delete File?        ");
         lcd.setCursor(0,2); lcd.print("Y Press Delete again");
         lcd.setCursor(0,3); lcd.print("N Press Stop        ");
       #endif
       #ifdef OLED1306
         setXY(0,0);
         printtext(fileName,0);
         OLED1306TXTDelete();       
       #endif
     Delete = 1;
     }
     debounce(btnDelete);
   }
   if(digitalRead(btnStop)==LOW && start==0 && Delete==1) {
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
       scrollText(fileName,0);
       printtext(PlayBytes,1);
     } 
     Delete = 0;
     debounce(btnStop);
   }
   if(digitalRead(btnRoot)==LOW && start==0 && Delete==1) {
     Delete = 0;
     debounce(btnRoot);
     resetFunc();
   }
   if(digitalRead(btnDelete)==LOW && start==0 && Delete==1) {
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
       seekFile(currentFile);
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
//         REWIND=0;  
         if (maxFile == numfiles) {
           seekFile(1);
         } else {
           seekFile(currentFile);
//         downFile();
         }
       CheckDirEmpty();
       }
     }
     Delete = 0;
     debounce(btnDelete);
   }
//   if((digitalRead(btnUp)==LOW || digitalRead(btnDown)==LOW || digitalRead(btnPlay)==LOW || digitalRead(btnRoot)==LOW) && start==0 && Delete==1) {
//     debounce(btnUp);
//     debounce(btnDown);
//     debounce(btnPlay);
//     debounce(btnRoot);
//   }
// Fin de borrado de archivos y Reset

} 
    

void debounce(int boton){
  while(digitalRead(boton)==LOW){
    //prevent button repeats by waiting until the button is released.
    delay(50);
  }
}

void debouncemax(int boton){
  timeDiff2 = millis();
  while ((digitalRead(boton)==LOW) && (millis() - timeDiff2 < 200)) {
    //prevent button repeats by waiting until the button is released.
    delay(50);
  }
}

void upFile() {    
  //move up a file in the directory
  oldMinFile = 1;
  oldMaxFile = maxFile;
  filenum = filenum - 1;
  currentFile--;
  if(currentFile<1) {
    getMaxFile();
    currentFile = maxFile;
  }
  REWIND=1;   
  seekFile(currentFile);
}

void downFile() {    
  //move down a file in the directory
  oldMinFile = 1;
  oldMaxFile = maxFile;
  filenum = filenum + 1;  
  currentFile++;
  if(currentFile>maxFile) { currentFile=1; }
  REWIND=1;  
  seekFile(currentFile);
}

void upHalfSearchFile() {    
  if (currentFile >oldMinFile) {
    oldMaxFile = currentFile;
    currentFile = oldMinFile + (oldMaxFile - oldMinFile)/2;
    REWIND=1;   
    seekFile(currentFile);
  }
}

void downHalfSearchFile() {    
  //move down to half-pos between currentFile amd oldMaxFile
  if (currentFile <oldMaxFile) {
    oldMinFile = currentFile;
    currentFile = oldMinFile + 1 + (oldMaxFile - oldMinFile)/2;
    REWIND=1;  
    seekFile(currentFile);
  } 
}

void seekFile(int pos) {    
  //move to a set position in the directory, store the filename, and display the name on screen.
  if (REWIND==1) {  
    RewindSD();
    for(int i=1;i<=pos-1;i++) {
  #ifdef SDFat
      entry.openNext(entry.cwd(),O_READ);
      entry.close();
  #else
      entry=cwdentry.openNextFile();
      entry.close();
  #endif
    }
  }
  if (pos==1) {RewindSD();}
  #ifdef SDFat
    entry.openNext(entry.cwd(),O_READ);
    entry.getName(fileName,filenameLength);
    entry.getSFN(sfileName);
  #else
    entry = cwdentry.openNextFile();
    char* sfileName=entry.name();
    char* fileName=sfileName;
  #endif
  filesize = entry.fileSize();
  #ifdef AYPLAY
    ayblklen = filesize + 3;  // add 3 file header, data byte and chksum byte to file length
  #endif
  if(entry.isDir() || !strcmp(sfileName, "ROOT")) { isDir=1; } else { isDir=0; }
  entry.close();
  PlayBytes[0]='\0'; 
  if (isDir==1) {
    #ifdef LCD16 || LCD20 || OLED1306
      if (subdir >0) strcpy(PlayBytes,prevSubDir[subdir-1]);
    #endif
    #ifdef OLED1306_128_64
      setXY(0,1);
      sendStr((unsigned char *)"                ");
    #endif
    #ifdef LCD16
      lcd.setCursor(0,1); lcd.print("                ");
    #endif    
    #ifdef LCD20
      lcd.setCursor(0,1); lcd.print("                    ");
    #endif
  } else {
    ltoa(filesize,PlayBytes,10); printtext(strcat_P(PlayBytes,PSTR(" bytes")),1);
  }
  //PlayBytes[0]='\0'; itoa(DirFilePos[0],PlayBytes,10);
  //printtext(prevSubDir[0],0);
  scrollPos=0;
  scrollText(fileName,0);
  printtext(PlayBytes,1);  
}

void stopFile() {
  //TZXStop();
  TZXStop();
  if(start==1){
    #ifdef OLED1306
      printtextF(PSTR("Stopped"),1);
    #else
      printtextF(PSTR("Stopped"),0);
    #endif
    //lcd_clearline(0);
    //lcd.print(F("Stopped"));
    start=0;
  }
}

void playFile() {
  //PlayBytes[0]='\0';
  //strcat_P(PlayBytes,PSTR("Playing "));ltoa(filesize,PlayBytes+8,10);strcat_P(PlayBytes,PSTR("B"));  
  if(isDir==1) {
    //If selected file is a directory move into directory
    changeDir();
  } else {
  #ifdef SDFat
    if(entry.cwd()->exists(sfileName)) {
  #else
    if(SD.exists(sfileName)) {
  #endif
      #ifdef LCD16
          String filename2 = fileName;
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print(filename2.substring(0,16));
          lcd.setCursor(0,1); lcd.print("                ");
          lcd.setCursor(0,1); lcd.print("Playing");
      #endif
      #ifdef LCD20
        String filename2 = fileName;
        lcd.setCursor(0,0); lcd.print("                    ");
        lcd.setCursor(0,0); lcd.print(filename2.substring(0,20));
        lcd.setCursor(0,1); lcd.print("                    ");
        lcd.setCursor(0,1); lcd.print("Playing");
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
//      scrollText(fileName);
      currpct=100;
      lcdsegs=0;
      UniPlay(sfileName);           //Load using the short filename
      start=1;       
    } else {
      #ifdef LCD16 || LCD20 || OLED1306
        CheckDirEmpty();
      #endif
    }
  }
}

void getMaxFile() {    
  //gets the total files in the current directory and stores the number in maxFile
  
#ifdef SDFat
  RewindSD();
  maxFile=0;
  while(entry.openNext(entry.cwd(),O_READ)) {
    //entry.getName(fileName,filenameLength);
    entry.close();
    maxFile++;
  }
#else
  //RewindSD();

  while(entry=cwdentry.openNextFile()) {
    //entry.getName(fileName,filenameLength);
    entry.close();
    maxFile++;
  }

#endif
  oldMaxFile = maxFile;
  numfiles = maxFile;
  //entry.cwd()->rewind();
}

void changeDir() {    
  //change directory, if fileName="ROOT" then return to the root directory
  //SDFat has no easy way to move up a directory, so returning to root is the easiest way. 
  //each directory (except the root) must have a file called ROOT (no extension)
  if(!strcmp(fileName, "ROOT")) {
    subdir=0;    
    sd.chdir("/");
  } else {
     //if (subdir >0) entry.cwd()->getName(prevSubDir[subdir-1],filenameLength); // Antes de cambiar
     DirFilePos[subdir] = currentFile;
     sd.chdir(fileName);
     if (strlen(fileName) > subdirLength) {
      //entry.getSFN(sfileName);
      strcpy(prevSubDir[subdir], sfileName);
     }
     else {
      strcpy(prevSubDir[subdir], fileName);
     }
     
     //entry.cwd()->getName(prevSubDir[subdir],filenameLength);
     //entry.getSFN(sfileName);
     //strcpy(prevSubDir[subdir], sfileName);
     //strcpy(prevSubDir[subdir], fileName);
     
     subdir++;      
  }
  getMaxFile();
/*
  #ifdef SHOW_DIRNAMES
    oldMaxFile=maxFile;  
    REWIND=1;
    seekFile(oldMaxFile);
    str4cpy(oldMaxFileName,fileName);    
  #endif
*/
  oldMinFile=1;  // Cheack and activate when new space for OLED
  currentFile=1;
  seekFile(currentFile);

/*
  #ifdef SHOW_DIRNAMES
    str4cpy(oldMinFileName,fileName);
  #endif
*/  
}

void scrollText(char* text, int rowtxt){
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
    printtext(outtext,rowtxt);
  //lcd_clearline(1);
  //lcd.print(outtext);
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
    printtext(outtext,rowtxt);
//    lcd_clearline(1);
//    lcd.print(outtext);
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
  #ifdef LCD16 || LCD20
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

void SetPlayBlock(){
  printtextF(PSTR(" "),0);
  #ifdef LCD16 || LCD20
    lcd.setCursor(0,0);
    lcd.print(F("BLK:"));
    lcd.print(block);lcd.print(' ');
    if (bytesRead > 0){
      lcd.print(F("ID:"));lcd.print(currentID,HEX); // Block ID en hex
    }
    #endif
    #ifdef OLED1306
      #ifdef XY2
        sendStrXY("BLK:",0,0);
        input[0]=48+block/10;input[1]=48+block%10;input[2]=0;sendStrXY((char *)input,4,0);
        //utoa(block, (char *)input, 10);sendStrXY(input,4,0);//sendChar(' ');
        if (bytesRead > 0){
          sendStrXY(" ID:", 6,0);
          if (currentID/16 < 10) input[0]=48+currentID/16;
          else input[0]=55+currentID/16;
          if (currentID%16 < 10) input[1]=48+currentID%16;
          else input[1]=55+currentID%16;
          input[2]=0;sendStrXY((char *)input,10,0);
        }          
      #else
          setXY(6,3);
          sendStr((unsigned char *)"BLK:");
          if (block < 10 ) {
            setXY(10,3);sendStr("0");
            setXY(11,3);
            utoa(block, (char *)input, 10);sendStr(input);sendChar(' ');  
          } else {
            setXY(10,3);
            utoa(block, (char *)input, 10);sendStr(input);sendChar(' ');
          }
          if (bytesRead > 0){
             setXY(0,3);
             sendStr((unsigned char *)"ID:");
             utoa(currentID,(char *)input,16);sendStr((unsigned char *)strupr((char *)input)); // Block ID en hex
          }
      #endif
    #endif      
       currpct=100; 
       lcdsegs=0;       
       currentBit=0;                               // fallo reproducción de .tap tras .tzx
       pass=0;
       checkForEXT (sfileName);
  if (!casduino) {
  currentBlockTask = READPARAM;               //First block task is to read in parameters
  Timer1.setPeriod(1000);                     //set 1ms wait at start of a file.
  }
}

void GetAndPlayBlock() {
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
   currentTask=PROCESSID; 
   SetPlayBlock(); 
}

void str4cpy (char *dest, char *src) {
  char x =0;
  while (*(src+x) && (x<4)) {
       dest[x] = src[x];
       x++;
  }
  for(x; x<4; x++) dest[x]=' ';
  dest[4]=0; 
}

void GetFileName (int pos) {
  RewindSD();
  for(int i=1;i<=pos-1;i++) {
#ifdef SDFat
    entry.openNext(entry.cwd(),O_READ);
    entry.close();
#else
    entry = cwdentry.openNextFile();
    entry.close();
#endif
  }
  //if (pos==1) {entry.cwd()->rewind();}
#ifdef SDFat
  entry.openNext(entry.cwd(),O_READ);
  entry.getName(fileName,filenameLength);
  //entry.getSFN(sfileName);
  entry.close();
  //scrollPos=0;
  //scrollText(fileName);
#else
  entry = cwdentry.openNextFile();
  char* fileName=entry.name();
  entry.close();
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
  cwdentry = SD.open(fileName);
  //cwdentry.rewindDirectory();
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
    delay(1500);
    reset_display();
  #endif
  #ifdef LCD16
    lcd.setCursor(1,0); lcd.print(F(VERSION));
    delay(2000);
    lcd.clear();
  #endif  
  #ifdef LCD20
    lcd.setCursor(0,0); lcd.print("--------------------");
    lcd.setCursor(3,1); lcd.print(F(VERSION));
    lcd.setCursor(0,2); lcd.print("--------------------");
    delay(2000);
    lcd.clear();
  #endif
}

void CheckDirEmpty() {
  if (isDir==1 && Delete==1 && start==0 && pauseOn==0) {
    CantDelDir();
    delay(1500);
    #ifdef OLED1306
      OledStatusLine();
    #endif
    #ifdef LCD20
      LCDBStatusLine();
    #endif    
    REWIND=1;
    seekFile(currentFile-1);
  }
  if (isDir==0 && Delete==1 && start==0 && pauseOn==0) {
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
  #endif
  #ifdef LCD16
    lcd.setCursor(0,0); lcd.print("Directory Empty ");
    lcd.setCursor(0,1); lcd.print("STOP to go Back ");
  #endif  
  #ifdef LCD20
    lcd.setCursor(0,0); lcd.print("Directory Empty     ");
    lcd.setCursor(0,1); lcd.print("STOP to go Back     ");
  #endif
}

void CantDelDir() {
  #ifdef OLED1306
    sendStrXY("Can't Delete DIR",0,0);
    sendStrXY("Going Back      ",0,1);
  #endif
  #ifdef LCD16
    lcd.setCursor(0,0); lcd.print("Can't Delete DIR");
    lcd.setCursor(0,1); lcd.print("Going Back      ");
  #endif  
  #ifdef LCD20
    lcd.setCursor(0,0); lcd.print("Can't Delete DIR    ");
    lcd.setCursor(0,1); lcd.print("Going Back          ");
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

void NoSDCard() {
  #ifdef OLED1306
    setXY(0,0); sendStr((unsigned char *)"----------------");
    setXY(0,1); sendStr((unsigned char *)"   No SD Card   ");
    setXY(0,2); sendStr((unsigned char *)"----------------");
    setXY(0,3); sendStr((unsigned char *)"   Insert  SD   ");
    setXY(0,4); sendStr((unsigned char *)"----------------");        
  #endif
  #ifdef LCD16
    lcd.setCursor(0,0); lcd.print("   No SD Card   ");
    lcd.setCursor(0,1); lcd.print("   Insert  SD   ");    
  #endif  
  #ifdef LCD20
    printtextF(PSTR("     No SD Card     "),0);
    printtextF(PSTR("--------------------"),1);
    printtextF(PSTR(" Please, insert SD. "),2);    
    printtextF(PSTR("--------------------"),3);
  #endif
}
