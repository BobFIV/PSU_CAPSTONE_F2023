# nRF9160: oneM2M IPE Program
Pennsylvania State University Fall 2023 Capstone Project

Sponsored by Exacta Global Smart Solutions

This program runs a oneM2M compliant IPE program, communicating to the cloud via MQTT over cellular.

The program also runs a Bluetooth® Low Energy central program to manage devices on the Raspberry Pi and ESP32.

## Requirements

The sample supports the following development kit:

- nrf9160DK

The program also supports connections to the following boards:

- Raspberry Pi 4B running a NUS peripheral program
- ESP32 running a NUS peripheral program

## Overview

Under the oneM2M standard, an IPE (interworking proxy entity) is a oneM2M compliant device that bridges non-oneM2M devices to the oneM2M network.

This program leverages the following connections:

- MQTT connection over cellular to an ACME server
- MQTT connection over cellular to a server hosting firmware
- UART connection to the on-board 52840 SoC, which runs as a BLE HCI controller
- BLE connection to an external Raspberry Pi
- BLE connection to an external ESP32

Using these connections, it accomplishes the following:

- Validate and receive 9160 firmware for an OTA (over the air) update
- Validate and forward ESP32 and Raspberry Pi firmware updates
- Handle errors for these updates
- Subscribe and Update the ACME CSE for device management
- Handle oneM2M messages and respond following the oneM2M standard
- Run Peer-to-Peer data transfer: It will transmit images from the ESP32 to the Raspberry Pi

## User interface
**LED 1** and **LED 2**:
- **LED 1** blinking: The device is connecting to the ESP32.
- **LED 1** lit: The device is connected to the ESP32.
- **LED 2** blining: The device is connecting to the Raspberry Pi
- **LED 2** lit: The device is connected to the Raspberry Pi
**LED 3** and **LED 4**:
- **LED 3** blinking: The device is connecting to the LTE network.
- **LED 3** lit: The device is connected to the LTE network.
- **LED 4** blinking: The device is receiving a firmware update over MQTT.
- **LED 4** lit: The device is completed recieving a firmware update and is going to restart.

All LEDs (1-4):
- Blinking in groups of two (**LED 1** and **LED 3**, **LED 2** and **LED 4**): Modem fault.
- Blinking in groups of two (**LED 1** and **LED 2**, **LED 3** and **LED 4**): Other error.

## Building and running

### Programming the Application

You must program the *board controller* with the `hci-lpuart` sample first, before programming the main controller with the LTE Sensor Gateway sample application.

Program the board controller as follows:

1. Set the **SW10** switch, marked as *debug/prog*, in the **NRF52** position. On nRF9160 DK board version 0.9.0 and earlier versions, the switch was called **SW5**.
2. The program **MUST** be built using west, specifying your board version (This can be found on the back of your nRF9160DK).

Build the `hci-lpuart` program for the nrf9160dk_nrf52840 build target and program the board controller with it.

> **Note:**
> To build the sample successfully, you must specify the board version along with the build target.
> The board version is printed on the label of your DK, just below the PCA number.
> For example, for board version 1.1.0, the sample must be built in the following way:
> ```
> west build --board nrf9160dk_nrf52840@1.1.0
> ```

After programming the board controller, you must program the main controller with this program.

Program the main controller as follows:

1. Set the **SW10** switch, marked as *debug/prog*, in the **NRF91** position.
2. Build the IPE program (this program) for the nrf9160dk_nrf9160_ns build target and program the main controller with it.

> **Note:**
> To build the sample successfully, you must specify the board version along with the build target.
> For example, for board version 1.1.0, the sample must be built in the following way:
> ```
> west build --board nrf9160dk_nrf9160_ns@1.1.0
> ```

If you build without specifying the board version, the program will compile but it will likely crash at the first BLE connection.

This is unfortunately a problem with nrf connect that is out of our control.

## File Structure Breakdown

`src` contains multiple header and source files. Although the names are self-explanatory, there is some overlap between the files. Here are the general guidelines for which files accomplish what:

- `aggregator.c/aggregator.h`: The FIFO queue for uplink and downlink packets. When messages from MQTT need to be forwarded to BLE or messages from BLE need to be forward over MQTT, it is placed in its respective aggregator.
- `ble.c/ble.h`: Manages the upper layers of the BLE stack (52840 manages the lower layers). All connections, transmissions, and reception over BLE are completed here.
- `cJSON.c/cJSON.h`: Created by Dave Gamble and cJSON contributors, open-source JSON library for C. All credit and appreciation to them.
- `main.c/main.h`: Runs initialization for all parts of code as well as LTE and modem.
- `mqtt_ble_pipe.c/mqtt_ble_pipe.h`: Runs the area between `aggregator.c` and `ble.c`/`mqtt_connection.c`. A message from aggregator will be sent here, then published/transmitted here based on a work timer.
- `mqtt_connection.c/mqtt_connection.h`: manages MQTT connections to the cloud. All publishing and subscriptions are handled. Notably, all firmware validation is completed in-house here.
- `update.c/update.h`: Although it houses variables pertaining to updates of ESP32 and Raspberry Pi, this logic only deals with updates pertaining to the 9160 using MCUboot.
