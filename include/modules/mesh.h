#ifndef mesh_h
#define mesh_h

#include "log.h"

void createMesh(painlessMesh *mesh, String prefix, String password, int port, int channel, String hostname, String ssid, String stationPassword, bool root, painlessmesh::receivedCallback_t onReceive);
IPAddress getlocalIP(painlessMesh *mesh);

#endif