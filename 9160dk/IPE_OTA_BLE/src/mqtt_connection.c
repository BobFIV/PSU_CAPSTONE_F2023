#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

#include <zephyr/sys/base64.h>		//Decode Base64.
#include <zephyr/sys/crc.h>			//For CRC32 checksum
#include <zephyr/sys/atomic.h>		//For atomic operations in counting and checking chunks. Not sure if necessary, but good to have when updates start flying.

#include "mqtt_connection.h"
#include "update.h"
#include "cJSON.h"
#include "aggregator.h"
#include "main.h"

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

#define MAX_CHUNK_SIZE 247 //Max size of a chunk of firmware update.
/* MQTT Broker details. */
static struct sockaddr_storage broker;

LOG_MODULE_DECLARE(lte_ble_gw);


char* strdup(const char* s) {
    char* new_str = malloc(strlen(s) + 1);
    if (new_str) {
        strcpy(new_str, s);
    }
    return new_str;
}

typedef struct {
    char *deviceName;
    char *deviceUpdateVersion;
    long totalUpdateSize;
    int chunkSize;
    int totalChunks;
    char *url;
} PackageInfo;


void freePackageInfo(PackageInfo *info);

/**@brief Function to get the payload of recived data.
 */
static int get_received_payload(struct mqtt_client *c, size_t length)
{
	int ret;
	int err = 0;

	/* Return an error if the payload is larger than the payload buffer.
	 * Note: To allow new messages, we have to read the payload before returning.
	 */
	if (length > sizeof(payload_buf)) {
		err = -EMSGSIZE;
	}

	/* Truncate payload until it fits in the payload buffer. */
	while (length > sizeof(payload_buf)) {
		ret = mqtt_read_publish_payload_blocking(
				c, payload_buf, (length - sizeof(payload_buf)));
		if (ret == 0) {
			return -EIO;
		} else if (ret < 0) {
			return ret;
		}

		length -= ret;
	}

	ret = mqtt_readall_publish_payload(c, payload_buf, length);
	if (ret) {
		return ret;
	}

	return err;
}

static int subscribe(struct mqtt_client *const c)
{
    struct mqtt_topic subscribe_topics[] = {
        {
            .topic = {
                .utf8 = CONFIG_MQTT_SUB_TOPIC, // Subscribes to the oneM2M topic
                .size = strlen(CONFIG_MQTT_SUB_TOPIC)
            },
            .qos = MQTT_QOS_1_AT_LEAST_ONCE
        },
        {
            .topic = {
                .utf8 = "/ESP32/Update", // Subscribes to the ESP32 Update topic
                .size = strlen("/ESP32/Update")
            },
            .qos = MQTT_QOS_1_AT_LEAST_ONCE
        },
		{
            .topic = {
                .utf8 = "/9160/Update", // Subscribes to the 9160 Update topic
                .size = strlen("/9160/Update")
            },
            .qos = MQTT_QOS_1_AT_LEAST_ONCE
        },
		{
            .topic = {
                .utf8 = "/RaspberryPi/Update", // Subscribes to the RaspberryPi Update topic
                .size = strlen("/RaspberryPi/Update")
            },
            .qos = MQTT_QOS_1_AT_LEAST_ONCE
        },
		{
            .topic = {
                .utf8 = "/oneM2M/req/capstone-iot/Notif", // Subscribes to the oneM2M notification topic
                .size = strlen("/oneM2M/req/capstone-iot/Notif")
            },
            .qos = MQTT_QOS_1_AT_LEAST_ONCE
        },
        
    };

    const struct mqtt_subscription_list subscription_list = {
        .list = subscribe_topics,
        .list_count = ARRAY_SIZE(subscribe_topics), // Number of topics
        .message_id = 1234 // Unique message ID
    };

    LOG_INF("Subscribing to multiple topics");
    for (int i = 0; i < ARRAY_SIZE(subscribe_topics); i++) {
        LOG_INF("Topic %d: %s", i, subscribe_topics[i].topic.utf8);
    }
	led_condition &= ~CONDITION_MQTT_CONNECTING;
	led_condition |= CONDITION_MQTT_LTE_CONNECTED;
    return mqtt_subscribe(c, &subscription_list);
}


