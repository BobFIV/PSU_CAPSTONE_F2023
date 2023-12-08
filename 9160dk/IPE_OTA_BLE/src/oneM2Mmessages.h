#ifndef oneM2MMessages_h
#define oneM2MMessages_h

#include "cJSON.h"
cJSON* createSubscribeMessage();

cJSON* updateFlexContainerForConnectedDevice(const char *deviceType, bool connected);

#endif /* oneM2MMessages_h */
