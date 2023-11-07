/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AGGREGATOR_H_
#define _AGGREGATOR_H_
#include <stdint.h>


/*
 * Simple aggregator module which can have sensordata added and retrieved in a
 * FIFO fashion. The queue allows TLD formated data to be aggregated from
 * sensors in a FIFO fashion. When some event in the future causes data
 * transfer to start, the most recent X datapoints (determined by ENTRY_MAX_SIZE
 * and HEAP_MEM_POOL_SIZE) can be transferred.
 *
 * Note: The module is single instance to allow simple global access to the
 * data.
 * 
 * This is edited from the original sample program.
 */


#define ENTRY_MAX_SIZE (100)				//This is probably wrong. Will have to change later.
#define FIFO_MAX_ELEMENT_COUNT 12

enum uplink_data_packet_type {						//Data types for going up are text and image.
	uplink_TEXT,
    IMAGE_DATA,
};
enum ble_source {
	SOURCE_ESP32,
	SOURCE_RaspberryPi,
};

struct uplink_data_packet {						//This is the data structure that will be used to store data going upwards to MQTT.
	uint8_t length;
	enum uplink_data_packet_type type;
	enum ble_source source;
	uint8_t data[ENTRY_MAX_SIZE];
};

enum downlink_data_packet_type {					//Data types for going down are firmware update and image.
	downlink_TEXT,
	FIRMWARE_UPDATE,
};
enum ble_destination {					//Destination for data going down is either ESP32 or Raspberry Pi.
	DESTINATION_ESP32,					
	DESTINATION_RaspberryPi,
	DESTINATION_UNKNOWN,
};

struct downlink_data_packet {				//This is the data structure that is used to store data going downwards to BLE.
	uint8_t length;
	enum downlink_data_packet_type type;
	enum ble_destination destination;
	uint8_t data[ENTRY_MAX_SIZE];
	
	char version[8];            // Assuming max version string length is 7 (e.g., "1.0.2") + null terminator
    uint32_t chunk;             // Suitable for a large number of chunks
    uint32_t total_chunks;      // Same as above
    size_t data_length;         // To keep track of the actual length of decoded data
    char checksum[65]; 
};
/*
// new struct for firmware update downlink
struct firmware_update_packet_t{
    char version[8];            // Assuming max version string length is 7 (e.g., "1.0.2") + null terminator
    enum ble_destination destination;       // "esp32" can fit easily within this, with some room to spare
    uint32_t chunk;             // Suitable for a large number of chunks
    uint32_t total_chunks;      // Same as above
    uint8_t data[ENTRY_MAX_SIZE];         // Adjust this size based on your expected max data length after decoding from Base64
    size_t data_length;         // To keep track of the actual length of decoded data
    char checksum[65];          // SHA-256 results in a 64-character hexadecimal number + null terminator
};
*/




int aggregator_init(void);

int uplink_aggregator_put(struct uplink_data_packet data);

int uplink_aggregator_get(struct uplink_data_packet *data);

int downlink_aggregator_put(struct downlink_data_packet data);

int downlink_aggregator_get(struct downlink_data_packet *data);

void aggregator_clear(void);

int aggregator_element_count_get(void);
/*
// new function for firmware update downlink
int fdownlink_aggregator_put(struct firmware_update_packet_t data);

int fdownlink_aggregator_get(struct firmware_update_packet_t *data);
*/
#endif /* _AGGREGATOR_H_ */
