#ifndef webserver_h
#define webserver_h

#include "modules/mesh.h"

#include <Arduino.h>
#ifdef ESP8266
#include "Hash.h"
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include <ArduinoJson.h>
#include "AsyncJson.h"

#endif