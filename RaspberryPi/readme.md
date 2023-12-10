# Raspberry Pi Code
## uart_peripheral.py:
the BLE connection to nRF9160. Grabbed from [here](https://scribles.net/creating-ble-gatt-server-uart-service-on-raspberry-pi/). Edited by Chang to write the output to a file.
## control.py:
The program to run, which will swap the running code to a.py as soon as it is written to via MQTT or BLE. Used MQTT directly in demo.
## demo_v1_dumb.py:
Only capabilities are start and stop train for demo.
## demo_v2_smart.py:
Added color handling. 
