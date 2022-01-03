#ifndef log_h
#define log_h

#include "env.h"

#ifndef LOG
#define LOG(...) \
    if (DEBUG)   \
    Serial.println(__VA_ARGS__)
#endif

#endif