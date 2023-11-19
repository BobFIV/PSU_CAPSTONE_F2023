#ifndef _MQTTCONNECTION_H_
#define _MQTTCONNECTION_H_
#define LED_CONTROL_OVER_MQTT          DK_LED1 /*The LED to control over MQTT*/
#define IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (IMEI_LEN + 6 + 1) /* Add 6 for \r\nOK\r\n and 1 for \0 */
#define CLIENT_ID_LEN sizeof("nrf-") + IMEI_LEN
#include <zephyr/net/socket.h>




/**@brief Initialize the MQTT client structure
 */
int client_init(struct mqtt_client *client);

/**@brief Initialize the file descriptor structure used by poll.
 */
int fds_init(struct mqtt_client *c, struct pollfd *fds);

/**@brief Function to publish data on the configured topic
 */
int data_publish(struct mqtt_client *c, enum mqtt_qos qos, uint8_t *data, size_t len, const char *pub_topic);


/**@brief Function to publish the missing packet error message to the specified topic for the specified chunk id.
 */
void publish_error_message(struct mqtt_client *mqtt_client, int expected_chunk_id, const char *topic);

/**@brief Function to publish the update ready message to the specified topic.
 */
void publishUpdateReady(struct mqtt_client *client, const char *topic, int expectedChunks, const char *expectedSize);
#endif /* _CONNECTION_H_ */
