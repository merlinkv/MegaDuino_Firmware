#ifndef DEFINES_CONFIG_H_INCLUDED
#define DEFINES_CONFIG_H_INCLUDED

#define __CONFIG_header(x) #x
#define _CONFIG_header(p,x) __CONFIG_header(p ## x.h)
#define CONFIG_header(p,x) _CONFIG_header(p,x)

#define NO_SUFFIX

#ifndef _CONFIG_FILE_DEFAULT_ATMEGA2560
#define _CONFIG_FILE_DEFAULT_ATMEGA2560 NO_SUFFIX
#endif

#ifndef _CONFIG_FILE_DEFAULT_STM32
#define _CONFIG_FILE_DEFAULT_STM32 NO_SUFFIX
#endif

#endif // DEFINES_CONFIG_H_INCLUDED
