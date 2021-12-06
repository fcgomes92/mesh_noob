#include "modules/ledStrip.h"

void setupLEDStrip(WS2812FX *ledStrip, DynamicJsonDocument nodeConfig)
{
    ledStrip->setBrightness(nodeConfig["ledStrip"]["bright"].as<uint8_t>());
    ledStrip->setSpeed(nodeConfig["ledStrip"]["speed"].as<uint16_t>());
    ledStrip->setMode(nodeConfig["ledStrip"]["mode"].as<uint8_t>());
    ledStrip->setColor(nodeConfig["ledStrip"]["color"].as<uint32_t>());
}