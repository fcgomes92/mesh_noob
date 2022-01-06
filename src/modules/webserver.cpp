#include "modules/webserver.h"

int createHTTPServer(AsyncWebServer *server, DynamicJsonDocument nodeConfig)
{
    LOG("setting up webserver");
    server->on("/settings/node", HTTP_GET, [&](AsyncWebServerRequest *request)
               {
                  AsyncJsonResponse *response = new AsyncJsonResponse();
                  response->addHeader("Server", nodeConfig["node"]["hostname"]);
                  JsonObject root = response->getRoot();
                  root.set(nodeConfig.as<JsonObject>());
                  response->setLength();
                  request->send(response); });
    server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
    return 0;
}