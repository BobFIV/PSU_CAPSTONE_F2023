# nRF9160: IPE PROGRAM
Pennsylvania State University Fall 2023 Capstone Project

Sponsored by Exacta Global Smart Solutions

Firmware for the Nordic Semicondutor nRF9160 Development Kit. See the readme in each program for deeper explanation here:
- [52840](hci_lpuart) runs the lower-levels of the BLE stack.
- [9160](IPE_OTA) runs the IPE-OTA program, which manages MQTT over LTE, oneM2M communications, OTA updates and the upper layers of the BLE stack. If you are interested in the 9160 DK's code, you are likely searching here.

Directions to build the programs are provided in the respective documents. Build and flash the 52840 (hci_lpuart) program  **first** before building and flashing the 9160 (IPE_OTA) program.
