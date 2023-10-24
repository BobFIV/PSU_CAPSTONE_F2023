#!/usr/bin/env python
import time
#from bluetooth.ble import DiscoveryService
#
#service = DiscoveryService("hci0")
#devices = service.discover(4)
#
#for address, name in list(devices.items()):
#    print("name: {}, address: {}".format(name, address))
#
#print("Done.")
from bluepy.btle import Scanner, DefaultDelegate,Peripheral

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev:
            print "Discovered device", dev.addr
        elif isNewData:
            print "Received new data from", dev.addr

send_buf=[0x31,0x32,0x33,0x34]
scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(10.0)
for dev in devices:
		info=dev.getScanData()
		print info
		print  len(info)
		if len(info)>2:	
			print  info[0][2]
			if info[0][2]=="SCREAM":
				print dev.addr 
				conn = Peripheral(dev.addr,"random")	
				while True:
					ble_buf=chr(send_buf[0])+chr(send_buf[1])+chr(send_buf[2])+chr(send_buf[3])
					conn.writeCharacteristic(0x0011,"41424344\r\n",withResponse=False)
#					time.sleep(1)