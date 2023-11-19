# Firmware Update Procedure
Firmware updates are initialized and oversee by oneM2M, but the update itself follows a different protocol. The whole update process from initialization to completion is outlined in this document.  
The firmware update process for specific devices (Raspberry Pi, ESP32, and nRF9160) are shown in their respective documents here:
- [Raspberry Pi](RpiUpdate)
- [ESP32](ESP32Update)
- [nRF 9160](9160Update.md)
# Initialization:
Initialization of the update is done completed following oneM2M. To learn more about oneM2M in our project, see the [oneM2M doc](oneM2M.md)

# The Update:
The maximum transfer unit, or MTU of our devices in a BLE network with the nRF9160DK is 247 Bytes. This means that although MQTT has a theoretical maximum package size of 256 MB, it is much less processing on the nRF9160DK to pre-chunk the packages down a maximum of 247 before sending.
## Topics:
Each topic will have specific MQTT topics where any information related to the topic will be published.  
These are:  
Raspberry Pi:  
```
/RaspberryPi/Update
```
ESP32:  
```
/ESP32/Update
```
nRF9160:  
```
/9160/Update
```
## Packets:

### Downlink:
Two downlink message types are sent from the webapp to the device. An update initilization message and the chunked firmware.  
  
Update Initialize:  
```
{
Total Update Chunks: 123
Total Size: 1 mB
}
```
Update Chunk:
```
{
I: 123,               //Chunk Number, between 0 and Total Update Chunks.
D: "Encoded_data",    //Data Encoded in Base64
C: "Checksum_value",  //Checksum calculated using CRC32
}
```


### Uplink:
Two uplink (Device to Webapp) messages are sent during the update. An acknowledge message and error messages, if necessary.
  
Update Acknowledge:
```
{
    "Firmware ACK": "Expected Chunks: 123 Expected Size: 1 mB"
}
```
Update Error (NACK): 
```
{
    "Firmware Error": "Packet Lost: 32"
}
```
```
Note: This is the message seen on Django.
The ESP32 or Raspberry Pi sends the message "Packet Lost: 32" over BLE to the 9160,
which the 9160 will wrap the string the shown JSON format.
```
Handshaking and acknowledgements are kept to a minimum to avoid cluttering BLE or MQTT during messages.  

## On Django WebApp:
On the Django WebApp, the steps are the following: 
1. Chunk the update, each to 200 bytes.
2. Create a checksum of each chunk (CRC32).
3. Base64 Encode each chunk.
4. Send a message stating the total amount of chunks and the total update size to the device's update topic.
5. Once acknowledgement is received from the nRF9160DK, begin sending the chunks.
6. Handle any errors. The error will state where the chunk was lost or corrupted and the Django WebApp will begin resending at the lost or corrupted chunk.

## On nRF-9160 DK: 
Each chunk number and checksum are checked on the 9160DK before being forwarded. This reduces response time to errors, should they happen over MQTT.
A flag is set on the nRF-9160DK with the first "Update Initialize" message. The 9160 also saves the total number of chunks, although this variable is not used at the moment. (May be implemented in a check).  
Importantly, the expected chunk number for the update is set to zero.  
  
Each packet is checked with the the expected chunk number on the 9160. the 9160 will then decode and re-calculate the checksum. If both of these checks pass, then the expected chunk number is incremented and the packet is forwarded over BLE.  
If either of these checks fail, an error message is published over MQTT to the corresponding topic.  
For more detail, see the [9160 firmware Update doc](9160Update.md).
## Confirmation:
