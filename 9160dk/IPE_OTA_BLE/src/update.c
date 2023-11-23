//Written by Yuh Kurose.
//This file primarily works to watch over the update features on each device, as well as conduct the update for the 9160.

#include <zephyr/kernel.h>          // For main kernel and thread functionalities
#include <zephyr/drivers/flash.h>   // For flash memory operations
#include <zephyr/dfu/mcuboot.h>            // For MCUboot functionalities
#include <zephyr/dfu/flash_img.h>          // For flash image operations
#include <zephyr/storage/flash_map.h>      // For flash map API
#include <zephyr/logging/log.h>            // For logging
#include <zephyr/sys/reboot.h>      // For system reboot API

#include <zephyr/sys/atomic.h>     // For atomic_t
#include <stdbool.h>

#include "update.h"                 // Update header file
#include "main.h"                // Main header file for LEDs.
#define SECONDARY_PARTITION_LABEL mcuboot_secondary
#define FIXED_CHUNK_SIZE 273



LOG_MODULE_DECLARE(lte_ble_gw);
#define MAX_BINARY_CHUNK_SIZE  247

//static update_start_cb update_start;
//static char filename[128] = {0};

//static struct flash_img_context ctx;

//static bool ctx_initialized = false;
//static const struct flash_area *flash_area;

//initializing all values to zero. 
atomic_t current9160Chunk = ATOMIC_INIT(0);
atomic_t currentRPiChunk = ATOMIC_INIT(0);
atomic_t currentESP32Chunk = ATOMIC_INIT(0);
int total9160Chunks = 0;
int totalRPiChunks = 0;
int totalESP32Chunks = 0;
bool update9160Flag = false;
bool updateRPiFlag = 0;
bool updateESP32Flag = false;


//Takes decoded binary data and processes it.



int finalize_firmware_update(void) {
    int err;

    // Inform MCUboot that the update is complete
    err = boot_request_upgrade(BOOT_UPGRADE_TEST);
    if (err != 0) {
        LOG_ERR("Failed to mark update as complete: %d", err);
        // Handle error (e.g., retry, log, notify server)
        return -1;
    }

    LOG_INF("Firmware update marked as complete. Rebooting...");

    // Trigger a system reboot
    sys_reboot(SYS_REBOOT_COLD);
    //this is never reached. 
    return 0;
}

static int write_firmware_chunk_to_flash(uint32_t chunkId, const uint8_t *data, size_t len) {
    const struct flash_area *secondary_area;
    int err;
    size_t offset;

    // Open the secondary slot partition
    err = flash_area_open(FLASH_AREA_ID(SECONDARY_PARTITION_LABEL), &secondary_area);
    if (err != 0) {
        printk("Error opening secondary slot flash area: %d\n", err);
        return err;
    }

    // Calculate the offset for this chunk
    // For example, if each chunk is of fixed size, this could be:
    offset = chunkId * FIXED_CHUNK_SIZE;

    // Ensure offset + len does not exceed the partition size
    if (offset + len > secondary_area->fa_size) {
        printk("Chunk write exceeds partition size\n");
        flash_area_close(secondary_area);
        return -EINVAL;
    }

    // Write the chunk to the flash
    err = flash_area_write(secondary_area, offset, data, len);
    if (err != 0) {
        printk("Error writing to flash: %d\n", err);
    }

    // Close the flash area
    flash_area_close(secondary_area);

    return err;
}

int process_firmware_chunk(int chunk_id, const char *decoded_data, size_t decoded_len) {
    int err;
    uint32_t currentChunk;

    // Write the decoded chunk to the appropriate flash location
    err = write_firmware_chunk_to_flash(chunk_id, (const uint8_t *)decoded_data, decoded_len);
    if (err != 0) {
        printk("Error writing firmware chunk to flash: %d\n", err);
        // Handle the error (e.g., abort the update, retry, log error, etc.)
        return err;
    }

    // Read the current chunk value from the atomic variable
    currentChunk = atomic_get(&current9160Chunk);

    // Check if this is the last chunk
    if (currentChunk == total9160Chunks) {
        printk("Firmware update complete. Finalizing update...\n");
        led_condition &= ~CONDITION_FIRMWARE_UPDATING;
        led_condition |= CONDITION_FIRMWARE_DONE;
        err = finalize_firmware_update();
        if (err != 0) {
            printk("Error finalizing firmware update: %d\n", err);
            return err;
        }
    }

    return 0; // Success
}


int OTAInit(void)
{
	LOG_INF("OTA update Inititalizing...");
	//Initialize all values for firmware updates to zero.

	//This is needed so that MCUBoot won't revert the update
	boot_write_img_confirmed();

    return 0;
}

