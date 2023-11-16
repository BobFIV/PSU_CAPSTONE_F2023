# Firmware Update
Firmware updates are done through a custom protocol.
## Initialization:
Initialization of the update is done completed following oneM2M.

## The Update:
The maximum transfer unit, or MTU of our devices in a BLE network with the nRF9160DK is 247 Bytes. This means that although MQTT has a theoretical maximum package size of 256 MB, it is much less processing on the nRF9160DK to pre-chunk the packages down a maximum of 247 before sending.
On the Django WebApp, the steps are the following: 
1. Chunk the update, each to 200 bytes.
2. Create a checksum of each chunk (CRC32).
3. Base64 Encode each chunk.
4. Send a message stating the total amount of chunks and the total update size to the device's update topic. For example, /ESP32/Update or /RaspberryPi/Update. 
5. Once acknowledgement is received from the nRF9160DK, begin sending the chunks.
6. Handle any errors. The error will state where the chunk was lost or corrupted and the Django WebApp will begin resending at the lost or corrupted chunk.

Each chunk number and checksum are checked on the 9160DK and the end-point device to ensure no corruption or loss happens over MQTT or BLE.

## Confirmation:
