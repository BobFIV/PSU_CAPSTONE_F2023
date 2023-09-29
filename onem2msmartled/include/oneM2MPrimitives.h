#ifndef onem2mprimitives_h
#define onem2mprimitives_h

#include "WString.h" 


bool Register(void);
bool createContainer(String resourceName);
bool subscribeToContainer(String containerResourceName);
bool createContentInstance(String containerResourceName, String content);
bool parseNotification(uint8_t *datas);

// define function pointer for enable/disble leds
typedef void (*ledHandler)(void);
void setDisableLed(ledHandler handler);
void setEnableLed(ledHandler handler);
#endif