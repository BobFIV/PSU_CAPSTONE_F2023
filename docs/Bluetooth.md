# Bluetooth Low-Energy Communication
All local communication is done using Bluetooth Low-Energy.

## Headers
All messages sent to the 9160 has a one-byte headers for processing. They are the following:
- 0x01: Firmware Error. Sent if the wrong chunk is received or the checksum fails. This message is forwarded by the 9160DK to MQTT and the appropriate update topic to restart packet sending.
- 0x02: Firmware Ack. This message is sent to the 9160DK to confirm the firmware initialization message. Sent after the device knows how many chunks and total bytes it is going to receive.
- 0x03: Firmware Version. Sends the current firmware version to the 9160DK. 9160DK will use this info to update the ACME.
- 0x04: Image. Not a firmware image, but a photograph. Sent by the ESP32-CAM to the 9160DK. 9160DK will then forward this message/chunk to the Raspberry Pi.
- 0x05: Train Location. This is strictly Demo data. It will be sent to the 9160 and published to the Demo topic to keep the WebApp aware of the train location of fruits and vegetables.
