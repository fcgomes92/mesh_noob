// TODO: setup command handler integration on the webserver main endpoint
// TODO: setup web client interface and integration to the command handler
// TODO: "Democracy" implementation
// TODO: cronie alike
// TODO: cleanup command handler response (avoid credentials)
#include "log.h"
#include "modules/mesh.h"
#include "modules/commands.h"
#if SINGLEOUTLET || DOUBLEOUTLET
#include "modules/outlets.h"
#endif
#include <PubSubClient.h>
#include <WiFiClient.h>

DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(NODE_CONFIG_SIZE));
AsyncWebServer server(80);
painlessMesh mesh;
IPAddress myIP(0, 0, 0, 0);
IPAddress mqttIP(0, 0, 0, 0);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#ifdef LEDSTRIP
#include "modules/ledStrip.h"
WS2812FX ws2812fx(LEDSTRIP_LEDS, LEDSTRIP_PIN, LEDSTRIP_TYPE);

void handleNodeUpdate(JsonObject payload, JsonObject nodeConfig, WS2812FX *ws2812fx);
#else
void handleNodeUpdate(JsonObject payload, JsonObject nodeConfig);
#endif

void mqttCallback(char *topic, byte *payload, unsigned int length);
void receivedCallback(const uint32_t &_from, const String &msg);
void publishConfig(JsonObject nodeConfig, PubSubClient *mqttClient, painlessMesh *mesh);

#ifdef SENSOR
#ifdef SDHT
#include "DHT.h"
DHT dht(DHTPIN, DHTTYPE);
#endif

Scheduler nodeScheduler;
bool setupSensor(painlessMesh *mesh, JsonObject nodeConfig, PubSubClient *mqttClient);
bool setupSensor(painlessMesh *mesh, JsonObject nodeConfig, PubSubClient *mqttClient)
{
#ifdef SDHT
    dht.begin();
#endif
    bool isRoot = nodeConfig["node"]["isRoot"].as<bool>();
    String module = nodeConfig["module"].as<String>();
    JsonObject sensorConfig = nodeConfig[module].as<JsonObject>();
    unsigned long updateInterval = sensorConfig["interval"].as<unsigned long>();
    mesh->addTask(updateInterval, TASK_FOREVER, [mesh, nodeConfig, mqttClient, isRoot]()
                  {
                      if (myIP.toString().equals(getlocalIP(mesh).toString()))
                        LOG("Running task");
                      {
                        DynamicJsonDocument jsonBuffer(1024);
                                JsonObject msg = jsonBuffer.to<JsonObject>();
                                msg["id"] = mesh->getNodeId();
#ifdef SDHT
                                msg["h"] = dht.readHumidity();
                                msg["t"] = dht.readTemperature();
#endif
#if SSOILM
                                msg["sh"] = analogRead(SOILM_PIN);
#endif

                                String str{""};
                                serializeJson(msg, str);
                                if (isRoot)
                                {
                                    String hostname = nodeConfig["node"]["hostname"].as<String>();
                                    String nodeTopic = nodeConfig["node"]["topic"].as<String>();
                                        // String publishPath = buildPublishTopic(hostname, nodeTopic);
                                        // String subPath = buildSubscribeTopic("#", nodeTopic);
                                        // publishConfig(nodeConfig, mqttClient, mesh);
                                    mqttClient->publish(buildPublishTopic(hostname + "/sensor", nodeTopic).c_str(), str.c_str());
                                        
                                } 
                                else 
                                {
                                    mesh->sendBroadcast(str); 
                                } 
                    } })
        ->enable();
    return 0;
}
#endif

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

    nodeConfig = loadConfig();
    bool isRoot = nodeConfig["node"]["isRoot"].as<bool>();

    LOG("Configuring mesh");
    createMesh(&mesh,
               nodeConfig["node"]["meshPrefix"].as<String>(),
               nodeConfig["node"]["meshPassword"].as<String>(),
               nodeConfig["node"]["meshPort"].as<int>(),
               nodeConfig["node"]["meshChannel"].as<int>(),
               nodeConfig["node"]["hostname"].as<String>(),
               nodeConfig["node"]["stationSSID"].as<String>(),
               nodeConfig["node"]["stationPassword"].as<String>(),
               isRoot,
               &receivedCallback);

    if (isRoot)
    {
        mqttIP.fromString(nodeConfig["node"]["mqttHost"].as<String>());
        mqttClient.setServer(mqttIP, 1883);
        mqttClient.setCallback(mqttCallback);
    }

    // createHTTPServer(&server, nodeConfig);
    server.on("/settings/node", HTTP_GET, [&](AsyncWebServerRequest *request)
               {
                  AsyncJsonResponse *response = new AsyncJsonResponse();
                  nodeConfig.garbageCollect();
                  response->addHeader("Server", nodeConfig["node"]["hostname"]);
                  JsonObject root = response->getRoot();
                  root.set(nodeConfig.as<JsonObject>());
                  response->setLength();
                  request->send(response); });
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/node/setup", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                           {
#ifdef LEDSTRIP
                                                                                   handleNodeUpdate(json.as<JsonObject>(), nodeConfig.as<JsonObject>(), &ws2812fx);
