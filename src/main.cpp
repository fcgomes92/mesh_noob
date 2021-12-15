#include "modules/mesh.h"
#include "modules/webserver.h"
#include "utils.h"
#include "log.h"

#include <CRC32.h>

#include <FS.h>
// #include <SPIFFS.h>

#define MESH_PREFIX uaiot
#define MESH_PASSWORD HafniumInsultSpecialLand9
#define MESH_PORT 5555
#define MESH_CHANNEL 6

#define STATION_SSID batata_frita
#define STATION_PASSWORD batataassadaS2

#define MQTT_HOST berry.ccs
#define HOSTNAME MQTT_Bridge_leaf
#define NODE_CONFIG_PATH "/config.txt"
#define NODE_CONFIG_CHKSUM_PATH "/configc.txt"

DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
AsyncWebServer server(80);
painlessMesh mesh;
IPAddress myIP(0, 0, 0, 0);

DynamicJsonDocument saveConfig(DynamicJsonDocument nodeConfig);
DynamicJsonDocument loadConfig();
void receivedCallback(const unsigned long int &_from, const String &msg);
// NodeConfig nodeConfig;

void setup()
{
    delay(250);
    LOG("###");
    Serial.begin(115200);

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // eSPIFFS fileSystem(&Serial);
    nodeConfig = loadConfig();

    LOG("Configuring mesh");
    createMesh(&mesh,
               nodeConfig["meshPrefix"].as<String>(),
               nodeConfig["meshPassword"].as<String>(),
               nodeConfig["meshPort"].as<int>(),
               nodeConfig["meshChannel"].as<int>(),
               nodeConfig["hostname"].as<String>(),
               nodeConfig["stationSSID"].as<String>(),
               nodeConfig["stationPassword"].as<String>(),
               nodeConfig["isRoot"].as<bool>(),
               &receivedCallback);

    // Async webserver
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/node/setup", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                           {
                                                                               if (request->method() == HTTP_POST)
                                                                               {
                                                                                   JsonObject jsonObj = json.as<JsonObject>();
                                                                                   AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                   response->addHeader("Server", nodeConfig["hostname"]);
                                                                                   JsonObject root = response->getRoot();
                                                                                   nodeConfig = jsonObj;
                                                                                   root["data"] = saveConfig(nodeConfig);
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
                                                                                        root["data"] = json.as<JsonObject>();
                                                                                        String msg;
                                                                                        serializeJson(root["data"], msg);
                                                                                        mesh.sendBroadcast(msg);
                                                                                        response->setLength();
                                                                                        response->setCode(200);
                                                                                        request->send(response); });
    server.addHandler(handlerBroadcast);
    server.on("/settings/node", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncJsonResponse *response = new AsyncJsonResponse();
                  response->addHeader("Server", nodeConfig["hostname"]);
                  JsonObject root = response->getRoot();
                  root["data"] = nodeConfig;
                  response->setLength();
                  request->send(response); });
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

DynamicJsonDocument saveConfig(DynamicJsonDocument nodeConfig)
{
    File newConfigFile = SPIFFS.open(NODE_CONFIG_PATH, "w");
    File newConfigCheckSumFile = SPIFFS.open(NODE_CONFIG_CHKSUM_PATH, "w");
    String newConfigOutput{""};
    unsigned long int chksum{0};
    CRC32 cs;

    // serialize current config
    serializeJson(nodeConfig, newConfigOutput);

    // calculate checksum of new config
    cs.update(newConfigOutput.c_str());
    chksum = cs.finalize();

    // save new config
    LOG("Writing new config");
    LOG(newConfigOutput);
    newConfigFile.print(newConfigOutput.c_str());
    newConfigFile.close();

    LOG("Writing new config chksum");
    LOG(chksum);
    newConfigCheckSumFile.print(String(chksum).c_str());
    newConfigCheckSumFile.close();
    return nodeConfig;
}

DynamicJsonDocument loadConfig()
{
    DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
    File configFile = SPIFFS.open(NODE_CONFIG_PATH, "r");
    File configCheckSumFile = SPIFFS.open(NODE_CONFIG_CHKSUM_PATH, "r");
    unsigned long int chksum{0};
    unsigned long int nodeConfigChksum = strtoul(configCheckSumFile.readString().c_str(), NULL, 10);
    String nodeConfigOutput = configFile.readString();

    configFile.close();
    configCheckSumFile.close();

    LOG("Loading config");
    LOG(nodeConfigOutput);

    CRC32 cs;
    cs.update(nodeConfigOutput.c_str());
    chksum = cs.finalize();

    LOG("hashes");
    LOG("nodeConfigChksum: " + (String)nodeConfigChksum);
    LOG("chksum: " + (String)chksum);
    if (nodeConfigChksum != chksum)
    {
        configFile = SPIFFS.open(NODE_CONFIG_PATH, "w");
        configCheckSumFile = SPIFFS.open(NODE_CONFIG_CHKSUM_PATH, "w");

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
        nodeConfig["isRoot"] = false;

        LOG("Writing default config");
        serializeJson(nodeConfig, defaultConfigOutput);
        configFile.print(nodeConfigOutput);

        CRC32 defaultCS;
        defaultCS.update(nodeConfigOutput.c_str());
        unsigned long int defaultCSchksum = defaultCS.finalize();
        configCheckSumFile.print(String(defaultCSchksum).c_str());

        configFile.close();
        configCheckSumFile.close();
    }
    else
    {
        DeserializationError err = deserializeJson(nodeConfig, nodeConfigOutput);
        if (err)
            LOG(err.f_str());
    }
    return nodeConfig;
}

void receivedCallback(const unsigned long int &_from, const String &msg)
{
    String from = (String)_from;
    LOG(String("bridge: Received from ") + from.c_str() + String(" msg=") + msg);
    String config{""};
    serializeJson(nodeConfig, config);
    LOG("config: \n" + config);
}
