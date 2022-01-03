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

bool updateNodeConfig(DynamicJsonDocument newNodeConfig)
{
    String newConfigOutput{""};
    serializeJson(newNodeConfig, newConfigOutput);
    uint32_t checksum = getCheckSum(newConfigOutput);
    bool contentResult = writeFileContent(STR_VALUE(NODE_CONFIG_PATH), newConfigOutput);
    bool checksumResult = writeFileContent(STR_VALUE(NODE_CONFIG_CHKSUM_PATH), String(checksum));
    return contentResult && checksumResult;
}

DynamicJsonDocument getDefaultNodeConfig()
{
    DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
    nodeConfig["meshPrefix"].set(STR_VALUE(MESH_PREFIX));
    nodeConfig["meshPassword"] = STR_VALUE(MESH_PASSWORD);
    nodeConfig["meshPort"] = MESH_PORT;
    nodeConfig["meshChannel"] = MESH_CHANNEL;
    nodeConfig["stationSSID"] = STR_VALUE(STATION_SSID);
    nodeConfig["stationPassword"] = STR_VALUE(STATION_PASSWORD);
    nodeConfig["mqttHost"] = STR_VALUE(MQTT_HOST);
    nodeConfig["hostname"] = STR_VALUE(HOSTNAME);
    nodeConfig["isRoot"] = NODE_IS_ROOT;
#ifdef LEDSTRIP
    JsonObject ledStrip = nodeConfig.createNestedObject("ledStrip");
    ledStrip["leds"] = LEDSTRIP_LEDS;
    ledStrip["type"] = LEDSTRIP_TYPE;
    ledStrip["bright"] = LEDSTRIP_BRIGHT;
    ledStrip["speed"] = LEDSTRIP_SPEED;
    ledStrip["mode"] = LEDSTRIP_MODE;
#endif

    return nodeConfig;
}

DynamicJsonDocument loadConfig()
{
    DynamicJsonDocument nodeConfig(JSON_OBJECT_SIZE(32));
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
    updateNodeConfig(nodeConfig);
    return nodeConfig;
}
