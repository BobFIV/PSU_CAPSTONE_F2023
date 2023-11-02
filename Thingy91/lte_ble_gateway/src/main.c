/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/socket.h>


#include <string.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <zephyr/console/console.h>
#include <dk_buttons_and_leds.h>
#include <modem/lte_lc.h>
#include <zephyr/sys/reboot.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>


#include "mqtt_connection.h"
#include "ble.h"
#include "mqtt_ble_pipe.h"


static K_SEM_DEFINE(lte_connected, 0, 1);
/* The mqtt client struct */
struct mqtt_client client;
/* File descriptor */
struct pollfd fds;

LOG_MODULE_REGISTER(lte_ble_gw, CONFIG_LTE_BLE_GW_LOG_LEVEL);

/* Interval in milliseconds between each time status LEDs are updated. */
#define LEDS_UPDATE_INTERVAL K_MSEC(500)

/* Interval in microseconds between each time LEDs are updated when indicating
 * that an error has occurred.
 */
#define LEDS_ERROR_UPDATE_INTERVAL 250000

#define BUTTON_1 BIT(0)
#define BUTTON_2 BIT(1)
#define SWITCH_1 BIT(2)
#define SWITCH_2 BIT(3)

#define LED_ON(x)			(x)
#define LED_BLINK(x)		((x) << 8)
#define LED_GET_ON(x)		((x) & 0xFF)
#define LED_GET_BLINK(x)	(((x) >> 8) & 0xFF)

/* Interval in milliseconds after the device will retry cloud connection
 * if the event NRF_CLOUD_EVT_TRANSPORT_CONNECTED is not received.
 */
#define RETRY_CONNECT_WAIT K_MSEC(90000)

//To do: Update LEDs.
enum {
	LEDS_INITIALIZING       = LED_ON(0),
	LEDS_LTE_CONNECTING     = LED_BLINK(DK_LED3_MSK),
	LEDS_LTE_CONNECTED      = LED_ON(DK_LED3_MSK),
	LEDS_CLOUD_CONNECTING   = LED_BLINK(DK_LED4_MSK),
	LEDS_CLOUD_PAIRING_WAIT = LED_BLINK(DK_LED3_MSK | DK_LED4_MSK),
	LEDS_CLOUD_CONNECTED    = LED_ON(DK_LED4_MSK),
	LEDS_ERROR              = LED_ON(DK_ALL_LEDS_MSK)
} display_state;




/* Structures for work */
struct k_work_delayable leds_update_work;
struct k_work_delayable periodic_transmit_work;
struct k_work_delayable periodic_publish_work;




enum error_type {
	ERROR_NRF_CLOUD,
	ERROR_MODEM_RECOVERABLE,
};

/* Forward declaration of functions. */
static void work_init(void);

static void lte_handler(const struct lte_lc_evt *const evt)
{
     switch (evt->type) {
     case LTE_LC_EVT_NW_REG_STATUS:
        if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
            (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
            break;
        }
		LOG_INF("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
				"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
        break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
				"Connected" : "Idle");
		break;
     default:
             break;
     }
}


/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err)
{
	if (err_type == ERROR_NRF_CLOUD) {
		int err;

		/* Turn off and shutdown modem */
		k_sched_lock();
		err = lte_lc_power_off();
		__ASSERT(err == 0, "lte_lc_power_off failed: %d", err);
		nrf_modem_lib_shutdown();
	}

#if !defined(CONFIG_DEBUG)
	sys_reboot(SYS_REBOOT_COLD);
#else
	uint8_t led_pattern;

	switch (err_type) {
	case ERROR_NRF_CLOUD:
		/* Blinking all LEDs ON/OFF in pairs (1 and 4, 2 and 3)
		 * f there is an application error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED4_MSK;
		LOG_ERR("Error of type ERROR_NRF_CLOUD: %d", err);
		break;
	case ERROR_MODEM_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED3_MSK;
		LOG_ERR("Error of type ERROR_MODEM_RECOVERABLE: %d", err);
		break;
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED2_MSK;
		break;
	}

	while (true) {
		dk_set_leds(led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
		dk_set_leds(~led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
	}
#endif /* CONFIG_DEBUG */
}

/**@brief Modem fault handler. */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	error_handler(ERROR_MODEM_RECOVERABLE, (int)fault_info->reason);
}