/**@brief Function to print strings without null-termination
 */
static void data_print(uint8_t *prefix, uint8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	LOG_INF("%s%s", (char *)prefix, (char *)buf);
}




//Publishes data on the specified topic.
int data_publish(struct mqtt_client *c, enum mqtt_qos qos, uint8_t *data, size_t len, const char *pub_topic)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = pub_topic; // Use the provided topic
	param.message.topic.topic.size = strlen(pub_topic);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	data_print("Publishing: ", data, len);
	LOG_INF("to topic: %s len: %u", pub_topic, (unsigned int)strlen(pub_topic));

	return mqtt_publish(c, &param);
}


char *createReadyPayload(int expectedChunks, const char *expectedSize) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        LOG_ERR("Failed to create JSON object");
        return NULL;
    }

    // Create combined string payload.
    char combinedStr[256];  
    snprintf(combinedStr, sizeof(combinedStr), "Expected Chunks: %d Expected Size: %s", expectedChunks, expectedSize);

    cJSON_AddStringToObject(root, "Firmware ACK", combinedStr); //add ack header to payload.

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return payload; 
}

void publishUpdateReady(struct mqtt_client *client, const char *topic, int expectedChunks, const char *expectedSize) {
    char *payload = createReadyPayload(expectedChunks, expectedSize);
    if (payload == NULL) {
        // Handle error
        return;
    }

    int err = data_publish(client, MQTT_QOS_1_AT_LEAST_ONCE, payload, strlen(payload), topic);
    if (err) {
        LOG_ERR("Failed to publish message: %d", err);
    }

    cJSON_free(payload);
}


//Function to create the error message sent up to the broker during update.
char *create_json_error_message(int expected_chunk_id) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        // Handle error
        return NULL;
    }
    if (expected_chunk_id == -1) {				//If an error id of -1 is passed, this means update chunks were sent without initialization message.
        cJSON_AddStringToObject(root, "message", "Not Ready for Update");
    } else {
        cJSON_AddNumberToObject(root, "Packet Lost at", expected_chunk_id);
    }
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_string; // Caller is responsible for freeing the memory
}

void publish_error_message(struct mqtt_client *mqtt_client, int expected_chunk_id, const char *topic) {
    char *error_message = create_json_error_message(expected_chunk_id);
    if (error_message == NULL) {
        // Handle error in creating JSON string
        return;
    }
    int err = data_publish(mqtt_client, MQTT_QOS_1_AT_LEAST_ONCE, (uint8_t *)error_message,
                           strlen(error_message), topic); //Publish the error message to the topic of origin.
    if (err) {
        // Handle error in publishing
        LOG_ERR("Error publishing: %d", err);
    }

    free(error_message); // Free the allocated JSON string
}


void incrementRpiChunk() {
    // Atomically increments the value of currentRPiChunk
    atomic_inc(&currentRPiChunk);
}

int getCurrentRpiChunk() {
    // Atomically reads the value of currentRPiChunk
    return atomic_get(&currentRPiChunk);
}

void incrementESP32Chunk() {
    // Atomically increments the value of currentRPiChunk
    atomic_inc(&currentESP32Chunk);
}

int getCurrentESP32Chunk() {
    // Atomically reads the value of currentRPiChunk
    return atomic_get(&currentESP32Chunk);
}

void increment9160Chunk() {
    // Atomically increments the value of currentRPiChunk
    atomic_inc(&current9160Chunk);
}

int getCurrent9160Chunk() {
    // Atomically reads the value of currentRPiChunk
    return atomic_get(&current9160Chunk);
}


