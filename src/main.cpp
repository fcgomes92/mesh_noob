// TODO: setup command handler integration on mesh callback
// TODO: setup MQTT bridge connection and command handler integration
// TODO: setup command handler integration on the webserver main endpoint
// TODO: setup web client interface and integration to the command handler
// TODO: "Democracy" implementation
// TODO: cronie alike
#include "log.h"
#include "modules/mesh.h"
#include "modules/webserver.h"
#include "modules/commands.h"
#include <PubSubClient.h>
#include <WiFiClient.h>

// #include <SPIFFS.h>

// #define MESH_PREFIX uaiot
// #define MESH_PASSWORD HafniumInsultSpecialLand9
// #define MESH_PORT 5555
// #define MESH_CHANNEL 6

// #define STATION_SSID batata_frita
// #define STATION_PASSWORD batataassadaS2

// #define MQTT_HOST berry.ccs
// #define HOSTNAME MQTT_Bridge_leaf
// #define NODE_CONFIG_PATH "/config.txt"
// #define NODE_CONFIG_CHKSUM_PATH "/configc.txt"

DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
AsyncWebServer server(80);
painlessMesh mesh;
IPAddress myIP(0, 0, 0, 0);
IPAddress mqttIP(0, 0, 0, 0);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#ifdef LEDSTRIP
#include "modules/ledStrip.h"
WS2812FX ws2812fx(LEDSTRIP_LEDS, 5, LEDSTRIP_TYPE);
// WS2812FX *ws2812fx;
#endif

void mqttCallback(char *topic, byte *payload, unsigned int length);
void receivedCallback(const uint32_t &_from, const String &msg);

void setup()
{
    delay(250);
    LOG("###");
    Serial.begin(115200);

    if (!SPIFFS.begin())
    {
        LOG("An Error has occurred while mounting SPIFFS");
        return;
    }

    // eSPIFFS fileSystem(&Serial);
    nodeConfig = loadConfig();
    bool isRoot = nodeConfig["isRoot"].as<bool>();

    LOG("Configuring mesh");
    createMesh(&mesh,
               nodeConfig["meshPrefix"].as<String>(),
               nodeConfig["meshPassword"].as<String>(),
               nodeConfig["meshPort"].as<int>(),
               nodeConfig["meshChannel"].as<int>(),
               nodeConfig["hostname"].as<String>(),
               nodeConfig["stationSSID"].as<String>(),
               nodeConfig["stationPassword"].as<String>(),
               isRoot,
               &receivedCallback);

    if (isRoot)
    {
        mqttIP.fromString(nodeConfig["mqttHost"].as<String>());
        mqttClient.setServer(mqttIP, 1883);
        mqttClient.setCallback(mqttCallback);
    }

    createHTTPServer(&server, nodeConfig);

    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/node/setup", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                           {
                                                                               if (request->method() == HTTP_POST)
                                                                               {
                                                                                   nodeConfig.set(json.as<JsonObject>());
                                                                                   updateNodeConfig(nodeConfig);

                                                                                   AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                   response->addHeader("Server", nodeConfig["hostname"]);
                                                                                   JsonObject root = response->getRoot();
                                                                                   root["data"] = nodeConfig;
#ifdef LEDSTRIP
                                                                                   setupLEDStrip(&ws2812fx, nodeConfig);
#endif
                                                                                   response->setLength();
                                                                                   request->send(response);
                                                                               }
                                                                               else
                                                                               {
                                                                                   request->send(404);
                                                                               } });
    server.addHandler(handler);

    AsyncCallbackJsonWebHandler *handlerBroadcast = new AsyncCallbackJsonWebHandler("/broadcast", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                    {
                                                                                        AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                        response->addHeader("Server", nodeConfig["hostname"]);
                                                                                        JsonObject root = response->getRoot();
                                                                                        root.set(json.as<JsonObject>());
                                                                                        
                                                                                        String msg{""};
                                                                                        serializeJson(root, msg);
                                                                                        // LOG("msg: \n" + msg);
                                                                                        mesh.sendBroadcast(msg.c_str());

                                                                                        response->setLength();
                                                                                        response->setCode(200);
                                                                                        request->send(response); });
    server.addHandler(handlerBroadcast);
    // Async webserver

    server.begin();
#ifdef LEDSTRIP
    // WS2812FX _ws2812fx(nodeConfig["ledStrip"]["leds"].as<uint8_t>(), 5, nodeConfig["ledStrip"]["type"].as<uint8_t>());
    // ws2812fx = _ws2812fx;
    ws2812fx.init();
    setupLEDStrip(&ws2812fx, nodeConfig);
    ws2812fx.start();
#endif
}

void loop()
{
    bool isRoot = nodeConfig["isRoot"].as<bool>();
    mesh.update();
    if (isRoot)
        mqttClient.loop();
    if (myIP != getlocalIP(&mesh))
    {
        myIP = getlocalIP(&mesh);
        LOG("My IP is " + myIP.toString());
        if (isRoot)
        {
            if (mqttClient.connect("painlessMeshClient"))
            {
                mqttClient.publish("painlessMesh/from/gateway", "Ready!");
                mqttClient.subscribe("painlessMesh/to/#");
            }
            else
            {

                LOG("MQTT Client failed with state: ");
                LOG(mqttClient.state());
                delay(500);
            }
        }
    }
#ifdef LEDSTRIP
    ws2812fx.service();
#endif
}

void receivedCallback(const uint32_t &_from, const String &msg)
{
    String topic = "painlessMesh/from/" + String(_from);
    LOG("msg: " + msg);
    LOG("from: " + topic);
    LOG("\n");

    bool isRoot = nodeConfig["isRoot"].as<bool>();
    if (isRoot)
    {
        mqttClient.publish(topic.c_str(), msg.c_str());
    }

    DynamicJsonDocument payload(JSON_OBJECT_SIZE(32));
    DeserializationError e = deserializeJson(payload, msg.c_str());
    if (e)
    {
        LOG("Error parsing payload");
        LOG(e.f_str());
    }
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
    char *cleanPayload = (char *)malloc(length + 1);
    memcpy(cleanPayload, payload, length);
    cleanPayload[length] = '\0';
    String msg = String(cleanPayload);
    free(cleanPayload);

    String targetStr = String(topic).substring(16);

    if (targetStr == "gateway")
    {
        if (msg == "getNodes")
        {
            auto nodes = mesh.getNodeList(true);
            String str;
            for (auto &&id : nodes)
                str += String(id) + String(";");
            mqttClient.publish("painlessMesh/from/gateway", str.c_str());
        }
    }
    else if (targetStr == "broadcast")
    {
        mesh.sendBroadcast(msg);
    }
    else
    {
        uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
        if (mesh.isConnected(target))
        {
            mesh.sendSingle(target, msg);
        }
        else
        {
            mqttClient.publish("painlessMesh/from/gateway", "Client not connected!");
        }
    }
}
