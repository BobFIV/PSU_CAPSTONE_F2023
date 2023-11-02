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
#include <bluetooth/conn_ctx.h>


#include <dk_buttons_and_leds.h>
#include <zephyr/sys/byteorder.h>

#include <stdio.h>
#include <zephyr/logging/log.h>
#include "aggregator.h"


LOG_MODULE_DECLARE(lte_ble_gw);

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

/*
#define BT_UUID_rPI_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_rPI_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_rPI__SERVICE_VAL)

#define BT_UUID_rPI_RX_VAL \
    BT_UUID_128_ENCODE(0x6E400002, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_rPI_RX \
    BT_UUID_DECLARE_128(BT_UUID_rPI__RX_VAL)

#define BT_UUID_rPI_TX_VAL \
    BT_UUID_128_ENCODE(0x6E400003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_rPI_TX \
    BT_UUID_DECLARE_128(BT_UUID_rPI__TX_VAL)
*/

#define MAX_CONNECTED_DEVICES 5

#define BT_NAME_MAX_LEN 248	//super overkill but this is the default, I suppose?

struct ble_device {	//device connected struct.
    struct bt_conn *conn;
    enum ble_destination destination;		//device name: i.e. Rasperry Pi, ESP32, etc.
	uint16_t rx_handle;
    uint16_t tx_handle;
};

static struct ble_device devices_list[MAX_CONNECTED_DEVICES];

struct bt_conn* get_conn_from_destination(enum ble_destination destination) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].destination == destination) {
            return devices_list[i].conn;
        }
    }
    return NULL; // No connection found for the given destination
}

enum ble_destination get_destination_from_conn(struct bt_conn *conn) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
		if (devices_list[i].conn == conn) {
			return devices_list[i].destination;
		}
	}
	return DESTINATION_UNKNOWN; // No destination found for the given connection
}

//Function prototyping for the device name function.
char *get_device_name(struct bt_conn *conn);


static void add_connection(struct bt_conn *conn) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (!devices_list[i].conn) {
            devices_list[i].conn = conn;
            devices_list[i].destination = DESTINATION_UNKNOWN;
			devices_list[i].rx_handle = 0;
			devices_list[i].tx_handle = 0; //populate destination, rx and tx after disovery.
			LOG_INF("Connection in array initialized.");
            break;
        }
    }
	
}

static void remove_connection(struct bt_conn *conn) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].conn == conn) {
            devices_list[i].conn = NULL;
            devices_list[i].destination = DESTINATION_UNKNOWN; // Set to an "unknown" or "unset" value
            break;
        }
    }
}

static uint8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	LOG_INF("Something received...");
	if (length > 0) {
		// Log the received data as a hex string
		LOG_HEXDUMP_INF(data, length, "Received data:");
		struct uplink_data_packet data_packet;
		//To-Do: Smart IPE to implement data logic. This will determine packet type.
		/*
		if (something)
			packet.type = FIRMWARE_UPDATE;
		else if (something else)
			packet.type = IMAGE_DATA;
		else
			packet.type = TEXT;
		//To-Do: Smart IPE to implement data logic. This will determine packet source.
		if (something)
			packet.source = SOURCE_ESP32;
		else
			packet.source = SOURCE_RaspberryPi;
		*/
		//For now:
		LOG_INF("Received data from ESP32");
		data_packet.type = uplink_TEXT;
		data_packet.source = SOURCE_ESP32;
		//setting length and copying data.
		data_packet.length = MIN(length, ENTRY_MAX_SIZE);
		memcpy(data_packet.data, data, data_packet.length);

		//Add the packet to the aggregator.
		int err = uplink_aggregator_put(data_packet);
		if (err) {
			LOG_ERR("Failed to put data into aggregator, err %d", err);
		}

	} else {
		LOG_DBG("Notification with 0 length");
	}
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t on_transmitted(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	LOG_INF("Something Transmitted...");
	if (!data) {
		LOG_INF("Unsubscribed from TX notifications");
		return 0;
	}

	if (length == 0) {
		LOG_WRN("Received empty TX notification");
		return 0;
	}

	// Log the transmitted data as a string (assuming it's a null-terminated string)
	LOG_INF("Transmitted data (string): %.*s", length, (char *)data);

	// Log the transmitted data in hexadecimal format
	LOG_HEXDUMP_INF(data, length, "Transmitted data (hex):");

	return 0;
}