typedef struct {
    const char *topic;
	int (*getCurrentChunk)(void);
    bool *flag;
} TopicMapping;


int chunkErrorChecker(struct mqtt_client *mqtt_client, const char *topic, int received_chunk_id, cJSON *data, uint32_t received_checksum) {
    // Define topic mappings
    TopicMapping topics[] = {
        {"/9160/Update", getCurrent9160Chunk, &update9160Flag},
        {"/ESP32/Update", getCurrentESP32Chunk, &updateESP32Flag},
        {"/RaspberryPi/Update", getCurrentRpiChunk, &updateRPiFlag},
        {NULL, NULL, NULL} // Sentinel to mark end of array
    };

    // Decode the data
    size_t decoded_len;
    unsigned char decoded_data[MAX_CHUNK_SIZE];
    int err = base64_decode(decoded_data, sizeof(decoded_data), &decoded_len, 
                           (const unsigned char *)data->valuestring, strlen(data->valuestring));
    if (err) {
        LOG_ERR("Failed to decode package.");
        return err;
    }

    // Calculate checksum
    uint32_t calculated_checksum = crc32_ieee(decoded_data, decoded_len);

    // Check each topic
    for (int i = 0; topics[i].topic != NULL; i++) {
        if (strncmp(topic, topics[i].topic, strlen(topics[i].topic)) == 0) {
            if (*(topics[i].flag) != true) {							//if the flag is not set for the update, error. 
                LOG_ERR("Not ready to receive firmware update for %s", topics[i].topic);
                publish_error_message(mqtt_client, -1, topics[i].topic);
                return -1;
            }
			int currentChunk = topics[i].getCurrentChunk();				//gets the current chunk
            if (currentChunk != received_chunk_id) {
                LOG_ERR("Current chunk id does not match the chunk id of the received chunk");
                publish_error_message(mqtt_client, currentChunk, topics[i].topic);
                return -1;
            }else if (calculated_checksum != received_checksum) {
                LOG_ERR("Checksum Failed.");
                publish_error_message(mqtt_client, currentChunk, topics[i].topic);	//functionally, you are requesting the same exact package. These are separate for LOG purposes.
                return -1;
            }

            return 0; // Success for matched topic
        }
    }
    // If no topic matched
    LOG_ERR("Unrecognized topic for update chunk");
    publish_error_message(mqtt_client, -1, topic);
    return -1;
}



