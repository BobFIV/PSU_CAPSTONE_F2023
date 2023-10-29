# nrf9160-dk firmware
## Function
Currently, it runs both MQTT connection and a sole bluetooth connection to the ESP32.  
## How to build:  
Set SW10 to flash to the nRF91.
Change the UUID in the bluetooth.c folder to change the device to connect to.
Build and flash the lte_ble_gateway program to the 9160.  
Set SW10 to flash to the nRF52
Build and flash the hci_lpuart to the nrf52  
#To-Do: 
Make the two work together  
Set LED blinking  
Improve Documentation 
