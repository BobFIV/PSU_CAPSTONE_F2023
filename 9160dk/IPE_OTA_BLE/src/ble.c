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


#define NUS_WRITE_TIMEOUT K_MSEC(150)

//#define DEVICE_NAME_RPI "RaspberryPi"
//#define DEVICE_NAME_ESP32 "ESP32"
static char* ESP32_device_name = "ESP32";
static char* RaspberryPi_device_name = "RaspberryPi";
static char* target_device_name;
//static char *current_scanning_for = NULL;
static struct bt_conn *default_conn;
//static struct bt_nus_client default_nus_client;
struct k_work_delayable periodic_switch_work;
K_SEM_DEFINE(nus_write_sem, 0, 1);

LOG_MODULE_DECLARE(lte_ble_gw);

//BLE one-byte headers for BLE communication used in receiver.
#define FIRMWARE_ERROR   0x01
#define FIRMWARE_ACK     0x02
#define FIRMWARE_VERSION 0x03
#define IMAGE            0x04
#define TRAIN_LOCATION   0x05

#define MAX_CONNECTED_DEVICES 2
#define DEVICE_NAME_MAX_SIZE 30
#define MAX_TRANSMIT_RETRY 5
#define TRANSMIT_RETRY_DELAY_MS 100
struct device_name_id_map {
    char device_name[DEVICE_NAME_MAX_SIZE]; 
    int id;
};
struct device_name_id_map device_maps[10];

BT_CONN_CTX_DEF(ctx_lib, MAX_CONNECTED_DEVICES, sizeof(struct bt_nus_client));


static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *data, uint16_t len);
/*
This function switches the device name that the BLE module is scanning for. Initially set to ESP32, it will swap over to the RaspberryPi after 10 seconds.
This uses the nordic work function to schedule a work item to be executed after a delay. It will stop scanning, change and update the filter, then begin
scanning.
Potential To-Do: Add a check to see if the device is already connected. If so, do not switch to that device (I.E. ESP32 connected, do not switch to ESP32).
*/
static void switch_device_name(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);

    LOG_INF("Switching device name...");
    const char *new_name;
    if (strcmp(target_device_name, RaspberryPi_device_name) == 0) {
        new_name = ESP32_device_name;
    } else {
        new_name = RaspberryPi_device_name;
    }

    strncpy(target_device_name, new_name, DEVICE_NAME_MAX_SIZE - 1);
    target_device_name[DEVICE_NAME_MAX_SIZE - 1] = '\0'; // Ensure null termination

    bt_scan_stop();
    bt_scan_filter_remove_all();

    int err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, target_device_name);
    if (err) {
        LOG_ERR("Scanning filters for %s service cannot be set", target_device_name);
        k_work_reschedule(dwork, K_MSEC(10000));
        return;
    }

    LOG_INF("FILTERS ADDED");
    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start after switching, err %d", err);
        k_work_reschedule(dwork, K_MSEC(10000));
        return;
    }

    LOG_INF("Now scanning for: %s", target_device_name);
    k_work_reschedule(dwork, K_MSEC(10000));
}

/*
Data Sent function. This is called when data is sent via BLE. It will release the semaphore that is used to block the thread until the data is sent.
*/
void ble_data_sent(struct bt_nus_client *nus, uint8_t err, const uint8_t *data, uint16_t len) 
{

	k_sem_give(&nus_write_sem);

	if (err) {
		LOG_WRN("ATT error code: 0x%02X", err);
	}
}