PackageInfo* extractPackage(const char *jsonString)
{
    // Parse the JSON string
    cJSON *root = cJSON_Parse(jsonString);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return NULL;
    }

    PackageInfo *info = malloc(sizeof(PackageInfo));
    if (info == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *pc = cJSON_GetObjectItemCaseSensitive(root, "pc");
    cJSON *sgn = pc ? cJSON_GetObjectItemCaseSensitive(pc, "m2m:sgn") : NULL;
    cJSON *nev = sgn ? cJSON_GetObjectItemCaseSensitive(sgn, "nev") : NULL;
    cJSON *rep = nev ? cJSON_GetObjectItemCaseSensitive(nev, "rep") : NULL;
    cJSON *dmPae = rep ? cJSON_GetObjectItemCaseSensitive(rep, "mad:dmPae") : NULL;

    if (dmPae) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(dmPae, "name");
        cJSON *url = cJSON_GetObjectItemCaseSensitive(dmPae, "url");

        if (name && cJSON_IsString(name) && (name->valuestring != NULL)) {
            char *token;
            char *nameStr = strdup(name->valuestring);

            // Extract deviceName
			token = strtok(nameStr, " :");
			info->deviceName = token ? strdup(token) : NULL;


			char *version = strtok(NULL, " :");
			char *totalSizeStr = strtok(NULL, " :");
			char *chunkSizeStr = strtok(NULL, " :");
			char *totalChunksStr = strtok(NULL, " :");


			int totalChunks = totalChunksStr ? atoi(totalChunksStr) : 0;

			info->deviceUpdateVersion = version ? strdup(version) : NULL;
			info->totalUpdateSize = totalSizeStr ? strtol(totalSizeStr, NULL, 10) : 0;
			info->chunkSize = chunkSizeStr ? atoi(chunkSizeStr) : 0;
			info->totalChunks = totalChunksStr ? atoi(totalChunksStr) : 0;


            // Set flags and chunks based on the device name
            if (token && strcmp(token, "rpi") == 0) {
                if (updateRPiFlag) {
                    LOG_ERR("RaspberryPi is already updating");
                    freePackageInfo(info);
                    cJSON_Delete(root);
                    return NULL;
                }
                totalRPiChunks = totalChunks;
                updateRPiFlag = true;
            } else if (token && strcmp(token, "esp32") == 0) {
                if (updateESP32Flag) {
                    LOG_ERR("ESP32 is already updating");
                    freePackageInfo(info);
                    cJSON_Delete(root);
                    return NULL;
                }
                totalESP32Chunks = totalChunks;
                updateESP32Flag = true;
            } else if (token && strcmp(token, "9160") == 0) {
                if (update9160Flag) {
                    LOG_ERR("9160 is already updating");
                    freePackageInfo(info);
                    cJSON_Delete(root);
                    return NULL;
                }
                total9160Chunks = totalChunks;
                update9160Flag = true;
            } else {
                LOG_ERR("Invalid device name");
                freePackageInfo(info);
                cJSON_Delete(root);
                return NULL;
            }
            free(nameStr);
        }

        // Extract the 'url' field
        if (url && cJSON_IsString(url) && (url->valuestring != NULL)) {
            info->url = strdup(url->valuestring);
        }
    }

    cJSON_Delete(root);
    return info; // The caller is responsible for freeing this memory
}

void freePackageInfo(PackageInfo *info) {
    if (info != NULL) {
        free(info->deviceName);
        free(info->deviceUpdateVersion);
        free(info->url);
        free(info);
    }
}



