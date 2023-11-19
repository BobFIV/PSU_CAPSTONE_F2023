# Firmware Update Procedure
Firmware updates are initialized and oversee by oneM2M, but the update itself follows a different protocol. The whole update process from initialization to completion is outlined in this document.  
The firmware update process for specific devices (Raspberry Pi, ESP32, and nRF9160) are shown in their respective documents here:
- [Raspberry Pi](RpiUpdate)
- [ESP32](ESP32Update)
- [nRF 9160](9160Update.md)
# Initialization:
Initialization of the update is done completed following oneM2M. To learn more about oneM2M in our project, see the [oneM2M doc](oneM2M.md)
## 1. Subscription Setup
- nRF9160 Subscription:
  - The nRF9160 is configured to subscribe to the Django WebApp Application Entity (AE).
  - This subscription enables the nRF9160 to receive notifications about updates and changes initiated by the Django WebApp AE.
## 2. Update Initiation
- Update Push by Django WebApp AE:
  - When an update for any of the boards is ready, the Django WebApp AE pushes this update.
  - The update could be related to firmware, software, or other critical data for the devices managed by the WebApp.
## 3. Notification to nRF9160
- Notification Receipt:
  - Upon the update being pushed, the nRF9160 receives a notification due to its active subscription to the Django WebApp AE.
  - This notification alerts the nRF9160 that a new update is available.
## 4. Request to Refresh Device Version
- Update Request Publication:
  - Following the receipt of the notification, the nRF9160 publishes an update request to the relevant Container within the oneM2M system.
  - This request is intended to refresh or update the version information of the affected device in the Container.
## 5. Django WebApp Observes Version Change
- Observation by Django WebApp:
  - The Django WebApp AE, also subscribed to the same Container, detects the change in version information.
  - This change triggers the next step in the update process within the WebApp's interface.
## 6. User Interface Interaction
- Update/Revert Options Presented to User:
  - In the Django WebApp's user interface, an update or revert option becomes available to the user, corresponding to the version change observed.
  - This allows the user to make an informed decision about proceeding with the update.
## 7. Execution of Update/Revert
- User Triggered Action:
  - When the user selects either the update or revert option in the WebApp interface, this action triggers the final update process.
  - The procedure for the update begins. Currently, once an update begins, there is no way for you to stop it.

Add view of CSE here.

The nRF9160 is subscribed to the Django WebApp AE. Once the AE pushes an update for any of the boards, the nRF9160 will get a notification. Following this notification, an update request is published to the container to refresh the version of the device in the container. The Django WebApp, also subscribed to each device, will see the version and an update/revert option will now be available to the user. Pressing this update/revert option will then trigger the update to occur.

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
After the board reboots, steps similar to initialization are taken. The 9160, seeing that the board is back from the update (either itself or the ESP32 and Raspberry Pi), will send a UPDATE request to update the firmware version in ACME. In the case of ESP32 and Raspberry Pi, a message containing the firmware version is sent to the 9160, which will use this data to update the CSE.
