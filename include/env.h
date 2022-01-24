#ifndef env_h
#define env_h

#include <painlessMesh.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#ifndef ESP8266
#include <SPIFFS.h>
#else
// #define SPIFFS FS
#endif
#include <CRC32.h>
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

#define NODE_CONFIG_SIZE 64

#ifndef STRINGIZER
#define STRINGIZER(arg) #arg
#endif

#ifndef STR_VALUE
#define STR_VALUE(arg) STRINGIZER(arg)
#endif

#ifndef DEBUG
#define DEBUG true
#endif

#ifndef USE_LittleFS
#define USE_LittleFS false
#endif

#ifndef MESH_PREFIX
#define MESH_PREFIX uaiot
#endif

#ifndef MESH_PASSWORD
#define MESH_PASSWORD HafniumInsultSpecialLand9
#endif

#ifndef MESH_PORT
#define MESH_PORT 5555
#endif

#ifndef MESH_CHANNEL
#define MESH_CHANNEL 6
#endif

#ifndef STATION_SSID
#define STATION_SSID batata_frita
#endif

#ifndef STATION_PASSWORD
#define STATION_PASSWORD batataassadaS2
#endif

#ifndef MQTT_HOST
#define MQTT_HOST 192.168.0.3
#endif

#ifndef HOSTNAME
#define HOSTNAME MQTT_Bridge_leaf3
#endif

#ifndef TOPIC
#define TOPIC ccs/outlets
#endif

#ifndef NODE_CONFIG_PATH
#define NODE_CONFIG_PATH /config.txt
#endif

#ifndef NODE_CONFIG_CHKSUM_PATH
#define NODE_CONFIG_CHKSUM_PATH /configc.txt
#endif

#ifndef NODE_IS_ROOT
#define NODE_IS_ROOT true
#endif

#ifdef LEDSTRIP
#ifndef LEDSTRIP_PIN
#define LEDSTRIP_PIN 5
#endif
#ifndef LEDSTRIP_LEDS
#define LEDSTRIP_LEDS 16
#endif
#ifndef LEDSTRIP_TYPE
#define LEDSTRIP_TYPE (NEO_GRB + NEO_KHZ800)
#endif
#ifndef LEDSTRIP_BRIGHT
#define LEDSTRIP_BRIGHT 80
#endif
#ifndef LEDSTRIP_SPEED
#define LEDSTRIP_SPEED 200
#endif
#ifndef LEDSTRIP_MODE
#define LEDSTRIP_MODE FX_MODE_RAINBOW_CYCLE
#endif
#endif

#ifdef SINGLEOUTLET
#ifndef SINGLEOUTLET_PIN
#define SINGLEOUTLET_PIN 0
#endif
#endif

#ifdef DOUBLEOUTLET
#ifndef DOUBLEOUTLET_PIN0
#define DOUBLEOUTLET_PIN0 4
#endif
#ifndef DOUBLEOUTLET_PIN1
#define DOUBLEOUTLET_PIN1 5
#endif
#endif

#endif