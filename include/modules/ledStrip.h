#ifndef ledStrip_h
#define ledStrip_h

#include "log.h"
#include <WS2812FX.h>

void setupLEDStrip(WS2812FX *ledStrip, DynamicJsonDocument nodeConfig);
#endif