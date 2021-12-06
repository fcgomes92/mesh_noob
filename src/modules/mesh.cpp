#include "modules/mesh.h"

void createMesh(painlessMesh *mesh, String prefix, String password, int port, int channel, String hostname, String ssid, String stationPassword, bool root, painlessmesh::receivedCallback_t onReceive)
{
    mesh->setDebugMsgTypes(ERROR | STARTUP | CONNECTION); // set before init() so that you can see startup messages

    // Channel set to 6. Make sure to use the same channel for your mesh and for you other
    // network (STATION_SSID)
    mesh->init(prefix,
               password,
               port,
               WIFI_AP_STA,
               channel || 6);
    mesh->onReceive(onReceive);
    mesh->setHostname(hostname.c_str());

    // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
    if (root)
    {
        mesh->stationManual(ssid, stationPassword);
        mesh->setRoot(root);
    }
    // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
    mesh->setContainsRoot(true);
}

IPAddress getlocalIP(painlessMesh *mesh)
{
    return IPAddress(mesh->getStationIP());
}
