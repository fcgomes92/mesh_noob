#include <painlessMesh.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "FS.h"
// #include "SPIFFS.h"
#include <CRC32.h>
#include <Effortless_SPIFFS.h>
#include "AsyncJson.h"

#ifdef ESP8266
#include "Hash.h"
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "env.h"

#define MESH_PREFIX uaiot
#define MESH_PASSWORD HafniumInsultSpecialLand9
#define MESH_PORT 5555
#define MESH_CHANNEL 6

#define STATION_SSID batata_frita
#define STATION_PASSWORD batataassadaS2

#define MQTT_HOST berry.ccs
#define HOSTNAME MQTT_Bridge
#define NODE_CONFIG_PATH "/nodeConfig.json"
#define NODE_CONFIG_CHKSUM_PATH "/nodeConfig.chksum.json"
DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
AsyncWebServer server(80);
painlessMesh mesh;
IPAddress myIP(0, 0, 0, 0);

void logConfig(DynamicJsonDocument *config);
DynamicJsonDocument saveConfig(eSPIFFS *fileSystem, DynamicJsonDocument nodeConfig);
DynamicJsonDocument loadConfig(eSPIFFS *fileSystem);
void createMesh(painlessMesh *mesh, String prefix, String password, int port, int channel, String hostname, String ssid, String stationPassword, bool root, painlessmesh::receivedCallback_t onReceive);
void receivedCallback(const uint32_t &_from, const String &msg);
IPAddress getlocalIP(painlessMesh *mesh);
// NodeConfig nodeConfig;

void setup()
{
    delay(250);
    LOG("###");
    Serial.begin(115200);

    eSPIFFS fileSystem(&Serial);
    nodeConfig = loadConfig(&fileSystem);

    LOG("Configuring mesh");
    createMesh(&mesh,
               nodeConfig["meshPrefix"].as<String>(),
               nodeConfig["meshPassword"].as<String>(),
               nodeConfig["meshPort"].as<int>(),
               nodeConfig["meshChannel"].as<int>(),
               nodeConfig["hostname"].as<String>(),
               nodeConfig["stationSSID"].as<String>(),
               nodeConfig["stationPassword"].as<String>(),
               true,
               &receivedCallback);

    //Async webserver
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/node/setup", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                           {
                                                                               if (request->method() == HTTP_POST)
                                                                               {
                                                                                   JsonObject jsonObj = json.as<JsonObject>();
                                                                                   AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                   response->addHeader("Server", nodeConfig["hostname"]);
                                                                                   JsonObject root = response->getRoot();
                                                                                   nodeConfig = jsonObj;
                                                                                   root["data"] = saveConfig(&fileSystem, nodeConfig);
                                                                                   response->setLength();
                                                                                   request->send(response);
                                                                               }
                                                                               else
                                                                               {
                                                                                   request->send(404);
                                                                               }
                                                                           });
    server.addHandler(handler);

    AsyncCallbackJsonWebHandler *handlerBroadcast = new AsyncCallbackJsonWebHandler("/broadcast", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                    {
                                                                                        AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                        response->addHeader("Server", nodeConfig["hostname"]);
                                                                                        JsonObject root = response->getRoot();
                                                                                        root["data"] = json.as<JsonObject>();
                                                                                        String msg;
                                                                                        serializeJson(root["data"], msg);
                                                                                        mesh.sendBroadcast(msg);
                                                                                        response->setLength();
                                                                                        response->setCode(200);
                                                                                        request->send(response);
                                                                                    });
    server.on("/settings/heap", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(ESP.getFreeHeap())); });
    server.addHandler(handlerBroadcast);
    server.on("/settings/node", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncJsonResponse *response = new AsyncJsonResponse();
                  response->addHeader("Server", nodeConfig["hostname"]);
                  JsonObject root = response->getRoot();
                  root["data"] = nodeConfig;
                  response->setLength();
                  request->send(response);
              });
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.htm", "text/html"); });
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
    server.begin();
}

