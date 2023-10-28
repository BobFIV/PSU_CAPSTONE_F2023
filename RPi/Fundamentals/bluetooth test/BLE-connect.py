from bluepy import btle
import time
import binascii


# Address of BLE device to connect to.
BLE_ADDRESS = "CA:67:45:DC:EE:05"

BLE_SERVICE_UUID ="6e400001-b5a3-f393-e0a9-e50e24dcca9e"

BLE_CHARACTERISTIC_TX_UUID= "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
BLE_CHARACTERISTIC_RX_UUID= "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

print("Connecting...")
dev = btle.Peripheral(BLE_ADDRESS,"random")

print("Services...")
for svc in dev.services:
    print(str(svc))

class MyDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)
        # ... initialise here

    def handleNotification(self, cHandle, data):
    	data = bytearray(data)
    	print('Developer: do what you want with the data.')
    	print(data)





dev.setDelegate( MyDelegate() )
 
service_uuid = btle.UUID(BLE_SERVICE_UUID)
ble_service = dev.getServiceByUUID(service_uuid)

uuidConfig = btle.UUID(BLE_CHARACTERISTIC_TX_UUID)
data_chrc = ble_service.getCharacteristics(uuidConfig)[0]
data_chrc.write(bytes("Connect with rPi", "utf-8"))


#uuidConfig_read = btle.UUID(BLE_CHARACTERISTIC_RX_UUID)
#data_chrc_read = ble_service.getCharacteristics(uuidConfig_read)[0]

time.sleep(1.0) # Allow sensor to stabilise
while True:
	if dev.waitForNotifications(1.0):
		
		# handleNotification() was called
		continue
	print("Waiting...")
	#val = binascii.b2a_hex(data_chrc_read.read())
	#val = binascii.unhexlify(val)
	#val = struct.unpack('f', val)[0]
	#print("Waiting... read value: "+val)
    
