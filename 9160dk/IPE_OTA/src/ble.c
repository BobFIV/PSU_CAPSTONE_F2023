/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/conn_ctx.h>
#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>

#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/sys/byteorder.h>

#include <errno.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "aggregator.h"
#include "ble.h"

static struct bt_conn *default_conn;
static struct bt_nus_client default_nus_client;

K_SEM_DEFINE(nus_write_sem, 0, 1);


LOG_MODULE_DECLARE(lte_ble_gw);
//BLE one-byte headers for BLE communication used in receiver.
#define FIRMWARE_ERROR   0x01
#define FIRMWARE_ACK     0x02
#define FIRMWARE_VERSION 0x03
#define IMAGE            0x04
#define TRAIN_LOCATION   0x05

//#define BT_UUID_NUS_SERVICE BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00000001, 0x710e, 0x4a5b, 0x8d75, 0x3e5b444bc3cf))
#define BT_UUID_NUS_SERVICE BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x6E400001, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E))
/*
#define BT_UUID_ESP32_RX_VAL \
    BT_UUID_128_ENCODE(0x6E400002, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_ESP32_RX \
    BT_UUID_DECLARE_128(BT_UUID_ESP32_RX_VAL)

#define BT_UUID_ESP32_TX_VAL \
    BT_UUID_128_ENCODE(0x6E400003, 0xB5A3, 0xF393, 0xE0A9, 0xE50E24DCCA9E)
#define BT_UUID_ESP32_TX \
    BT_UUID_DECLARE_128(BT_UUID_ESP32_TX_VAL)
*/
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
/*
struct ble_device {   // device connected struct.
    struct bt_conn *conn;
    enum ble_destination destination;       // device name: i.e. Rasperry Pi, ESP32, etc.
    struct bt_nus_client *nus_client;        // NUS client instance for the device
};
*/

//static struct ble_device devices_list[MAX_CONNECTED_DEVICES];

struct bt_conn* get_conn_from_destination(enum ble_destination destination) {
    /*
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].destination == destination) {
            return devices_list[i].conn;
        }
    }
    return NULL; // No connection found for the given destination
    */
    return default_conn;
}
/*
enum ble_destination get_destination_from_conn(struct bt_conn *conn) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
		if (devices_list[i].conn == conn) {
			return devices_list[i].destination;
		}
	}
	return DESTINATION_UNKNOWN; // No destination found for the given connection
}
*/
//Function prototyping for the device name function.
char *get_device_name(struct bt_conn *conn);

/*
static void setup_connection(struct bt_conn *conn, struct bt_nus_client *nus) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (!devices_list[i].conn) {
            devices_list[i].conn = conn;
            devices_list[i].destination = DESTINATION_UNKNOWN;
			devices_list[i].nus_client = nus;

            LOG_INF("Connection in array initialized.");
            break;
        }
    }
}
*/
/*
static void remove_connection(struct bt_conn *conn) {
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].conn == conn) {
            devices_list[i].conn = NULL;
            devices_list[i].destination = DESTINATION_UNKNOWN; // Set to an "unknown" or "unset" value
            break;
        }
    }
}
*/
static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
					const uint8_t *const data, uint16_t len)
{
	LOG_INF("Data sent via NUS");
	ARG_UNUSED(nus);

	//free buffer after send?

	k_sem_give(&nus_write_sem);

	if (err) {
		LOG_WRN("ATT error code: 0x%02X", err);
	}

	
}


int ble_transmit(struct bt_conn *conn, uint8_t *data, size_t length) {
    LOG_INF("Transmit called...");

    if (!data || !length) {
        return -EINVAL;
    }
    if (!conn) {
        LOG_INF("No connection found for destination");
        return -EINVAL;
    }
    int err = bt_nus_client_send(&default_nus_client, data, length);
    if (err) {
        LOG_ERR("Failed to send data over BLE using NUS: %d", err);
        return err;
    }

    return 0;
}