#else
                                                                                   handleNodeUpdate(json.as<JsonObject>(), nodeConfig.as<JsonObject>());
#endif
                                                                                   nodeConfig.garbageCollect();
                                                                                   AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                   response->addHeader("Server", nodeConfig["node"]["hostname"]);
                                                                                   JsonObject root = response->getRoot();
                                                                                   root["data"] = nodeConfig;
                                                                                   publishConfig(nodeConfig.as<JsonObject>(), &mqttClient, &mesh);
                                                                                   response->setLength();
                                                                                   request->send(response); });
    server.addHandler(handler);

    AsyncCallbackJsonWebHandler *handlerBroadcast = new AsyncCallbackJsonWebHandler("/broadcast", [&](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                    {
                                                                                        AsyncJsonResponse *response = new AsyncJsonResponse();
                                                                                        response->addHeader("Server", nodeConfig["node"]["hostname"]);
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
    LOG("Setting up ledStrip");
    LOG("LED INFO: " + String(LEDSTRIP_LEDS) + " leds | pin " + String(LEDSTRIP_PIN));
    ws2812fx.init();
    setupLEDStrip(&ws2812fx, nodeConfig);
    ws2812fx.start();
    pinMode(LEDSTRIP_PIN, OUTPUT);
#endif

#ifdef SINGLEOUTLET
    LOG("Setting up single outlet");
    pinMode(SINGLEOUTLET_PIN, OUTPUT);
    setupOutlet(SINGLEOUTLET_PIN, nodeConfig["outlets"]["state"][0]);
#endif
#ifdef DOUBLEOUTLET
    LOG("Setting up double outlet");
    LOG("PIN0: " + String(DOUBLEOUTLET_PIN0));
    LOG("PIN1: " + String(DOUBLEOUTLET_PIN1));
    pinMode(DOUBLEOUTLET_PIN0, OUTPUT);
    setupOutlet(DOUBLEOUTLET_PIN0, nodeConfig["outlets"]["state"][0]);

    pinMode(DOUBLEOUTLET_PIN1, OUTPUT);
    setupOutlet(DOUBLEOUTLET_PIN1, nodeConfig["outlets"]["state"][1]);
#endif

#ifdef SENSOR
    LOG("Setting up sensor");
    setupSensor(&mesh, nodeConfig.as<JsonObject>(), &mqttClient);
    // nodeScheduler.addTask(*nodeSensorTask);
    // nodeSensorTask->enable();
    // mesh.addTask(nodeSensorTask);

#endif
    LOG("Finished setup");
}

void loop()
{
    bool isRoot = nodeConfig["node"]["isRoot"].as<bool>();
    mesh.update();
    if (myIP != getlocalIP(&mesh))
    {
        myIP = getlocalIP(&mesh);
        LOG("My IP is " + myIP.toString());
        if (isRoot)
        {
            if (mqttClient.connect(nodeConfig["node"]["hostname"].as<String>().c_str()))
            {
                String hostname = nodeConfig["node"]["hostname"].as<String>();
                String nodeTopic = nodeConfig["node"]["topic"].as<String>();
                // String publishPath = buildPublishTopic(hostname, nodeTopic);
                String subPath = buildSubscribeTopic("#", nodeTopic);
                // String configStr{""};
                // serializeJson(nodeConfig, configStr);

                // mqttClient.publish(publishPath.c_str(), configStr.c_str());
                publishConfig(nodeConfig.as<JsonObject>(), &mqttClient, &mesh);
                mqttClient.subscribe(subPath.c_str());
            }
            else
            {

                LOG("MQTT Client failed with state: ");
                LOG(mqttClient.state());
                delay(500);
            }
        }
    }
    if (isRoot)
        mqttClient.loop();
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

    bool isRoot = nodeConfig["node"]["isRoot"].as<bool>();
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
        return;
    }

#ifdef LEDSTRIP
    handleNodeUpdate(payload.as<JsonObject>(), nodeConfig.as<JsonObject>(), &ws2812fx);
#else
    handleNodeUpdate(payload.as<JsonObject>(), nodeConfig.as<JsonObject>());
#endif
    String nodeConfigString{""};
    serializeJson(nodeConfig, nodeConfigString);
    mqttClient.publish(topic.c_str(), nodeConfigString.c_str());
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
    char *cleanPayload = (char *)malloc(length + 1);
    memcpy(cleanPayload, payload, length);
    cleanPayload[length] = '\0';
    String msg = String(cleanPayload);
    free(cleanPayload);
    String hostname = nodeConfig["node"]["hostname"].as<String>();
    String nodeTopic = nodeConfig["node"]["topic"].as<String>();
    String publishPath = buildPublishTopic(hostname, nodeTopic);
    String targetStr = String(topic).substring(nodeTopic.length() + 4);

    if (targetStr == hostname)
    {
        if (msg == "getNodes")
        {
            auto nodes = mesh.getNodeList(true);
            String str;
            for (auto &&id : nodes)
                str += String(id) + String(";");
            mqttClient.publish(publishPath.c_str(), str.c_str());
        }
        else
        {
            DynamicJsonDocument data(JSON_OBJECT_SIZE(32));
            DeserializationError e = deserializeJson(data, msg.c_str());
            if (e)
            {
                LOG("Error parsing payload");
                LOG(e.f_str());
            }
            else
            {
#ifdef LEDSTRIP
                handleNodeUpdate(data.as<JsonObject>(), nodeConfig.as<JsonObject>(), &ws2812fx);
#else
                handleNodeUpdate(data.as<JsonObject>(), nodeConfig.as<JsonObject>());
#endif
                data.clear();
                nodeConfig.garbageCollect();
                // String result{""};
                // serializeJson(nodeConfig, result);
                // mqttClient.publish(publishPath.c_str(), result.c_str());
                publishConfig(nodeConfig.as<JsonObject>(), &mqttClient, &mesh);
            }
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
            mqttClient.publish(publishPath.c_str(), "Client not connected!");
        }
    }
}

#ifdef LEDSTRIP
void handleNodeUpdate(JsonObject payload, JsonObject nodeConfig, WS2812FX *ws2812fx)
#else
void handleNodeUpdate(JsonObject payload, JsonObject nodeConfig)
#endif
{
    uint8_t type = payload["t"].as<uint8_t>();
    JsonObject data = payload["p"].as<JsonObject>();
    LOG("NodeUpdate: " + String(type));
    switch (type)
    {
    case 1:
        nodeConfig.set(data);
        break;
    case 2:
    {
        String module = nodeConfig["module"].as<String>();
        nodeConfig[module].set(data);
        break;
    }
    }

    // saves the NodeConfig to a file
    updateNodeConfig(nodeConfig);

#ifdef LEDSTRIP
    setupLEDStrip(ws2812fx, nodeConfig);
#endif
#ifdef SINGLEOUTLET
    setupOutlet(SINGLEOUTLET_PIN, nodeConfig["outlets"]["state"][0]);
#endif
#ifdef DOUBLEOUTLET
    setupOutlet(DOUBLEOUTLET_PIN0, nodeConfig["outlets"]["state"][0]);
    setupOutlet(DOUBLEOUTLET_PIN1, nodeConfig["outlets"]["state"][1]);
#endif
    data.clear();
}

void publishConfig(JsonObject nodeConfig, PubSubClient *mqttClient, painlessMesh *mesh)
{
    bool isRoot = nodeConfig["node"]["isRoot"].as<bool>();
    String hostname = nodeConfig["node"]["hostname"].as<String>();
    String nodeTopic = nodeConfig["node"]["topic"].as<String>();
    String publishPath = buildPublishTopic(hostname, nodeTopic);
    String module = nodeConfig["module"].as<String>();
    String moduleConfig{""};
    String node{""};
    serializeJson(nodeConfig[module], moduleConfig);
    serializeJson(nodeConfig["node"], node);
    if (isRoot)
    {
        mqttClient->publish(buildPublishTopic(hostname + "/" + module, nodeTopic).c_str(), moduleConfig.c_str());
        mqttClient->publish(buildPublishTopic(hostname + "/node", nodeTopic).c_str(), node.c_str());
    }
    else
    {
        mesh->sendBroadcast(moduleConfig);
    }
}