#ifndef log_h
#define log_h

#include <Arduino.h>
#include "env.h"

#ifndef LOG
#define LOG(...) \
    if (DEBUG)   \
    Serial.println(__VA_ARGS__)
#endif

#ifndef STRINGIZER
#define STRINGIZER(arg) #arg
#endif

#ifndef STR_VALUE
#define STR_VALUE(arg) STRINGIZER(arg)
#endif

#endif