static void discovery_completed(struct bt_gatt_dm *dm, void *context)
{
    struct bt_nus_client *nus = context;
    LOG_INF("Service discovery completed");
    bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	


    /*
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	struct bt_nus_client *nus = (struct bt_nus_client *)context;
	//setup_connection(conn, nus);  // Add the connection to the list and initialize the NUS client for this connection
    LOG_INF("Service discovery completed");

    // Print discovered attributes (for debugging)
    bt_gatt_dm_data_print(dm);

    // Assign handles using the NUS client library function
    if (bt_nus_handles_assign(dm, nus) < 0) {
        LOG_ERR("Failed to assign handles");
        goto release;
    }

    // Subscribe to the RX characteristic using the NUS client library function
    if (bt_nus_subscribe_receive(nus) < 0) {
        LOG_ERR("Failed to subscribe to RX characteristic");
        goto release;
    }

    
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (devices_list[i].conn == conn) {

            char *device_name = get_device_name(conn);
            if (device_name) {
                // Now you can use the device_name to determine the device type
                if (strncmp(device_name, "ESP32", 6) == 0) {
                    LOG_INF("Device is ESP32");
                    devices_list[i].destination = DESTINATION_ESP32;
                } else if (strncmp(device_name, "RaspberryPi-", 12) == 0) {
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
    */

//release:
    bt_gatt_dm_data_release(dm);
}


/*
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

*/

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_ERR("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_completed,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};


static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       &default_nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		LOG_INF("exchange_func succesfully executed.");
        uint16_t mtu = bt_gatt_get_mtu(conn);
        LOG_INF("Negotiated MTU size: %u", mtu);
	} else {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err) {
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_ERR("Failed to connect to %s (%d)", addr, conn_err);

        if (default_conn == conn) {
            bt_conn_unref(default_conn);
            default_conn = NULL;

            err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            if (err) {
                LOG_ERR("Scanning failed to start (err %d)", err);
            }
        }

        LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);
        return;
    }

    LOG_INF("Connected: %s", addr);
    LOG_INF("Called MTU exchange function");
    static struct bt_gatt_exchange_params exchange_params;
    exchange_params.func = exchange_func;

    LOG_INF("Called bt_gatt_exchange_mtu");
    LOG_INF("exchange_params.func address: 0x%08x", (uint32_t) exchange_params.func);
    LOG_INF("Connection pointer: 0x%08x", (uint32_t) conn);
    err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err) {
        LOG_WRN("MTU exchange failed (err %d)", err);
    }

    //k_sleep(K_MSEC(5000));  // Wait for 5 seconds before starting the discovery

    
    
    
    LOG_INF("Called Security function");
    err = bt_conn_set_security(conn, BT_SECURITY_L1);
    if (err) {
        LOG_WRN("Failed to set security: %d", err);
        gatt_discover(conn);
    }
    LOG_INF("Security Set. Starting Discovery...");
    
    LOG_INF("Calling bt_gatt_dm_start");

    err = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE, &discovery_cb, &default_nus_client);
    if (err) {
        LOG_ERR("Could not start service discovery, err %d", err);
    } else {
       // k_sleep(K_MSEC(100));
        gatt_discover(conn);
    }

   // k_sleep(K_MSEC(500));  // Another 500 milliseconds delay after discovery

    err = bt_scan_stop();
    if ((!err) && (err != -EALREADY)) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
    }
    default_conn = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s, Reason: %u", addr, reason);
    //remove_connection(conn);  // Remove the connection from the list

    if (default_conn != conn) {
        return;
    }

    bt_conn_unref(default_conn);
    default_conn = NULL;

    
    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
    }
    LOG_INF("Scan started again due to disconnect.");
}
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", addr,
			level, err);
	}

	gatt_discover(conn);
}



BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
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

static int scan_init(void) {
    int err = 0;

    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = 0x0100,
        .window = 0x0050,
    };

    struct bt_scan_init_param scan_init = {
        .connect_if_match = 1,
        .scan_param = &scan_param,
        .conn_param = BT_LE_CONN_PARAM_DEFAULT,
    };

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    // Add filter for ESP32 service
    //err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_ESP32_SERVICE);
    err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_NUS_SERVICE);
    if (err) {
        LOG_ERR("Scanning filters for ESP32 service cannot be set");
        return err;
    }

	// Add filter for Raspberry Pi Service service
	/*
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_rPI_SERVICE);
    if (err) {
        LOG_ERR("Scanning filters for rPI service cannot be set");
        return;
    }
	*/
    

    err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
    if (err) {
        LOG_ERR("Filters cannot be turned on (err %d)", err);
        return err;
    }

	return err;

}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}



