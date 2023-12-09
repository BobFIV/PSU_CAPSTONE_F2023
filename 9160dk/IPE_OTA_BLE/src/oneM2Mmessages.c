#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>

#include "oneM2Mmessages.h"
#include "mqtt_connection.h"
#include "cJSON.h"
#include "aggregator.h"
#include "main.h"

LOG_MODULE_DECLARE(lte_ble_gw);

//dmPackage creations are handled in mqtt_connection.c
//This file handles all outgoing oneM2M messages.
//Subscriptions are created here and updates to dmDevice Info are created and sent here.



int createSubscribeMessage()
{

    cJSON *root = cJSON_CreateObject();

    // Value pairs
    cJSON_AddStringToObject(root, "fr", "CNRFParent");
    cJSON_AddNumberToObject(root, "op", 1);
    cJSON_AddStringToObject(root, "rqi", "m_createSUBChild12");
    cJSON_AddNumberToObject(root, "rvi", 4);
    cJSON_AddStringToObject(root, "to", "cse-in/NRFParent");
    cJSON_AddNumberToObject(root, "ty", 23);

    // Create the 'pc' object
    cJSON *pc = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "pc", pc);

    // Create the 'm2m:sub' object
    cJSON *sub = cJSON_CreateObject();
    cJSON_AddItemToObject(pc, "m2m:sub", sub);

    // Add elements to 'm2m:sub'
    cJSON_AddNumberToObject(sub, "nct", 1);
    cJSON_AddStringToObject(sub, "rn", "NotificationRpiNew");

    // Create the 'nu' array
    cJSON *nu = cJSON_CreateArray();
    cJSON_AddItemToArray(nu, cJSON_CreateString("mqtt://52.14.47.13:1883//NRFParent/json"));
    cJSON_AddItemToObject(sub, "nu", nu);

    // Create the 'enc' object
    cJSON *enc = cJSON_CreateObject();
    cJSON_AddItemToObject(sub, "enc", enc);

    // Create the 'net' array and add it to 'enc'
    cJSON *net = cJSON_CreateIntArray((int[]){1, 2, 3, 4}, 4);
    cJSON_AddItemToObject(enc, "net", net);

    data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, (uint8_t *)root->valuestring, strlen(root->valuestring), "cse-in/NRFParent/json");
    cJSON_Delete(root);
    return 0;
}


/*
UpdateFlexContainerForConnectedDevice
This function creates a JSON structure to update the dmDIO (dmDeviceInfo) attribute of a specific device's flex container.
When connected is true, the device is connected and the state is set to 3 (active).
When connected is false, the device is disconnected and the state is set to 1 (inactive).
*/

int updateFlexContainerForConnectedDevice(const char *deviceType, bool connected)  //Called when a device is connected.
{
    // Create the root node
    cJSON *root = cJSON_CreateObject();

    char to[50];    // Set the flex container to update.

    if (strcmp(deviceType, "9160") == 0) {
        snprintf(to, sizeof(to), "cse-in/NRFParent/9160Container");
    } else if (strcmp(deviceType, "ESP32") == 0) {
        snprintf(to, sizeof(to), "cse-in/NRFParent/ESPContainer");
    } else if (strcmp(deviceType, "RPi") == 0) {
        snprintf(to, sizeof(to), "cse-in/NRFParent/RPiContainer");
    } else {
        LOG_ERR("Invalid device type");
        return -1;
    }

    // Add simple key-value pairs
    cJSON_AddNumberToObject(root, "op", 3);
    cJSON_AddStringToObject(root, "to", to);
    cJSON_AddStringToObject(root, "fr", "CAdmin");
    cJSON_AddStringToObject(root, "rqi", "123");
    cJSON_AddStringToObject(root, "rvi", "3");

    // Create the 'pc' object
    cJSON *pc = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "pc", pc);

    // Create the 'mad:dmPae' object
    cJSON *dmDIo = cJSON_CreateObject();
    cJSON_AddItemToObject(pc, "mad:dmDIo", dmDIo);

    // Add elements to 'mad:dmPae'
    cJSON_AddStringToObject(dmDIo, "name", "2.0");
    if (connected) {
        cJSON_AddNumberToObject(dmDIo, "state", 3); //active state is 3 for state.
    } else {
        cJSON_AddNumberToObject(dmDIo, "state", 1); //inactive state is 1 for state
    }
    
    if (strcmp(deviceType, "9160") == 0) {
        data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, (uint8_t *)root->valuestring, strlen(root->valuestring), "cse-in/NRFParent/9160Container/json");
    } else if (strcmp(deviceType, "ESP32") == 0) {
       data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, (uint8_t *)root->valuestring, strlen(root->valuestring), "cse-in/NRFParent/ESPContainer/json");
    } else if (strcmp(deviceType, "RPi") == 0) {
        data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, (uint8_t *)root->valuestring, strlen(root->valuestring), "cse-in/NRFParent/RPiContainer/json");
    } else {
        LOG_ERR("Invalid device type");
        return -1;
    } 

    cJSON_Delete(root);
    return 0;
}

