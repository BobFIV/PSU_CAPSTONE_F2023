/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "aggregator.h"
#include "mqtt_connection.h"
#include "ble.h"
#include "main.h"
#include "cJSON.h"

LOG_MODULE_DECLARE(lte_ble_gw);

#define SERIALIZED_DATA_MAX_SIZE 300 // Some number. Probably wrong. To-Do: Update.
K_FIFO_DEFINE(uplink_aggregator_fifo);
K_FIFO_DEFINE(downlink_aggregator_fifo);

static uint32_t uplink_entry_count;

static uint32_t downlink_entry_count;

struct uplink_fifo_entry {
	void *fifo_reserved;
	uint8_t data[sizeof(struct uplink_data_packet)];
};

struct downlink_fifo_entry {
	void *fifo_reserved;
	uint8_t data[sizeof(struct downlink_data_packet)];
};

int uplink_aggregator_put(struct uplink_data_packet in_data)
{
	struct uplink_fifo_entry *fifo_data = NULL;
	uint32_t  lock = irq_lock();
	int    err  = 0;

	if (uplink_entry_count == FIFO_MAX_ELEMENT_COUNT) {
		fifo_data = k_fifo_get(&uplink_aggregator_fifo, K_NO_WAIT);

		__ASSERT(fifo_data != NULL, "fifo_data should not be NULL");

		uplink_entry_count--;
	}

	if (fifo_data == NULL) {
		fifo_data = k_malloc(sizeof(struct uplink_fifo_entry));
	}

	if (fifo_data == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	memcpy(fifo_data->data, &in_data, sizeof(in_data));

	k_fifo_put(&uplink_aggregator_fifo, fifo_data);
	uplink_entry_count++;

exit:
	irq_unlock(lock);
	return err;
}

int uplink_aggregator_get(struct uplink_data_packet *out_data)
{
	void  *fifo_data;
	int   err = 0;

	if (out_data == NULL) {
		return -EINVAL;
	}

	uint32_t lock = 0;
	lock = irq_lock();

	fifo_data = k_fifo_get(&uplink_aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		err = -ENODATA;
		goto exit;
	}

	memcpy(out_data, ((struct uplink_fifo_entry *)fifo_data)->data,
	       sizeof(struct uplink_data_packet));

	k_free(fifo_data);
	uplink_entry_count--;

exit:
	irq_unlock(lock);
	return err;
}




int downlink_aggregator_get(struct downlink_data_packet *out_data)
{
	//LOG_INF ("downlink_aggregator_get called"); //Super spammy.
	void  *fifo_data;
	int   err = 0;
	
	if (out_data == NULL) {
		//LOG_INF ("OutData is NULL");
		return -EINVAL;
	}


	fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		err = -ENODATA;
		//LOG_INF ("FIFO is Empty"); //Super spammy.
		goto exit;
	}


	uint32_t lock = irq_lock();


	memcpy(out_data, ((struct downlink_fifo_entry *)fifo_data)->data,
	       sizeof(struct downlink_data_packet));
	LOG_INF ("downlink_aggregator_get: %s", out_data->data);
	k_free(fifo_data);
	downlink_entry_count--;

exit:
	irq_unlock(lock);
	return err;
}

int downlink_aggregator_put(struct downlink_data_packet in_data)
{
	struct downlink_fifo_entry *fifo_data = NULL;
	uint32_t  lock = irq_lock();
	int	err  = 0;
	LOG_INF ("downlink_aggregator_put called");
	if (downlink_entry_count == FIFO_MAX_ELEMENT_COUNT) {
		fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);

		__ASSERT(fifo_data != NULL, "fifo_data should not be NULL");

		downlink_entry_count--;
	}

	if (fifo_data == NULL) {
		fifo_data = k_malloc(sizeof(struct downlink_fifo_entry));
	}

	if (fifo_data == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	
	memcpy(fifo_data->data, &in_data, sizeof(struct downlink_data_packet));
	LOG_INF("downlink_aggregator_put: %s", in_data.data);
	k_fifo_put(&downlink_aggregator_fifo, fifo_data);
	downlink_entry_count++;

exit:
	irq_unlock(lock);
	return err;
}

void aggregator_clear(void)
{
	void *fifo_data;
	uint32_t lock = 0;
	lock = irq_lock();

	while (1) {
		fifo_data = k_fifo_get(&uplink_aggregator_fifo, K_NO_WAIT);

		if (fifo_data == NULL) {
			break;
		}

		k_free(fifo_data);
	};

	uplink_entry_count = 0;

	while (1) {
		fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);

		if (fifo_data == NULL) {
			break;
		}

		k_free(fifo_data);
	};
	downlink_entry_count = 0;
	irq_unlock(lock);
}



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