void loop()
{
    mesh.update();
    if (myIP != getlocalIP(&mesh))
    {
        myIP = getlocalIP(&mesh);
        LOG("My IP is " + myIP.toString());
    }
}

DynamicJsonDocument saveConfig(eSPIFFS *fileSystem, DynamicJsonDocument nodeConfig)
{
    String newConfigOutput;
    uint32_t chksum = 0;
    CRC32 cs;

    // serialize current config
    serializeJson(nodeConfig, newConfigOutput);

    // calculate checksum of new config
    cs.update(newConfigOutput);
    chksum = cs.finalize();

    // save new config
    LOG("Writing new config");
    fileSystem->saveToFile(NODE_CONFIG_PATH, newConfigOutput);

    LOG("Writing new config chksum");
    fileSystem->saveToFile(NODE_CONFIG_CHKSUM_PATH, chksum);
    return nodeConfig;
}

DynamicJsonDocument loadConfig(eSPIFFS *fileSystem)
{
    DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
    uint32_t chksum = 0;
    uint32_t nodeConfigChksum = 0;
    String nodeConfigOutput;

    fileSystem->openFromFile(NODE_CONFIG_PATH, nodeConfigOutput);
    fileSystem->openFromFile(NODE_CONFIG_CHKSUM_PATH, nodeConfigChksum);

    CRC32 cs;
    cs.update(nodeConfigOutput);
    chksum = cs.finalize();

    if (nodeConfigChksum == chksum)
    {
        LOG("Loading config");
        LOG(nodeConfigOutput);
        deserializeJson(nodeConfig, nodeConfigOutput);
    }
    else
    {
        LOG("Creating default config");
        String defaultConfigOutput;
        nodeConfig["meshPrefix"].set(STR_VALUE(MESH_PREFIX));
        nodeConfig["meshPassword"] = STR_VALUE(MESH_PASSWORD);
        nodeConfig["meshPort"] = MESH_PORT;
        nodeConfig["meshChannel"] = MESH_CHANNEL;
        nodeConfig["stationSSID"] = STR_VALUE(STATION_SSID);
        nodeConfig["stationPassword"] = STR_VALUE(STATION_PASSWORD);
        nodeConfig["mqttHost"] = STR_VALUE(MQTT_HOST);
        nodeConfig["hostname"] = STR_VALUE(HOSTNAME);
        nodeConfig["isRoot"] = true;
        LOG("Writing default config");
        serializeJson(nodeConfig, defaultConfigOutput);
        fileSystem->saveToFile(NODE_CONFIG_PATH, defaultConfigOutput);
        LOG("Writing default config chksum");
        fileSystem->saveToFile(NODE_CONFIG_CHKSUM_PATH, chksum);
    }
    LOG("#######");
    return nodeConfig;
}

void createMesh(painlessMesh *mesh, String prefix, String password, int port, int channel, String hostname, String ssid, String stationPassword, bool root, painlessmesh::receivedCallback_t onReceive)
{
    mesh->setDebugMsgTypes(ERROR | STARTUP | CONNECTION); // set before init() so that you can see startup messages

    // Channel set to 6. Make sure to use the same channel for your mesh and for you other
    // network (STATION_SSID)
    LOG("===");
    LOG(prefix);
    LOG(password);
    LOG(ssid);
    LOG("===");
    mesh->init(prefix,
               password,
               port,
               WIFI_AP_STA,
               channel || 6);
    mesh->onReceive(onReceive);

    mesh->stationManual(ssid, stationPassword);
    mesh->setHostname((hostname).c_str());

    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
    mesh->setRoot(root);
    // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
    mesh->setContainsRoot(true);
}

void receivedCallback(const uint32_t &_from, const String &msg)
{
    String from = (String)_from;
    LOG(String("bridge: Received from ") + from.c_str() + String(" msg=") + msg);
}

IPAddress getlocalIP(painlessMesh *mesh)
{
    return IPAddress(mesh->getStationIP());
}