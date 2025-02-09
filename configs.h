// includes the corresponding user config.h file based on target platform

#ifndef CONFIGS_H_INCLUDED
#define CONFIGS_H_INCLUDED

#include "defines_config.h"

// IF YOU REALLY WANT TO DEFINE YOUR CONFIG VERSION IN THIS HEADER FILE,
// YOU CAN, BY UNCOMMENTING AND CHANGING THE FOLLOWING LINE
// BUT DOING IT VIA BUILD SETTINGS MIGHT BE EASIER
// #define CONFIGFILE 7
// ALTERNATIVELY YOU CAN DEFINE A DEFAULT FOR A PLATFORM WITHOUT CHANGING
// THE DEFAULTS FOR OTHER PLATFORMS
//#define _CONFIG_FILE_DEFAULT_ATMEGA328P 7

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*   OPTION TO HAVE ALL CONFIG PARAMETERS DEFINED ENTIRELY VIA -D / platformio.ini                                      */
#if defined(CONFIGFILE) && CONFIGFILE == -1

  #include "userconfig_blank.h"

#else
  #if defined(__AVR_ATmega2560__)
    #ifndef CONFIGFILE
    #define CONFIGFILE _CONFIG_FILE_DEFAULT_ATMEGA2560
    #endif
    #define CONFIG_PATH Mega2560config
  #elif defined(__arm__) && defined(__STM32F1__)
    #ifndef CONFIGFILE
    #define CONFIGFILE _CONFIG_FILE_DEFAULT_STM32
    #endif
    #define CONFIG_PATH STM32config
  #else //__AVR_ATmega328P__
    #include "userconfig.h" // legacy
    #ifndef CONFIGFILE
    #define CONFIGFILE _CONFIG_FILE_DEFAULT_ATMEGA328P
    #endif
    #define CONFIG_PATH userconfig
  #endif

  #include CONFIG_header(CONFIG_PATH, CONFIGFILE)

#endif

#endif // CONFIGS_H_INCLUDED
