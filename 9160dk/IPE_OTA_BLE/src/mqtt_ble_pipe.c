#include <string.h>
#include <stdbool.h>
#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>
#include <stdio.h>


#include "aggregator.h"
#include "mqtt_connection.h"
#include "ble.h"
#include "mqtt_ble_pipe.h"
#include "main.h"
#include "cJSON.h"

LOG_MODULE_DECLARE(lte_ble_gw);


#define SERIALIZED_DATA_MAX_SIZE 300  // Some number. Probably wrong. To-Do: Update.


char *serialize_uplink_packet_to_json(const struct uplink_data_packet *packet) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        LOG_ERR("Failed to create JSON object");
        return NULL;
    }

    switch (packet->type) {
        case uplink_FIRMWARE_ERROR:
            
            cJSON_AddStringToObject(root, "Firmware Error", (char *)packet->data);
            break;

        case uplink_FIRMWARE_ACK:
            
            cJSON_AddStringToObject(root, "Firmware ACK", (char *)packet->data);
            break;

        case uplink_FIRMWARE_VERSION:
            // TODO: Format the firmware version into oneM2M format
            cJSON_AddStringToObject(root, "Firmware Version", (char *)packet->data);
            break;

        case uplink_TRAIN_LOCATION:
            cJSON_AddStringToObject(root, "type", "Train Location");
            cJSON_AddStringToObject(root, "location", (char *)packet->data);
            break;

        default:
            LOG_ERR("Unsupported packet type");
            cJSON_Delete(root);
            return NULL;
    }

    char *jsonString = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return jsonString; // Caller must free this memory with cJSON_free()
}

const char* setTopic(enum uplink_data_packet_type packetType, enum ble_source source) {
    switch (packetType) {
        case uplink_FIRMWARE_ERROR:
        case uplink_FIRMWARE_ACK:
            if (source == SOURCE_ESP32) {
                return "/ESP32/Update";
            } else if (source == SOURCE_RaspberryPi) {
                return "/RaspberryPi/Update";
            }
            break;

        case uplink_FIRMWARE_VERSION:
            return "/oneM2M/resp/capstone-iot/NRFParent/json";  

        case uplink_TRAIN_LOCATION:
            // Location data from Raspberry Pi to Django
            return "/RaspberryPi/Data";


        default:
            LOG_ERR("Unknown data type");
            break;
    }

    // Default topic or error handling
    return "/Unknown/Topic";
}



void publish_aggregated_data(struct k_work *work) {
    ARG_UNUSED(work);
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);

    struct uplink_data_packet data_packet;

    while (uplink_aggregator_get(&data_packet) == 0) {
        // Serialize data packet to JSON
        char *json_payload = serialize_uplink_packet_to_json(&data_packet);
        if (json_payload == NULL) {
            continue;
        }

        // Determine the appropriate topic
        const char *topic = setTopic(data_packet.type, data_packet.source);

        if (topic == NULL) {
            LOG_ERR("Unknown data source");
            cJSON_free(json_payload);
            continue;
        }

        // Publish the JSON payload
        if (data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, json_payload, strlen(json_payload), topic) != 0) {
            LOG_ERR("Failed to publish data to topic %s", topic);
        }

        cJSON_free(json_payload);
    }

    k_work_reschedule(dwork, K_MSEC(100));
}

char *create_json_message_for_ble(const struct downlink_data_packet *packet) {
    if (!packet) {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        // Handle error
        return NULL;
    }

    switch (packet->type) {
        case FIRMWARE_UPDATE:
            cJSON_AddNumberToObject(root, "I", packet->chunk_id); // Assuming chunk_id is a field in your struct
            cJSON_AddStringToObject(root, "D", packet->data);
            char checksumStr[20]; // Make sure this buffer is large enough to hold the checksum as a string
            snprintf(checksumStr, sizeof(checksumStr), "%u", packet->checksum);
            cJSON_AddStringToObject(root, "C", checksumStr);
            break;
        case downlink_TEXT:
            cJSON_AddStringToObject(root, "Message", packet->data);
            break;
        default:
            cJSON_Delete(root);
            return NULL;
    }

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string; // Remember to free this string after use
}


void transmit_aggregated_data(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);

    struct downlink_data_packet packet;
    while (downlink_aggregator_get(&packet) == 0) {
        char *json_message = create_json_message_for_ble(&packet);
        
        if (json_message == NULL) {
            LOG_ERR("Failed to create JSON message");
            continue;
        }
        if (packet.destination == DESTINATION_ESP32){
            ble_transmit("ESP32", (uint8_t *)json_message, strlen(json_message));
        } else if (packet.destination == DESTINATION_RaspberryPi) {
            ble_transmit("RaspberryPi", (uint8_t *)json_message, strlen(json_message));
        } else {
            LOG_ERR("Unknown destination");
            continue;
        }

        free(json_message);
    }

    k_work_reschedule(dwork, K_MSEC(100));
}