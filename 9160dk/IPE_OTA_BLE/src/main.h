#ifndef MAIN_H
#define MAIN_H
#include <stdint.h>


//why the heck do I need to do this...
extern struct mqtt_client client;
extern struct k_work_delayable periodic_publish_work;
extern struct k_work_delayable periodic_transmit_work;

extern uint32_t led_condition;
#define CONDITION_ESP32_CONNECTING   (1 << 0)
#define CONDITION_ESP32_SCANNING     (1 << 1)
#define CONDITION_ESP32_UPDATING     (1 << 2)
#define CONDITION_ESP32_CONNECTED    (1 << 3)
#define CONDITION_RPI_CONNECTING     (1 << 4)
#define CONDITION_RPI_SCANNING       (1 << 5)
#define CONDITION_RPI_UPDATING       (1 << 6)
#define CONDITION_RPI_CONNECTED      (1 << 7)
#define CONDITION_LTE_CONNECTING     (1 << 8)
#define CONDITION_MQTT_CONNECTING    (1 << 9)
#define CONDITION_MQTT_LTE_CONNECTED (1 << 10)
#define CONDITION_FIRMWARE_UPDATING  (1 << 11)
#define CONDITION_FIRMWARE_DONE      (1 << 12)

#endif // MAIN_H