/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
	/* STEP 5 - Subscribe to the topic CONFIG_MQTT_SUB_TOPIC when we have a successful connection */
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed: %d", evt->result);
			break;
		}

		LOG_INF("MQTT client connected");
		subscribe(c);
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected: %d", evt->result);
		break;

	case MQTT_EVT_PUBLISH:
	{
		const struct mqtt_publish_param *p = &evt->param.publish;
        printk("Received message on topic: %.*s\n", p->message.topic.topic.size, p->message.topic.topic.utf8);
		//Extract the data of the recived message 
		err = get_received_payload(c, p->message.payload.len);
		
		//Send acknowledgment to the broker on receiving QoS1 publish message 
		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgment. */
			mqtt_publish_qos1_ack(c, &ack);
		}

		

		cJSON *json_payload = cJSON_Parse(payload_buf);
		if (!json_payload) {
			LOG_ERR("Failed to parse JSON payload");
			return;
		}
		if (strncmp(p->message.topic.topic.utf8, "/oneM2M/req/capstone-iot/NRFParent/json", p->message.topic.topic.size) == 0) {
        // Handle message for the /oneM2M/req/capstone-iot/NRFParent/json topic

			if (err >= 0) {
				// Extract relevant fields from the payload
				cJSON *rqi = cJSON_GetObjectItemCaseSensitive(json_payload, "rqi");
				cJSON *ot = cJSON_GetObjectItemCaseSensitive(json_payload, "ot");
				// Add more fields as per your oneM2M requirements

				if (rqi && cJSON_IsString(rqi) && (rqi->valuestring != NULL)) {
					// Process the message based on the extracted data
					// ...

					// Prepare a response message
					cJSON *response_json = cJSON_CreateObject();
					cJSON_AddStringToObject(response_json, "rqi", rqi->valuestring);
					cJSON_AddStringToObject(response_json, "ot", ot ? ot->valuestring : "");
					cJSON_AddNumberToObject(response_json, "rsc", 2001); // Response Status Code

					char *response_str = cJSON_PrintUnformatted(response_json);
					if (response_str) {
						// Publish the response
						struct mqtt_publish_param publish_param;
						memset(&publish_param, 0, sizeof(publish_param));
						publish_param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
						publish_param.message.topic.topic.utf8 = "/oneM2M/resp/capstone-iot/NRFParent/json";
						publish_param.message.topic.topic.size = strlen("/oneM2M/resp/capstone-iot/NRFParent/json");
						publish_param.message.payload.data = response_str;
						publish_param.message.payload.len = strlen(response_str);

						err = mqtt_publish(c, &publish_param);
						if (err) {
							LOG_ERR("Failed to publish response: %d", err);
						}

						free(response_str);
					}

					cJSON_Delete(response_json);
				} else {
					LOG_ERR("Required fields missing in the payload");
				}

				cJSON_Delete(json_payload);
			} else if (err == -EMSGSIZE) {
				LOG_ERR("Received payload is larger than buffer.");
			} else {
				LOG_ERR("Failed to extract payload: %d", err);
			}
   		} 		// Check for the type of message
		/*
		if (cJSON_HasObjectItem(json_payload, "Total Update Chunks")) { //In this case its a message regarding update.
			cJSON *total_chunks = cJSON_GetObjectItem(json_payload, "Total Update Chunks");
			cJSON *total_size = cJSON_GetObjectItem(json_payload, "Total Size");

			if (!cJSON_IsNumber(total_chunks) || !cJSON_IsString(total_size)) {
				LOG_ERR("Invalid format for update metadata");
				cJSON_Delete(json_payload);
				return;
			}

			struct downlink_data_packet packet;
			packet.type = downlink_TEXT; // Set type to text for metadata
			packet.length = snprintf(packet.data, ENTRY_MAX_SIZE, "Total Chunks: %d, Size: %s",
									total_chunks->valueint, total_size->valuestring);
			packet.chunk_id = -1;
			if (strncmp(p->message.topic.topic.utf8, "/ESP32/Update", p->message.topic.topic.size) == 0) {
			// Handle Initialization Message for /ESP32/Update
				if (updateESP32Flag == true)
				{
					LOG_ERR("ESP32 is already updating");			//TO-DO: Send error message to broker about ongoing update.
					cJSON_Delete(json_payload);
					return;
				}
				totalESP32Chunks = total_chunks->valueint;
				updateESP32Flag = true;
				packet.destination = DESTINATION_ESP32;
				led_condition &= ~CONDITION_ESP32_CONNECTED;
				led_condition |= CONDITION_ESP32_UPDATING;
			} else if (strncmp(p->message.topic.topic.utf8, "/9160/Update", p->message.topic.topic.size) == 0) {
				// Handle message for the /9160/Update topic
				if (update9160Flag == true)
				{
					LOG_ERR("9160 is already updating");			//TO-DO: Send error message to broker about ongoing update.
					cJSON_Delete(json_payload);
					return;
				}
				total9160Chunks = total_chunks->valueint;
				update9160Flag = true;
				led_condition |= CONDITION_FIRMWARE_UPDATING;
				publishUpdateReady(c, "/9160/Update", total9160Chunks, total_size->valuestring); //Publish ready to update message for 9160. 
				LOG_INF("Ready to receive updates for 9160!");
			} else if (strncmp(p->message.topic.topic.utf8, "/RaspberryPi/Update", p->message.topic.topic.size) == 0) {
				// Handle message for the /RaspberryPi/Update topic
				if (updateRPiFlag == true)
				{
					LOG_ERR("RaspberryPi is already updating");			//TO-DO: Send error message to broker about ongoing update.
					cJSON_Delete(json_payload);
					return;
				}
				totalRPiChunks = total_chunks->valueint;
				updateRPiFlag = true;
				LOG_INF("Ready to receive updates for RaspberryPi!");
				packet.destination= DESTINATION_RaspberryPi;
				led_condition &= ~CONDITION_RPI_CONNECTED;
				led_condition |= CONDITION_RPI_UPDATING;
			} else {
				LOG_WRN("Receieved message on unknown topic: %.*s",
					p->message.topic.topic.size,
					p->message.topic.topic.utf8);
				cJSON_Delete(json_payload);
				return;
			}
			
			downlink_aggregator_put(packet);
			cJSON_Delete(json_payload);
		*/else if (strncmp(p->message.topic.topic.utf8, "/oneM2M/req/capstone-iot/Notif", p->message.topic.topic.size) == 0){	//if the message is in the NOTIFY topic for the oneM2M subscription
			PackageInfo *pkgInfo = extractPackage(payload_buf);
			if (pkgInfo == NULL) {
				fprintf(stderr, "Failed to extract package info\n");
				return;
			}
			struct downlink_data_packet packet;
			memset(&packet, 0, sizeof(packet)); // Initialize to zero
			if (strcmp(pkgInfo->deviceName, "esp32") == 0) {
				packet.destination = DESTINATION_ESP32;
			} else if (strcmp(pkgInfo->deviceName, "rpi") == 0) {
				packet.destination = DESTINATION_RaspberryPi;
			} else {
				packet.destination = DESTINATION_UNKNOWN;
			}

			snprintf(packet.data, ENTRY_MAX_SIZE, "Version: %s, Size: %ld, Chunks: %d, Chunk Size: %d, URL: %s",
						pkgInfo->deviceUpdateVersion, 
						pkgInfo->totalUpdateSize, 
						pkgInfo->totalChunks, 
						pkgInfo->chunkSize, 
						pkgInfo->url);
			packet.type = FIRMWARE_UPDATE;

			cJSON_Delete(json_payload);
			free(pkgInfo);

		} else if (cJSON_HasObjectItem(json_payload, "I") && cJSON_HasObjectItem(json_payload, "D") && cJSON_HasObjectItem(json_payload, "C")) { 
			//In this case its a firmware update
			cJSON *chunk_id = cJSON_GetObjectItem(json_payload, "I");
			cJSON *data = cJSON_GetObjectItem(json_payload, "D");
			uint32_t received_checksum = (uint32_t)cJSON_GetObjectItem(json_payload, "C")->valuedouble;
			if (!cJSON_IsNumber(chunk_id) || !cJSON_IsString(data)) {
				LOG_ERR("Invalid format for firmware chunk");
				cJSON_Delete(json_payload);
				return;
			}
			
			int received_chunk_id = chunk_id->valueint; 
			err = chunkErrorChecker(c, p->message.topic.topic.utf8, received_chunk_id, data, received_checksum); // error checking for packet		
			if (err) {
				LOG_ERR("Error in chunkErrorChecker");
				cJSON_Delete(json_payload);
				return;
			}

			struct downlink_data_packet packet;
			packet.type = FIRMWARE_UPDATE; // Set type to firmware update
			size_t payload_length = p->message.payload.len;
			if (payload_length > ENTRY_MAX_SIZE) {				// Ensure buffer size
				LOG_ERR("Payload size exceeds buffer size");
				cJSON_Delete(json_payload);
				return;
			}
			packet.length = payload_length;
			memcpy(packet.data, p->message.payload.data, payload_length);
			packet.chunk_id = chunk_id->valueint;
			packet.checksum = received_checksum;
			if (strncmp(p->message.topic.topic.utf8, "/ESP32/Update", p->message.topic.topic.size) == 0) {
				// Handle message for the /ESP32/Update topic
				packet.destination = DESTINATION_ESP32;
				downlink_aggregator_put(packet);
				incrementESP32Chunk();
			} else if (strncmp(p->message.topic.topic.utf8, "/9160/Update", p->message.topic.topic.size) == 0) {
				// Handle message for the /9160/Update topic
				size_t decoded_len;
    			unsigned char decoded_data[MAX_CHUNK_SIZE]; //Max Len 273 Bytes. 
				err = base64_decode(decoded_data, sizeof(decoded_data), &decoded_len, 
                            (const unsigned char *)data->valuestring, strlen(data->valuestring));
				if (err)
				{
					LOG_ERR("Failed to decode package.");
					cJSON_Delete(json_payload);
					return;
				}
				int chunkIdValue = cJSON_GetNumberValue(chunk_id); 
				err = process_firmware_chunk(chunkIdValue, decoded_data, decoded_len);	
				if (err)
				{
					LOG_ERR("Failed to process firmware chunk for 9160");
					cJSON_Delete(json_payload);
					return;
				}
				increment9160Chunk();
				//current9160Chunk is cumulated in process chunk.
			} else if (strncmp(p->message.topic.topic.utf8, "/RaspberryPi/Update", p->message.topic.topic.size) == 0) {
				// Handle message for the /RaspberryPi/Update topic
				packet.destination = DESTINATION_RaspberryPi;
				downlink_aggregator_put(packet);
				incrementRpiChunk();
			}
			cJSON_Delete(json_payload);
		} else {
			LOG_ERR("Unrecognized message format");
        } 
		break;
	} 
	

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error: %d", evt->result);
			break;
		}

		LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT SUBACK error: %d", evt->result);
			break;
		}

		LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
		break;

	case MQTT_EVT_PINGRESP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PINGRESP error: %d", evt->result);
		}
		break;

	default:
		LOG_INF("Unhandled MQTT event type: %d", evt->type);
		break;
	}
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static int broker_init(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_MQTT_BROKER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo failed: %d", err);
		return -ECHILD;
	}

	addr = result;

	/* Look for address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
			LOG_INF("IPv4 Address found %s", (char *)(ipv4_addr));

			break;
		} else {
			LOG_ERR("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return err;
}

/* Function to get the client id */
static const uint8_t* client_id_get(void)
{
	static uint8_t client_id[MAX(sizeof(CONFIG_MQTT_CLIENT_ID),
				     CLIENT_ID_LEN)];

	if (strlen(CONFIG_MQTT_CLIENT_ID) > 0) {
		snprintf(client_id, sizeof(client_id), "%s",
			 CONFIG_MQTT_CLIENT_ID);
		goto exit;
	}

	char imei_buf[CGSN_RESPONSE_LENGTH + 1];
	int err;

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		goto exit;
	}

	imei_buf[IMEI_LEN] = '\0';

	snprintf(client_id, sizeof(client_id), "nrf-%.*s", IMEI_LEN, imei_buf);

exit:
	LOG_DBG("client_id = %s", (char *)(client_id));

	return client_id;
}


/**@brief Initialize the MQTT client structure
 */
/* STEP 3 - Define the function client_init() to initialize the MQTT client instance.  */
int client_init(struct mqtt_client *client)
{
	int err;
	/* Initializes the client instance. */
	mqtt_client_init(client);

	/* Resolves the configured hostname and initializes the MQTT broker structure */
	err = broker_init();
	if (err) {
		LOG_ERR("Failed to initialize broker connection");
		return err;
	}

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = client_id_get();
	client->client_id.size = strlen(client->client_id.utf8);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* We are not using TLS in Exercise 1 */
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;


	return err;
}

/**@brief Initialize the file descriptor structure used by poll.
 */
int fds_init(struct mqtt_client *c, struct pollfd *fds)
{
	if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds->fd = c->transport.tcp.sock;
	} else {
		return -ENOTSUP;
	}

	fds->events = POLLIN;

	return 0;
}