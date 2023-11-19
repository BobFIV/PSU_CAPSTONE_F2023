#
# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)



find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lte-ble-gateway)

# NORDIC SDK APP START
target_sources(app PRIVATE src/mqtt_connection.c)
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/ble.c)
target_sources(app PRIVATE src/mqtt_ble_pipe.c)
target_sources(app PRIVATE src/aggregator.c)
target_sources(app PRIVATE src/cJSON.c)
target_sources(app PRIVATE src/update.c)
# NORDIC SDK APP END

zephyr_include_directories(src)
include_directories(src/orientation_detector)