/*
BLE transmit function. Called from MQTT-BLE pipe.c, this function will send data via BLE to the target device.
*/
int ble_transmit(const char* target, uint8_t *data, uint16_t length) {
    int err = 0;
    err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
    if (err)
    {
    LOG_WRN("NUS send timeout");
    }
    if (!data || !length) {
        return -EINVAL;
    }
    int index = -1;
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        if (strcmp(device_maps[i].device_name, target) == 0) {
            index = device_maps[i].id;
            break;
        }
    }
    if (index == -1) {
        LOG_INF("No device found for destination: %s", target);
        return -EINVAL;
    }
    LOG_INF("Calling bt_conn_ctx_get_by_id");
    const struct bt_conn_ctx *ctx = bt_conn_ctx_get_by_id(&ctx_lib_ctx_lib, index);
    LOG_INF("bt_conn_ctx_get_by_id returned");
    if (ctx) {   
        LOG_INF("Connection context found for destination");
        
        struct bt_nus_client *nus = ctx->data;
        LOG_INF("getting NUS CLIENT");
        if (nus != NULL) {
            err = bt_nus_client_send(nus, data, length);
            if (err){
                LOG_ERR("Unable to send data, err %d", err);
            } else {
                LOG_INF("Data sent successfully to %s", target);
            }
                bt_conn_ctx_release(&ctx_lib_ctx_lib, (void *)ctx->data); 
        } 

     }
    return err;
}

/*
Discovery Completed function. This is called when the BLE module has completed discovery of the target device. It will assign the handles for the NUS service.
It also maps the device to the table to get the device ID in the nus context library using the name.
*/
static void discovery_completed(struct bt_gatt_dm *dm, void *context)
{
    LOG_INF("Service discovery completed");
    struct bt_nus_client *nus = context; //bt_conn_ctx_alloc(&ctx_lib_ctx_lib, context);
        if (!nus) {
        LOG_ERR("No free memory to allocate the connection context");
        return;
    }
    LOG_INF("set nus_client. Calling bt_gatt_dm_data_print");
    bt_gatt_dm_data_print(dm);
    LOG_INF("calling bt_nus_handles_assign");
	bt_nus_handles_assign(dm, nus);
    LOG_INF("calling bt_nus_subscribe_receive");
	bt_nus_subscribe_receive(nus);

    LOG_INF("calling bt_gatt_dm_data_release");
    bt_gatt_dm_data_release(dm);
    LOG_INF("Grabbing the number of connections");

    int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
	} else {
		LOG_INF("Scanning started");
	}

    size_t num_nus_conns = bt_conn_ctx_count(&ctx_lib_ctx_lib);
    LOG_INF("Calculating the index for this connection.");
    size_t nus_index = 99;
    // This is weird logic. The microcontroller knows which index the device is connected to, but we dont.
    // So we have to iterate through the context library to find the device that matches the nus client we just discovered.
    // Then, we map that device to the table so we can pick the device using the name when we send data later.
    // We compare the target device name to the device we are scanning for, and set it to that in our table.  (if we switch during this function we're in big trouble)
    for (size_t i = 0; i < num_nus_conns; i++) {
		const struct bt_conn_ctx *ctx =
				bt_conn_ctx_get_by_id(&ctx_lib_ctx_lib, i);
		
		if (ctx) {
			if (ctx->data == nus) {
				nus_index = i;
                const char *device_name = (strcmp(target_device_name, "RaspberryPi") == 0) ? "RaspberryPi" : "ESP32";
                strncpy(device_maps[nus_index].device_name, device_name, sizeof(device_maps[nus_index].device_name));
                device_maps[nus_index].id = nus_index;
                
                err = bt_nus_client_send(nus, (uint8_t *) "ping!", 5);
                if (err) {
                    LOG_ERR("Unable to send data, err %d", err);
                } else {
                    LOG_INF("Data sent successfully");
                }
                bt_conn_ctx_release(&ctx_lib_ctx_lib, (void *) ctx->data);
				break;
			}
		} else {
            LOG_ERR("No context found for connection");
            bt_conn_ctx_release(&ctx_lib_ctx_lib, (void *) ctx->data);
        }
	}
    /*
    LOG_INF("Current device map is:");
    for (size_t i = 0; i < num_nus_conns; i++) {
    LOG_INF("Device map [%d]: Name = %s, ID = %d", i, device_maps[i].device_name, device_maps[i].id);
    }
    */
    
    LOG_INF("Donezo.");

}


