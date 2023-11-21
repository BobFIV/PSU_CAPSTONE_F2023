/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UPDATE_H__
#define UPDATE_H__

#define TLS_SEC_TAG 42
#include <zephyr/sys/atomic.h>     // For atomic_t
#ifndef CONFIG_USE_HTTPS
#define SEC_TAG (-1)
#else
#define SEC_TAG (TLS_SEC_TAG)
#endif



extern atomic_t  current9160Chunk;
extern int total9160Chunks;
extern bool update9160Flag;

extern atomic_t  currentRPiChunk;
extern int totalRPiChunks;
extern bool updateRPiFlag;

extern atomic_t  currentESP32Chunk;
extern int totalESP32Chunks;
extern bool updateESP32Flag;


int process_firmware_chunk(int chunk_id, const char *decoded_data, size_t decoded_len);

int OTAInit(void);


#endif /* UPDATE_H__ */
