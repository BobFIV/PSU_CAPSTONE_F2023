/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef _BLE_H_
#define _BLE_H_

#include "aggregator.h"  

int ble_init(void);

#define BT_UUID_ESP32_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_ESP32_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_ESP32_SERVICE_VAL)


struct bt_conn* get_conn_from_destination(enum ble_destination destination);

int ble_transmit(const char* target, uint8_t *data, uint16_t length);


#endif /* _BLE_H_ */
