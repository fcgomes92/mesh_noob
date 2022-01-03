#ifndef webserver_h
#define webserver_h

#include "log.h"
#include "modules/commands.h"

int createHTTPServer(AsyncWebServer *server, DynamicJsonDocument nodeConfig);

#endif