int ble_transmit(struct bt_conn *conn, uint8_t *data, size_t length) {
	LOG_INF("Transmit called...");

	if (!data || !length)
	{
		return -EINVAL;
	}
	if (!conn) {
		LOG_INF("No connection found for destination");
        return -EINVAL;
    }
    struct ble_device *device = NULL;
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].conn == conn) {
            device = &devices_list[i];
            break;
        }
    }

    if (!device) {
        LOG_ERR("Device not found for connection");
        return -ENODEV;
    }

	uint16_t handle_to_use = device->tx_handle;		//The 9160dk is the Central. Thus it sends to the TX handle.

    int err = bt_gatt_write_without_response(conn, handle_to_use, data, length, false);
    if (err) {
        LOG_ERR("Failed to send data over BLE: %d", err);
        return err;
    }

    return 0;
}

static void discovery_completed(struct bt_gatt_dm *disc, void *ctx)
{

    int err;
	LOG_INF("discover completed called");
	
    struct bt_conn_info info;
    err = bt_conn_get_info(bt_gatt_dm_conn_get(disc), &info);
    if (err) {
        LOG_ERR("Unable to get connection info (err %d)", err);
    } else {
        LOG_INF("Connection role: %u", info.role);
        LOG_INF("Connection type: %u", info.type);
        LOG_INF("Connection status: %u", info.state);
    }
	
    static struct bt_gatt_subscribe_params param_tx = {	
		.notify = on_transmitted,
        .value = BT_GATT_CCC_NOTIFY,

    };

    static struct bt_gatt_subscribe_params param_rx = {	
		.notify = on_received,
        .value = BT_GATT_CCC_NOTIFY,
    };

    const struct bt_gatt_dm_attr *chrc;
    const struct bt_gatt_dm_attr *desc;
	const struct bt_gatt_dm_attr *chrc_value;

    // For RX characteristic
	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_ESP32_RX);
	if (!chrc) {
		LOG_ERR("Missing ESP32 RX characteristic");
		goto release;
	}
	param_rx.value_handle = chrc->handle;
	LOG_INF("ESP32 RX handle: %d", param_rx.value_handle);

	

	chrc_value = bt_gatt_dm_attr_next(disc, chrc); // Get the characteristic value attribute
	if (!chrc_value) {
		LOG_ERR("Missing ESP32 RX value");
		goto release;
	}

	char uuid_str[37]; // 36 bytes for UUID and 1 byte for '\0'
	bt_uuid_to_str(chrc_value->uuid, uuid_str, sizeof(uuid_str));
	LOG_INF("ESP32 RX UUID: %s", uuid_str);


	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_ERR("Missing RX CCC descriptor");
		goto release;
	}
	param_rx.ccc_handle = desc->handle;
	

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param_rx);
	if (err) {
    LOG_ERR("Subscribe RX failed (err %d)", err);
	}
	/* This fails for some reason. 
	else {
	k_sleep(K_MSEC(1000));  // Wait for 1 second
    bool is_subscribed = bt_gatt_is_subscribed(bt_gatt_dm_conn_get(disc), param_rx.value_handle, BT_GATT_CCC_NOTIFY);
    LOG_INF("RX Characteristic is %ssubscribed for notifications", is_subscribed ? "" : "not ");
	} 
	*/
	
	

	// For TX characteristic
	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_ESP32_TX);
	if (!chrc) {
		LOG_ERR("Missing ESP32 TX characteristic");
		goto release;
	}
	param_tx.value_handle = chrc->handle;
	LOG_INF("ESP32 TX handle: %d", param_tx.value_handle);

	chrc_value = bt_gatt_dm_attr_next(disc, chrc); // Get the characteristic value attribute
	if (!chrc_value) {
		LOG_ERR("Missing ESP32 TX value");
		goto release;
	}

	bt_uuid_to_str(chrc_value->uuid, uuid_str, sizeof(uuid_str));
	LOG_INF("ESP32 TX UUID: %s", uuid_str);


	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_ERR("Missing TX CCC descriptor");
		goto release;
	}
	param_tx.ccc_handle = desc->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param_tx);
	if (err) {
		LOG_ERR("Subscribe TX failed (err %d)", err);
	}

    struct bt_conn *conn = bt_gatt_dm_conn_get(disc);
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].conn == conn) {
            devices_list[i].rx_handle = param_rx.value_handle;
            devices_list[i].tx_handle = param_tx.value_handle;

			char *device_name = get_device_name(conn);
			if (device_name) {
				// Now you can use the device_name to determine the device type
				if (strncmp(device_name, "ESP32", 6) == 0) {
					LOG_INF("Device is ESP32");
					devices_list[i].destination = DESTINATION_ESP32;
				} else if (strncmp(device_name, "RaspberryPi-", 12) == 0) {			//12 chars for RaspberryPi- then add a number.
					LOG_INF("Device is Raspberry Pi");
					devices_list[i].destination = DESTINATION_RaspberryPi;
				} else {
					LOG_INF("Device is Unknown. Setting it to ESP32 for now.");
					devices_list[i].destination = DESTINATION_ESP32;

				}
				LOG_INF("Device name: %s", device_name);
				LOG_HEXDUMP_INF(device_name, 6, "Device name:");
			}

            break;
        }
    }
