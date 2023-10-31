#include <string.h>
#include <stdbool.h>
#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>


#include "aggregator.h"
#include "mqtt_connection.h"
#include "ble.h"
#include "mqtt_ble_pipe.h"
#include "main.h"

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

    struct uplink_data_packet data_packet;
    uint8_t serialized_data[SERIALIZED_DATA_MAX_SIZE];
    size_t serialized_length;

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
    k_work_schedule(&periodic_publish_work, K_MSEC(100));
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
    struct downlink_data_packet packet;
    uint8_t serialized_data[SERIALIZED_DATA_MAX_SIZE];
    size_t serialized_length;

    // Continue retrieving and transmitting as long as there's data in the aggregator
    while (downlink_aggregator_get(&packet) == 0) {

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

    k_work_reschedule(&periodic_transmit_work, K_MSEC(100));
}

