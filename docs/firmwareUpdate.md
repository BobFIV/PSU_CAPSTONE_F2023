# Firmware Update Procedure
Firmware updates are initialized and oversee by oneM2M, but the update itself follows a different protocol. The whole update process from initialization to completion is outlined in this document.  
The firmware update process for specific devices (Raspberry Pi, ESP32, and nRF9160) are shown in their respective documents here:
- [Raspberry Pi](RpiUpdate)
- [ESP32](ESP32Update)
- [nRF 9160](9160Update.md)
# Initialization:
Initialization of the update is done completed following oneM2M. To learn more about oneM2M in our project, see the [oneM2M doc](oneM2M.md)
## 1. oneM2M setup
- Create the Flex Node:
  - When the 9160DK establishes a connection the the ACME CSE, a CREATE request is sent to establish a new Node and associated FlexNode for itself.
    - The 9160DK will populate this node with relevant device management information, such as dmDeviceInfo.
    - This process is repeated for the ESP32 and Raspberry Pi when they are connected to the 9160DK. 
    - The ESP32 and Raspberry Pi will send relevant information to the 9160 DK to help populate the Flex Nodes for each device.
- nRF9160 Subscription:
  - The nRF9160 is configured to subscribe to the Django WebApp Application Entity (AE).
  - This subscription enables the nRF9160 to receive notifications about updates and changes initiated by the Django WebApp AE.
  - The nRF9160 DK is also subscribed to all of the resources corresponding to the Flex nodes for each device. Thus, any change in the Flex Nodes will notify the 9160 DK.
- Django Subscription:
  - Likewise, Django is also subscribed to each device and their device management resources. This is done so when an update is completed, Django is notified from ACME.
## 2. Update Initiation
- Update Push by Django WebApp AE:
  - In our system, the user is able to upload firmware to the WebApp to be sent to any of the devices.
  - Once firmware is updated, the Django performs a RETRIEVE operation on the dmDeviceInfo resource in the FlexNode of the corresponding device to check the current firmware version against the new version.
  - If the version on Django is newer, an update is initiated.
	  - In our demo, the update is initiated via a button. If the version on Django is newer, an "update" button is presented. If it is older, a "revert" button is presented instead.
## 3. Update Command
- Django sends an UPDATE (or CREATE) command to the dmPackage field of the device.
	- The dmPackage resource will contain the following information:
		- The total update size (in mB)
		- The total amount of update chunks.
		- The location for the Update (MQTT Topic, in our case). 
- The nRF9160 DK will receive this information from the resource since it is subscribed to all device management fields. 
	- The nRF9160 DK will then forward this information to the device if necessary, and the target device will send a message to initiate the update. 
	- Once this message is received by the device, the device sets a flag and is ready to accept the update. It will send an update acknowledge message to the corresponding MQTT Topic, which will begin sending the updates in chunks (200 bytes each, 47 Bytes left for header.)

Add view of CSE here.
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
Downlink message types are sent from the webapp to the device. To reduce strain during critical events, the only downlink message is the chunked firmware: 
  
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
# Conclusion:
After the board reboots, steps similar to **initialization** are taken. 
1. The 9160DK will send an UPDATE in dmDeviceInfo to update the firmware version of the device.
	- If the update was for the ESP32 or Raspberry Pi, the 9160 DK will send a message requesting information from the ESP32 or Raspberry Pi when a connection is re-established.
2. The Django, being subscribed to the device management components, are provided with the new firmware version on each device.
In the case of Update failure, a timer is enabled on the nRF 9160-DK after it receives a rebooting message. If this timer times out, an update failed message is sent to update the dmDeviceInfo instead.
