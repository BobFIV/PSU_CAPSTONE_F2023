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


size_t serialize_uplink_data_packet(const struct uplink_data_packet *packet, uint8_t *buffer, size_t buffer_size)
{
    if (!packet || !buffer) {
        return 0;
    }

    size_t serialized_length = 0;

    switch (packet->type) {
        case uplink_TEXT:
            serialized_length = snprintf((char *)buffer, buffer_size, "TYPE:TEXT,DATA:%s", packet->data);
            break;
        case IMAGE_DATA:
            // Might want to encode before sending...
            serialized_length = snprintf((char *)buffer, buffer_size, "TYPE:IMAGE,DATA:%s", "IMAGE_DATA_PLACEHOLDER");
            break;
        default:
            // Unknown type
            return 0;
    }

    return serialized_length;
}

void publish_aggregated_data(struct k_work *work)
{
    ARG_UNUSED(work);
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);

    struct uplink_data_packet data_packet;
    uint8_t serialized_data[SERIALIZED_DATA_MAX_SIZE];
    size_t serialized_length;
    const char *jsonString = "{\"op\": 2, \"to\": \"cse-in/NRFParent\", \"fr\": \"CNRFParent\", \"rqi\": \"123\", \"rvi\": \"3\", \"ty\": 4, \"rcn\": 8}";
    cJSON *jsonObject = cJSON_Parse(jsonString);
    if (jsonObject == NULL) {
        LOG_ERR("Error parsing JSON\n");
    }
    cJSON *op = cJSON_GetObjectItem(jsonObject, "op");
    cJSON *to = cJSON_GetObjectItem(jsonObject, "to");
    cJSON *fr = cJSON_GetObjectItem(jsonObject, "fr");
    cJSON *rqi = cJSON_GetObjectItem(jsonObject, "rqi");
    cJSON *rvi = cJSON_GetObjectItem(jsonObject, "rvi");
    cJSON *ty = cJSON_GetObjectItem(jsonObject, "ty");
    cJSON *rcn = cJSON_GetObjectItem(jsonObject, "rcn");







    while (uplink_aggregator_get(&data_packet) == 0) {
        //Serialize the data packet
        serialized_length = serialize_uplink_data_packet(&data_packet, serialized_data, sizeof(serialized_data));
        if (serialized_length == 0) {
            LOG_ERR("Failed to serialize data packet");
            continue;
        }
        
        //Publish the data packet
        if (data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE, serialized_data, serialized_length) != 0) {
            LOG_ERR("Failed to publish data");
        }
    }
    k_work_reschedule(dwork, K_MSEC(100));  //Reschedule the work after 100 milliseconds
    cJSON_Delete(jsonObject);

}



size_t serialize_downlink_data_packet(const struct downlink_data_packet *packet, uint8_t *buffer, size_t buffer_size)
{
    if (!packet || !buffer) {
        return 0;
    }

    size_t serialized_length = 0;

    switch (packet->type) {
        case downlink_TEXT:
            serialized_length = snprintf((char *)buffer, buffer_size, "TYPE:TEXT,DATA:%s", packet->data);
            break;
        case FIRMWARE_UPDATE: 
            serialized_length = snprintf((char *)buffer, buffer_size, "TYPE:FIRMWARE,DATA:%s", packet->data);
            break;
        default:
            // Unknown type
            return 0;
    }

    return serialized_length;
}

void transmit_aggregated_data(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);

    struct downlink_data_packet packet;
    uint8_t serialized_data[SERIALIZED_DATA_MAX_SIZE];
    size_t serialized_length;
    //LOG_INF("Spam!");
    // Continue retrieving and transmitting as long as there's data in the aggregator
    while (downlink_aggregator_get(&packet) == 0) {
       // LOG_INF("Preparing to tranmit downlink data packet");
        // Serialize the data packet (if necessary)
        serialized_length = serialize_downlink_data_packet(&packet, serialized_data, sizeof(serialized_data));
        if (serialized_length == 0) {
            LOG_ERR("Failed to serialize downlink data packet");
            continue;
        }

        // Get the connection for the packet's destination
        struct bt_conn *target_conn = get_conn_from_destination(packet.destination);
        if (!target_conn) {
            LOG_ERR("No connection found for destination %d", packet.destination);
            continue;
        }

        // Transmit the serialized data over BLE
        ble_transmit(target_conn, serialized_data, serialized_length);
    }

    k_work_reschedule(dwork, K_MSEC(100));  // Updated this line
}
