#ifndef oneM2MMessages_h
#define oneM2MMessages_h

#include "cJSON.h"
int createSubscribeMessage();

int updateFlexContainerForConnectedDevice(const char *deviceType, bool connected);

#endif /* oneM2MMessages_h */
