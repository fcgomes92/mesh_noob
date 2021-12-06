#ifndef commands_h
#define commands_h

#include "log.h"
#ifdef LEDSTRIP
#include <WS2812FX.h>
#endif

String getFileContent(String fileName);
String writeFileContent(String fileName, String data);
uint32_t getCheckSum(String data);
bool updateNodeConfig(JsonObject newNodeConfig);
DynamicJsonDocument loadConfig();
DynamicJsonDocument getDefaultNodeConfig();
String buildSubscribeTopic(String to, String topic);
String buildPublishTopic(String from, String topic);

#endif