/**@brief Update LEDs state. */
static void leds_update(struct k_work *work)
{
	static bool led_on;
	static uint8_t current_led_on_mask;
	uint8_t led_on_mask = current_led_on_mask;

	ARG_UNUSED(work);

	/* Reset LED3 and LED4. */
	led_on_mask &= ~(DK_LED3_MSK | DK_LED4_MSK);

	/* Set LED3 and LED4 to match current state. */
	led_on_mask |= LED_GET_ON(display_state);

	led_on = !led_on;
	if (led_on) {
		led_on_mask |= LED_GET_BLINK(display_state);
	} else {
		led_on_mask &= ~LED_GET_BLINK(display_state);
	}

	if (led_on_mask != current_led_on_mask) {
		dk_set_leds(led_on_mask);
		current_led_on_mask = led_on_mask;
	}

	k_work_schedule(&leds_update_work, LEDS_UPDATE_INTERVAL);
}


/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(uint32_t buttons, uint32_t has_changed)
{
	switch (has_changed) {
	case DK_BTN1_MSK:
		/* STEP 7.2 - When button 1 is pressed, call data_publish() to publish a message */
		if (buttons & DK_BTN1_MSK){
			int err = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
				   CONFIG_BUTTON_EVENT_PUBLISH_MSG, sizeof(CONFIG_BUTTON_EVENT_PUBLISH_MSG)-1);
			if (err) {
				LOG_INF("Failed to send message, %d", err);
				return;
			}
		}
		break;
	}
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_work_init_delayable(&leds_update_work, leds_update);
	k_work_schedule(&leds_update_work, LEDS_UPDATE_INTERVAL);
	k_work_init_delayable(&periodic_transmit_work, transmit_aggregated_data);
	k_work_schedule(&periodic_transmit_work, K_MSEC(10000));
	k_work_init_delayable(&periodic_publish_work, publish_aggregated_data);
	k_work_schedule(&periodic_publish_work, K_MSEC(10000));
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */

static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return err;
	}

	LOG_INF("Connecting to LTE network");

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_INF("Modem could not be configured, error: %d", err);
		return err;
	}

	k_sem_take(&lte_connected, K_FOREVER);
	LOG_INF("Connected to LTE network");
	display_state = LEDS_LTE_CONNECTED;

	return 0;
}

int main(void)
{
	int err;
	uint32_t connect_attempt = 0;
	LOG_INF("LTE Sensor Gateway sample started");

	
	
	

	if (dk_leds_init() != 0) {
		LOG_ERR("Failed to initialize the LED library");
	}

	err = modem_configure();
	if (err) {
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

	if (dk_buttons_init(button_handler) != 0) {
		LOG_ERR("Failed to initialize the buttons library");
	}

	err = client_init(&client);
	if (err) {
		LOG_ERR("Failed to initialize MQTT client: %d", err);
		return 0;
	}
	work_init();		//Currently just using it to blink some LEDs.
	ble_init();		    //Initializes bluetooth (ble.c)
do_connect:
	if (connect_attempt++ > 0) {
		LOG_INF("Reconnecting in %d seconds...",
			CONFIG_MQTT_RECONNECT_DELAY_S);
		k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
	}
	err = mqtt_connect(&client);
	if (err) {
		LOG_ERR("Error in mqtt_connect: %d", err);
		goto do_connect;
	}

	err = fds_init(&client,&fds);
	if (err) {
		LOG_ERR("Error in fds_init: %d", err);
		return 0;
	}

	while (1) {
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if (err < 0) {
			LOG_ERR("Error in poll(): %d", errno);
			break;
		}

		err = mqtt_live(&client);
		if ((err != 0) && (err != -EAGAIN)) {
			LOG_ERR("Error in mqtt_live: %d", err);
			break;
		}

		if ((fds.revents & POLLIN) == POLLIN) {
			err = mqtt_input(&client);
			if (err != 0) {
				LOG_ERR("Error in mqtt_input: %d", err);
				break;
			}
		}

		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
			break;
		}

		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("POLLNVAL");
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client");

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("Could not disconnect MQTT client: %d", err);
	}
	goto do_connect;

	/* This is never reached */
	return 0;
}
