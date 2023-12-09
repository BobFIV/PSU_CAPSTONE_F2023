/*
*	Revision: Release Version 1.0
*	Last Edited: 11/15/2023
*	Author: Yuh Kurose, based on many different samples and examples from Nordic Semiconductor ASA.
*	Sponsor: Exacta Global Smart Solutions
*	See the README for more information.
 */
/*
	main.c is used for initialization of the device.
*/
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/socket.h>


#include <string.h>
#include <zephyr/console/console.h>
#include <dk_buttons_and_leds.h>
#include <modem/lte_lc.h>
#include <zephyr/sys/reboot.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
#include <stdbool.h>


#include "mqtt_connection.h"
#include "ble.h"
#include "aggregator.h"
#include "update.h"
#include "oneM2Mmessages.h"
#include "main.h"




#define FAST_BLINK_INTERVAL 250  // 250 ms.
#define SLOW_BLINK_INTERVAL 500  
#define VERY_SLOW_BLINK_INTERVAL 1000  

static K_SEM_DEFINE(lte_connected, 0, 1);
/* The mqtt client struct */
struct mqtt_client client;
/* File descriptor */
struct pollfd fds;

uint32_t led_condition = 0; //Current LED condition.

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
#define LEDS_RAPID_BLINK_INTERVAL K_MSEC(100) // 100 milliseconds


/* Interval in milliseconds after the device will retry cloud connection
 * if the event NRF_CLOUD_EVT_TRANSPORT_CONNECTED is not received.
 */
#define RETRY_CONNECT_WAIT K_MSEC(90000)

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
//static uint32_t last_update = 0;

/**@brief Update LEDs state. */
static void leds_update(struct k_work *work) {
    static bool fast_blink_on = false, slow_blink_on = false, very_slow_blink_on = false;
    static uint32_t last_fast_blink = 0, last_slow_blink = 0, last_very_slow_blink = 0;
    uint8_t led_blink_mask = 0, led_on_mask = 0;
    uint32_t now = k_uptime_get_32();

    // Fast Blink Toggle
    if (now - last_fast_blink >= FAST_BLINK_INTERVAL) {
        fast_blink_on = !fast_blink_on;
        last_fast_blink = now;
    }

    // Slow Blink Toggle
    if (now - last_slow_blink >= SLOW_BLINK_INTERVAL) {
        slow_blink_on = !slow_blink_on;
        last_slow_blink = now;
    }

    // Very Slow Blink Toggle
    if (now - last_very_slow_blink >= VERY_SLOW_BLINK_INTERVAL) {
        very_slow_blink_on = !very_slow_blink_on;
        last_very_slow_blink = now;
    }

    // Set LED masks based on conditions
    if ((led_condition & CONDITION_ESP32_CONNECTING) && fast_blink_on) {
        led_blink_mask |= DK_LED1_MSK;
    } else if ((led_condition & CONDITION_ESP32_SCANNING) && slow_blink_on) {
        led_blink_mask |= DK_LED1_MSK;
    } else if ((led_condition & CONDITION_ESP32_UPDATING) && very_slow_blink_on) {
        led_blink_mask |= DK_LED1_MSK;
    } else if (led_condition & CONDITION_ESP32_CONNECTED) {
        led_on_mask |= DK_LED1_MSK;
    }

	if ((led_condition & CONDITION_RPI_CONNECTING) && fast_blink_on) {
		led_blink_mask |= DK_LED2_MSK;
	} else if ((led_condition & CONDITION_RPI_SCANNING) && slow_blink_on) {
		led_blink_mask |= DK_LED2_MSK;
	} else if ((led_condition & CONDITION_RPI_UPDATING) && very_slow_blink_on) {
		led_blink_mask |= DK_LED2_MSK;
	} else if (led_condition & CONDITION_RPI_CONNECTED) {
		led_on_mask |= DK_LED2_MSK;
	}

	// LTE and MQTT Conditions
	if ((led_condition & CONDITION_LTE_CONNECTING) && slow_blink_on) {
		led_blink_mask |= DK_LED3_MSK;
	} else if ((led_condition & CONDITION_MQTT_CONNECTING) && fast_blink_on) {
		led_blink_mask |= DK_LED3_MSK;
	} else if (led_condition & CONDITION_MQTT_LTE_CONNECTED) {
		led_on_mask |= DK_LED3_MSK;
	}

   // Update LEDs
    dk_set_leds(led_blink_mask | led_on_mask);

    // Reschedule work
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
				   CONFIG_BUTTON_EVENT_PUBLISH_MSG, sizeof(CONFIG_BUTTON_EVENT_PUBLISH_MSG), "/oneM2M/req/NRFParent:NRF9160/capstone-iot/json"); //if this doesn't work, add -1 back to sizeof. 
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
	k_work_schedule(&periodic_transmit_work, K_MSEC(1000));
	k_work_init_delayable(&periodic_publish_work, publish_aggregated_data);
	k_work_schedule(&periodic_publish_work, K_MSEC(1000));
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */

static int modem_configure(void)
{
	int err;
	led_condition |= CONDITION_LTE_CONNECTING; //Set LED condition to LTE connecting.
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
	led_condition &= ~CONDITION_LTE_CONNECTING; 
	led_condition |= CONDITION_MQTT_CONNECTING;	//LTE is complete. Next step is MQTT.

	return 0;
}

int main(void)
{
	
	int err;
	uint32_t connect_attempt = 0;
	LOG_INF("Welcome to PSU CAPSTONE FALL 2023");

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
	//updating the flex container for 9160.
	updateFlexContainerForConnectedDevice("9160", true);
	
	work_init();		//Initalize work: Uplink Aggregator publish, Downlink transmit and LED functions.
	err = ble_init();		    //Initializes bluetooth (ble.c)
	if (err) {
		LOG_ERR("Failed to initialize BLE: %d", err);
		return 0;
	}
	err = OTAInit();		//Initializes OTA update (update.c)
	if (err) {
		LOG_ERR("Failed to initialize OTA: %d", err);
		return 0;
	}
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
	LOG_ERR("How'd we get here?");
	/* This is never reached */
	return 0;
}
