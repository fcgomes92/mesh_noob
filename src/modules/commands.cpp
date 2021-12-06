#include "modules/commands.h"

String getFileContent(String fileName)
{
    File f = SPIFFS.open(fileName, "r");
    String content = f.readString();
    f.close();
    return content;
}

String writeFileContent(String fileName, String data)
{
    File f = SPIFFS.open(fileName, "w");
    f.print(data.c_str());
    f.close();
    return data;
}

uint32_t getCheckSum(String data)
{
    CRC32 cs;
    cs.update(data.c_str());
    return cs.finalize();
}

bool updateNodeConfig(JsonObject newNodeConfig)
{
    String newConfigOutput{""};
    serializeJson(newNodeConfig, newConfigOutput);
    uint32_t checksum = getCheckSum(newConfigOutput);
    String contentResult = writeFileContent(STR_VALUE(NODE_CONFIG_PATH), newConfigOutput);
    String checksumResult = writeFileContent(STR_VALUE(NODE_CONFIG_CHKSUM_PATH), String(checksum));
    return contentResult && checksumResult;
}

DynamicJsonDocument getDefaultNodeConfig()
{
    DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(NODE_CONFIG_SIZE));
    JsonObject node = nodeConfig.createNestedObject("node");
    node["meshPrefix"].set(STR_VALUE(MESH_PREFIX));
    node["meshPassword"] = STR_VALUE(MESH_PASSWORD);
    node["meshPort"] = MESH_PORT;
    node["meshChannel"] = MESH_CHANNEL;
    node["stationSSID"] = STR_VALUE(STATION_SSID);
    node["stationPassword"] = STR_VALUE(STATION_PASSWORD);
    node["mqttHost"] = STR_VALUE(MQTT_HOST);
    node["hostname"] = STR_VALUE(HOSTNAME);
    node["topic"] = STR_VALUE(TOPIC);
    node["isRoot"] = NODE_IS_ROOT;
#ifdef LEDSTRIP
    node["module"] = "lefStrip";
    JsonObject ledStrip = nodeConfig.createNestedObject("ledStrip");
    ledStrip["leds"] = LEDSTRIP_LEDS;
    ledStrip["type"] = LEDSTRIP_TYPE;
    ledStrip["bright"] = LEDSTRIP_BRIGHT;
    ledStrip["speed"] = LEDSTRIP_SPEED;
    ledStrip["mode"] = LEDSTRIP_MODE;
    ledStrip["color"] = 16777216;
#endif
#ifdef SINGLEOUTLET
    node["module"] = "outlets";
    JsonObject outlets = nodeConfig.createNestedObject("outlets");
    JsonArray state = outlets.createNestedArray("state");
    state.add(false);
#endif
#ifdef DOUBLEOUTLET
    node["module"] = "outlets";
    JsonObject outlets = nodeConfig.createNestedObject("outlets");
    JsonArray state = outlets.createNestedArray("state");
    state.add(false);
    state.add(false);
#endif
    return nodeConfig;
}

DynamicJsonDocument loadConfig()
{
    DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(NODE_CONFIG_SIZE));
    String nodeConfigOutput = getFileContent(STR_VALUE(NODE_CONFIG_PATH));
    uint32_t nodeConfigChksum = strtoul(getFileContent(STR_VALUE(NODE_CONFIG_CHKSUM_PATH)).c_str(), NULL, 10);
    uint32_t chksum = getCheckSum(nodeConfigOutput);

    LOG("Loading config");
    LOG(nodeConfigOutput);

    LOG("hashes");
    LOG("nodeConfigChksum: " + (String)nodeConfigChksum);
    LOG("chksum: " + (String)chksum);
    // if (nodeConfigChksum == chksum)
    if (true)
    {
        DeserializationError err = deserializeJson(nodeConfig, nodeConfigOutput);
        if (err)
        {
            LOG("Error loading node config");
            LOG(err.f_str());
        }
        else
        {
            return nodeConfig;
        }
    }

    LOG("Creating default config");
    nodeConfig = getDefaultNodeConfig();
    LOG("Writing default config");
    updateNodeConfig(nodeConfig.as<JsonObject>());
    return nodeConfig;
}

String buildPublishTopic(String from, String topic)
{
    return topic + "/from/" + from;
}

String buildSubscribeTopic(String to, String topic)
{
    return topic + "/to/" + to;
}