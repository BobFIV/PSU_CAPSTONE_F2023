# Raspberry Pi Code
## uart_peripheral:
the BLE connection to nRF9160. Grabbed from [here](https://scribles.net/creating-ble-gatt-server-uart-service-on-raspberry-pi/). Edited by Chang to write the output to a file.  
## control.py:
The program to run, which will swap the running code to a.py as soon as it is written to via MQTT or BLE. Used MQTT directly in demo.
