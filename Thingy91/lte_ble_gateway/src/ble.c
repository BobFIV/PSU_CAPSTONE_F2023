/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/sys/byteorder.h>


#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>
#include "aggregator.h"

LOG_MODULE_DECLARE(lte_ble_gw);

/* Thinghy advertisement UUID */
/*
#define BT_UUID_THINGY_VAL \
	BT_UUID_128_ENCODE(0xef680100, 0x9b35, 0x4933, 0x9b10, 0x52ffa9740042)

#define BT_UUID_THINGY \
	BT_UUID_DECLARE_128(BT_UUID_THINGY_VAL)
	*/
/* Thingy service UUID */
/*
#define BT_UUID_TMS_VAL \
	BT_UUID_128_ENCODE(0xef680400, 0x9b35, 0x4933, 0x9b10, 0x52ffa9740042)

#define BT_UUID_TMS \
	BT_UUID_DECLARE_128(BT_UUID_TMS_VAL) */
		/* Thingy characteristic UUID */
/*
#define BT_UUID_TOC_VAL \
	BT_UUID_128_ENCODE(0xef680403, 0x9b35, 0x4933, 0x9b10, 0x52ffa9740042)

#define BT_UUID_TOC \
	BT_UUID_DECLARE_128(BT_UUID_TOC_VAL) */
//#define BT_UUID_GATT_CCC BT_UUID_DECLARE_16(0x2902)

#define BT_UUID_ESP32_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_ESP32_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_ESP32_SERVICE_VAL)

#define BT_UUID_ESP32_RX_VAL \
    BT_UUID_128_ENCODE(0x6E400002, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_ESP32_RX \
    BT_UUID_DECLARE_128(BT_UUID_ESP32_RX_VAL)

#define BT_UUID_ESP32_TX_VAL \
    BT_UUID_128_ENCODE(0x6E400003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_ESP32_TX \
    BT_UUID_DECLARE_128(BT_UUID_ESP32_TX_VAL)


extern void alarm(void);



static uint8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{

	if (length > 0) {
		// Log the received data as a hex string
		LOG_HEXDUMP_INF(data, length, "Received data:");

	} else {
		LOG_DBG("Notification with 0 length");
	}
	return BT_GATT_ITER_CONTINUE;
}

static void on_transmitted(const struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		LOG_INF("Unsubscribed from TX notifications");
		return;
	}

	// Handle the transmitted data here
	LOG_INF("Transmitted data: %s", (char *)data);

}


static void discovery_completed(struct bt_gatt_dm *disc, void *ctx)
{
    int err;

    static struct bt_gatt_subscribe_params param_rx = {
        .notify = on_received,
        .value = BT_GATT_CCC_NOTIFY,
    };

    static struct bt_gatt_subscribe_params param_tx = {
        .notify = on_transmitted,
        .value = BT_GATT_CCC_NOTIFY,
    };

    const struct bt_gatt_dm_attr *chrc;
    const struct bt_gatt_dm_attr *desc;

    // Discover RX characteristic
    chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_ESP32_RX);
    if (!chrc) {
        LOG_ERR("Missing ESP32 RX characteristic");
        goto release;
    }
    param_rx.value_handle = chrc->handle;

    // Discover TX characteristic
    chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_ESP32_TX);
    if (!chrc) {
        LOG_ERR("Missing ESP32 TX characteristic");
        goto release;
    }
    param_tx.value_handle = chrc->handle;

    desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
    if (!desc) {
        LOG_ERR("Missing CCC descriptor");
        goto release;
    }

    param_rx.ccc_handle = desc->handle;
    err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param_rx);
    if (err) {
        LOG_ERR("Subscribe RX failed (err %d)", err);
    }

    param_tx.ccc_handle = desc->handle;
    err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param_tx);
    if (err) {
        LOG_ERR("Subscribe TX failed (err %d)", err);
    }

release:
    err = bt_gatt_dm_data_release(disc);
    if (err) {
        LOG_ERR("Could not release discovery data, err: %d", err);
    }
}


static void discovery_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_ERR("Thingy orientation service not found!");
}

static void discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_ERR("The discovery procedure failed, err %d", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error_found,
};

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_INF("Connected: %s", addr);

	err = bt_gatt_dm_start(conn, BT_UUID_ESP32_SERVICE, &discovery_cb, NULL);
	if (err) {
		LOG_ERR("Could not start service discovery, err %d", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
};

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Device found: %s", addr);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_ERR("Connection to peer failed!");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);

static void scan_start(void)
{
	int err;

	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_ESP32_SERVICE);
	if (err) {
		LOG_ERR("Scanning filters cannot be set");
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on");
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start, err %d", err);
	}

	LOG_INF("Scanning...");
}

static void ble_ready(int err)
{
	LOG_INF("Bluetooth ready");

	bt_conn_cb_register(&conn_callbacks);
	scan_start();
}

void ble_init(void)
{
	int err;

	LOG_INF("Initializing Bluetooth..");
	err = bt_enable(ble_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
}
