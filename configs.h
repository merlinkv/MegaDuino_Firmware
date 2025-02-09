// includes the corresponding user config.h file based on target platform

#ifndef CONFIGS_H_INCLUDED
  #define CONFIGS_H_INCLUDED
  #include "defines_config.h"
  #if defined(__AVR_ATmega2560__)
    #ifndef CONFIGFILE
    #define CONFIGFILE _CONFIG_FILE_DEFAULT_ATMEGA2560
    #endif
    #define CONFIG_PATH Mega2560config
  #endif
  #if defined(__arm__) && defined(__STM32F1__)
    #ifndef CONFIGFILE
    #define CONFIGFILE _CONFIG_FILE_DEFAULT_STM32
    #endif
    #define CONFIG_PATH STM32config
  #endif
  #include CONFIG_header(CONFIG_PATH, CONFIGFILE)
#endif // CONFIGS_H_INCLUDED