static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("Service not found");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
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
    LOG_INF("gatt_discover called");
	struct bt_nus_client *nus_client = bt_conn_ctx_get(&ctx_lib_ctx_lib, conn);
    if (!nus_client) {
        LOG_ERR("No NUS client found for connection");
        return;
    }
    LOG_INF("set nus_client");
    LOG_INF("calling bt_gatt_dm_start");
    
	err = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE, &discovery_cb, nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
    
    bt_conn_ctx_release(&ctx_lib_ctx_lib, (void *) nus_client);
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

        // Initialize NUS client
    struct bt_nus_client_init_param init = {
        .cb = {
            .received = ble_data_received,
            .sent = ble_data_sent,
        }
    };

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

    struct bt_nus_client *nus_client = bt_conn_ctx_alloc(&ctx_lib_ctx_lib, conn);

    if (!nus_client) {
        LOG_WRN("No free memory to allocate the connection context");
        return;
    }

    memset(nus_client, 0, bt_conn_ctx_block_size_get(&ctx_lib_ctx_lib));

    err = bt_nus_client_init(nus_client, &init);

    bt_conn_ctx_release(&ctx_lib_ctx_lib, nus_client);

    
    if (err) {
        LOG_ERR("NUS Client initialization failed (err %d)", err);
    } else {
        LOG_INF("NUS Client module initialized");
    }

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
    
    
    LOG_INF("Called Security function");
    err = bt_conn_set_security(conn, BT_SECURITY_L1);
    if (err) {
        LOG_WRN("Failed to set security: %d", err);
        gatt_discover(conn);
    }

   LOG_INF("Discovery started");
    gatt_discover(conn);
    /*Stop scanning during the discovery*/
	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
    //default_conn = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s, Reason: %u", addr, reason);
    //remove_connection(conn);  // Remove the connection from the list
    //To-DO: Remove from table.
    err = bt_conn_ctx_free(&ctx_lib_ctx_lib, conn);
    if (err) {
		LOG_WRN("The memory was not allocated for the context of this connection.");
	}

    bt_conn_unref(conn);
    default_conn = NULL;


    //bt_conn_unref(default_conn);
    //default_conn = NULL;

    /*
    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
    }
    LOG_INF("Scan started again due to disconnect.");
    */
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

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, scan_connecting);

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

    target_device_name  = k_malloc(30);
    if (!target_device_name) {
        LOG_ERR("Failed to allocate memory for target_device_name");
        return -ENOMEM; // or another appropriate error code
    } 
    char * default_target_name = "ESP32";
    strncpy(target_device_name, default_target_name, 30); //initailize to the ESP32.

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, target_device_name);
    if (err) {
        LOG_ERR("Scanning filters for %s service cannot be set", target_device_name);
        return err;
    }

    err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, true);
    if (err) {
        LOG_ERR("Filters cannot be turned on (err %d)", err);
        return err;
    }
    LOG_INF("Filters set, scheduling work...");
    k_work_init_delayable(&periodic_switch_work, switch_device_name);
    k_work_schedule(&periodic_switch_work, K_MSEC(10000)); // Switch scan target every 10 seconds if nothing found.
    

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
                struct downlink_data_packet dl_data_packet;
                dl_data_packet.type = downlink_IMAGE;
                dl_data_packet.destination = DESTINATION_RaspberryPi;
                dl_data_packet.length = MIN(len - 1, ENTRY_MAX_SIZE); // Subtract 1 to exclude the header
                memcpy(dl_data_packet.data, data + 1, dl_data_packet.length); // Start copying from data + 1
                err = downlink_aggregator_put(dl_data_packet);
                if (err) {
                    LOG_ERR("Failed to put data into aggregator, err %d", err);
                }
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

int ble_init(void)
{
	int err;

	LOG_INF("Initializing Bluetooth..");
    
    for (int i = 0; i < MAX_CONNECTED_DEVICES; i++) {
        device_maps[i].id = -1;
        device_maps[i].device_name[0] = '\0';
    }

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

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start, err %d", err);
        return err;
    }
    
    LOG_INF("Scanning...");
	
    return err;
}