/* This check always makes the code restart as soon as a connection is added.
	bool is_subscribed_rx = bt_gatt_is_subscribed(conn, param_rx.value_handle, BT_GATT_CCC_NOTIFY);
	LOG_INF("RX Characteristic is %ssubscribed for notifications", is_subscribed_rx ? "" : "not ");

	bool is_subscribed_tx = bt_gatt_is_subscribed(conn, param_tx.value_handle, BT_GATT_CCC_NOTIFY);
	LOG_INF("TX Characteristic is %ssubscribed for notifications", is_subscribed_tx ? "" : "not ");
*/
release:
    err = bt_gatt_dm_data_release(disc);
    if (err) {
        LOG_ERR("Could not release discovery data, err: %d", err);
    }
}

char *get_device_name(struct bt_conn *conn) {
    static char name[BT_NAME_MAX_LEN + 1]; // +1 for null-terminator
    struct bt_conn_info info;

    if (!conn) {
        return NULL;
    }

    if (bt_conn_get_info(conn, &info) < 0) {
        return NULL;
    }

    // Assuming you're dealing with LE connections
    if (info.type == BT_CONN_TYPE_LE) {
        strncpy(name, info.le.dst->a.val, BT_NAME_MAX_LEN);
        name[BT_NAME_MAX_LEN] = '\0'; // Ensure null-termination
        return name;
    }

    return NULL;
}




static void discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_ERR("The discovery procedure failed, err %d", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed,
	.error_found = discovery_error_found,
};

static void connected(struct bt_conn *conn, uint8_t conn_err) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);
        return;
    }

    LOG_INF("Connected: %s", addr);
    add_connection(conn);  // Add the connection to the list

    int err = bt_gatt_dm_start(conn, BT_UUID_ESP32_SERVICE, &discovery_cb, NULL);
    if (err) {
        LOG_ERR("Could not start service discovery, err %d", err);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s, Reason: %u", addr, reason);
    remove_connection(conn);  // Remove the connection from the list
}


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
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
