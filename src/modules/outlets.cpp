#include "modules/outlets.h"

void setupOutlet(uint8_t pin, bool state)
{
    digitalWrite(pin, state ? LOW : HIGH);
}