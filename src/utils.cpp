#include "utils.h"


IPAddress getlocalIP(painlessMesh *mesh)
{
    return IPAddress(mesh->getStationIP());
}