static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", addr, reason);
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *data, uint16_t len)
{
    //Handles data received via Bluetooth.
    /*
    Data consists of:
    1. Firmware error messages from a device to respective topic in MQTT (/ESP32/Update or /RaspberryPi/Update)
    2. Firmware Update Ready (ACK) messages from a device to respective topic in MQTT (/ESP32/Update or /RaspberryPi/Update)
    3. Firmware versions from initial connection on each device. This will be package in oneM2M and sent as an UPDATE call to the ACME CSE.
    4. Images going from ESP32 to rPI
    5. Location of the train from the rPI to Django
    #define FIRMWARE_ERROR   0x01
    #define FIRMWARE_ACK     0x02
    #define FIRMWARE_VERSION 0x03
    #define IMAGE            0x04
    #define TRAIN_LOCATION   0x05
    */
    ARG_UNUSED(nus);

    LOG_INF("Data received via NUS...");
    if (len > 0) {
        LOG_HEXDUMP_INF(data, len, "Received data:");
        int err = 0;
        // Check the first byte to determine the data type
        uint8_t data_type = data[0];
        struct uplink_data_packet data_packet;
        data_packet.source = SOURCE_ESP32; // This is just a placeholder. 
        //#TO-DO The source will be determined by the device name 
        data_packet.length = MIN(len - 1, ENTRY_MAX_SIZE); // Subtract 1 to exclude the header
        memcpy(data_packet.data, data + 1, data_packet.length); // Start copying from data + 1
        switch (data_type) {
            case FIRMWARE_ERROR:
                data_packet.type = uplink_FIRMWARE_ERROR;
                err = uplink_aggregator_put(data_packet);
                if (err) {
                    LOG_ERR("Failed to put data into aggregator, err %d", err);
                }
                break;
            case FIRMWARE_ACK:
                data_packet.type = uplink_FIRMWARE_ACK;
                err = uplink_aggregator_put(data_packet);
                if (err) {
                    LOG_ERR("Failed to put data into aggregator, err %d", err);
                }
                break;
            case FIRMWARE_VERSION:
                data_packet.type = uplink_FIRMWARE_VERSION;
                err = uplink_aggregator_put(data_packet);
                if (err) {
                    LOG_ERR("Failed to put data into aggregator, err %d", err);
                }
                break;
            case IMAGE:
                //transfer_image_to_rpi(data + 1, len - 1);               //This must be resent over BLE to the rPI.
                break;
            case TRAIN_LOCATION:
                data_packet.type = uplink_TRAIN_LOCATION;
                err = uplink_aggregator_put(data_packet);
                if (err) {
                    LOG_ERR("Failed to put data into aggregator, err %d", err);
                }          
                break;
            default:
                LOG_ERR("Unknown data type received: %d", data_type);
                break;
        }
    } else {
        LOG_DBG("Notification with 0 length");
    }

    return BT_GATT_ITER_CONTINUE;
}


static int nus_client_init(void)
{
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}
	};

	err = bt_nus_client_init(&default_nus_client, &init);
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("Default NUS Client module initialized");
	return err;
}



int ble_init(void)
{
	int err;

	LOG_INF("Initializing Bluetooth..");

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		return err;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return err;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}


	err = scan_init();
	if (err != 0) {
		LOG_ERR("scan_init failed (err %d)", err);
		return err;
	}
	
	err = nus_client_init(); // NULL for default callbacks, or you can provide your own
	if (err) {
		LOG_ERR("Failed to initialize default NUS client (err %d)", err);
		return err;
	}
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start, err %d", err);
        return err;
    }

    LOG_INF("Scanning...");
	
    return err;
}
