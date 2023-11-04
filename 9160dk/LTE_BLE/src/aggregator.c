/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "aggregator.h"

LOG_MODULE_DECLARE(lte_ble_gw);

#define SERIALIZED_DATA_MAX_SIZE 300 // Some number. Probably wrong. To-Do: Update.
K_FIFO_DEFINE(uplink_aggregator_fifo);
K_FIFO_DEFINE(downlink_aggregator_fifo);

static uint32_t uplink_entry_count;

static uint32_t downlink_entry_count;

struct uplink_fifo_entry {
	void *fifo_reserved;
	uint8_t data[sizeof(struct uplink_data_packet)];
};

struct downlink_fifo_entry {
	void *fifo_reserved;
	uint8_t data[sizeof(struct downlink_data_packet)];
};

//newstruct for firmware:
struct fdownlink_fifo_entry {
	void *fifo_reserved;
	uint8_t data[sizeof(struct firmware_update_packet_t)];
};

int uplink_aggregator_put(struct uplink_data_packet in_data)
{
	struct uplink_fifo_entry *fifo_data = NULL;
	uint32_t  lock = irq_lock();
	int    err  = 0;

	if (uplink_entry_count == FIFO_MAX_ELEMENT_COUNT) {
		fifo_data = k_fifo_get(&uplink_aggregator_fifo, K_NO_WAIT);

		__ASSERT(fifo_data != NULL, "fifo_data should not be NULL");

		uplink_entry_count--;
	}

	if (fifo_data == NULL) {
		fifo_data = k_malloc(sizeof(in_data));
	}

	if (fifo_data == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	memcpy(fifo_data->data, &in_data, sizeof(in_data));

	k_fifo_put(&uplink_aggregator_fifo, fifo_data);
	uplink_entry_count++;

exit:
	irq_unlock(lock);
	return err;
}

int uplink_aggregator_get(struct uplink_data_packet *out_data)
{
	void  *fifo_data;
	int   err = 0;

	if (out_data == NULL) {
		return -EINVAL;
	}

	uint32_t lock = 0;
	lock = irq_lock();

	fifo_data = k_fifo_get(&uplink_aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		err = -ENODATA;
		goto exit;
	}

	memcpy(out_data, ((struct uplink_fifo_entry *)fifo_data)->data,
	       sizeof(struct uplink_data_packet));

	k_free(fifo_data);
	uplink_entry_count--;

exit:
	irq_unlock(lock);
	return err;
}




int downlink_aggregator_get(struct downlink_data_packet *out_data)
{
	//LOG_INF ("downlink_aggregator_get called"); //Super spammy.
	void  *fifo_data;
	int   err = 0;
	
	if (out_data == NULL) {
		//LOG_INF ("OutData is NULL");
		return -EINVAL;
	}


	fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		err = -ENODATA;
		//LOG_INF ("FIFO is Empty"); //Super spammy.
		goto exit;
	}


	uint32_t lock = irq_lock();


	memcpy(out_data, ((struct downlink_fifo_entry *)fifo_data)->data,
	       sizeof(struct downlink_data_packet));
	LOG_INF ("downlink_aggregator_get: %s", out_data->data);
	k_free(fifo_data);
	downlink_entry_count--;

exit:
	irq_unlock(lock);
	return err;
}

int downlink_aggregator_put(struct downlink_data_packet in_data)
{
	struct downlink_fifo_entry *fifo_data = NULL;
	uint32_t  lock = irq_lock();
	int	err  = 0;
	LOG_INF ("downlink_aggregator_put called");
	if (downlink_entry_count == FIFO_MAX_ELEMENT_COUNT) {
		fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);

		__ASSERT(fifo_data != NULL, "fifo_data should not be NULL");

		downlink_entry_count--;
	}

	if (fifo_data == NULL) {
		fifo_data = k_malloc(sizeof(struct downlink_fifo_entry));
	}

	if (fifo_data == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	
	memcpy(fifo_data->data, &in_data, sizeof(struct downlink_data_packet));
	LOG_INF("downlink_aggregator_put: %s", in_data.data);
	k_fifo_put(&downlink_aggregator_fifo, fifo_data);
	downlink_entry_count++;

exit:
	irq_unlock(lock);
	return err;
}

//new downlink put function for new struct:
int fdownlink_aggregator_get(struct firmware_update_packet_t *out_data)
{
	//LOG_INF ("downlink_aggregator_get called"); //Super spammy.
	void  *fifo_data;
	int   err = 0;
	
	if (out_data == NULL) {
		//LOG_INF ("OutData is NULL");
		return -EINVAL;
	}


	fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);
	if (fifo_data == NULL) {
		err = -ENODATA;
		//LOG_INF ("FIFO is Empty"); //Super spammy.
		goto exit;
	}


	uint32_t lock = irq_lock();


	memcpy(out_data, ((struct fdownlink_fifo_entry *)fifo_data)->data,
	       sizeof(struct firmware_update_packet_t));
	LOG_INF ("downlink_aggregator_get: %s", out_data->data);
	k_free(fifo_data);
	downlink_entry_count--;

exit:
	irq_unlock(lock);
	return err;
}

int fdownlink_aggregator_put(struct firmware_update_packet_t in_data)
{
	struct downlink_fifo_entry *fifo_data = NULL;
	uint32_t  lock = irq_lock();
	int	err  = 0;
	LOG_INF ("downlink_aggregator_put called");
	if (downlink_entry_count == FIFO_MAX_ELEMENT_COUNT) {
		fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);

		__ASSERT(fifo_data != NULL, "fifo_data should not be NULL");

		downlink_entry_count--;
	}

	if (fifo_data == NULL) {
		fifo_data = k_malloc(sizeof(struct fdownlink_fifo_entry));
	}

	if (fifo_data == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	
	memcpy(fifo_data->data, &in_data, sizeof(struct firmware_update_packet_t));
	LOG_INF("downlink_aggregator_put: %s", in_data.data);
	k_fifo_put(&downlink_aggregator_fifo, fifo_data);
	downlink_entry_count++;

exit:
	irq_unlock(lock);
	return err;
}

void aggregator_clear(void)
{
	void *fifo_data;
	uint32_t lock = 0;
	lock = irq_lock();

	while (1) {
		fifo_data = k_fifo_get(&uplink_aggregator_fifo, K_NO_WAIT);

		if (fifo_data == NULL) {
			break;
		}

		k_free(fifo_data);
	};

	uplink_entry_count = 0;

	while (1) {
		fifo_data = k_fifo_get(&downlink_aggregator_fifo, K_NO_WAIT);

		if (fifo_data == NULL) {
			break;
		}

		k_free(fifo_data);
	};
	downlink_entry_count = 0;
	irq_unlock(lock);
}

