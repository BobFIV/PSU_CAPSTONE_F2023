#pragma once

// create a function pointer that takes a pointer to a uint8_t
typedef bool (*notificationHandler)(uint8_t *datas);

void setupWifi(void);
void notificationListener();
void setNotificationHandler(notificationHandler handler);