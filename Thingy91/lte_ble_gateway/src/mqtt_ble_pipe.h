#ifndef _MQTT_BLE_PIPE_H_
#define _MQTT_BLE_PIPE_H_

void publish_aggregated_data(struct k_work *work);

void transmit_aggregated_data(struct k_work *work);

size_t serialize_downlink_data_packet(const struct downlink_data_packet *packet, uint8_t *buffer, size_t buffer_size);

#endif /* _MQTT_BLE_PIPE_H_ */