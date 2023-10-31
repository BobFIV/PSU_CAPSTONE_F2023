/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef _BLE_H_
#define _BLE_H_

#include "aggregator.h"  

void ble_init(void);




struct bt_conn* get_conn_from_destination(enum ble_destination destination);

void ble_transmit(struct bt_conn* conn, uint8_t* data, size_t length);




struct connected_device {
    struct bt_conn *conn;
    enum ble_destination type;
};


#endif /* _BLE_H_ */
