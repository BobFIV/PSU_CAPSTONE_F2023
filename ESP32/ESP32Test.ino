#include "Arduino.h"
#include "Stream.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


#define DEVICE_NUMBER "1"
#define DEVICE_NAME "ESP32-" DEVICE_NUMBER

//GPIO pins for our light control
// Note: These are not physical pin numbers, these are GPIO pin numbers. Example: r1Pin is set to GPIO23, which is physical pin #37
//int r1Pin = 23;

class BLESerial: public Stream
{
    public:

        BLESerial(void);
        ~BLESerial(void);

        bool begin(char* localName="IoT-Bus UART Service");
        int available(void);
        int peek(void);
        bool connected(void);
        int read(void);
        size_t write(uint8_t c);
        size_t write(const uint8_t *buffer, size_t size);
        void flush();
        void end(void);

    private:
        String local_name;
        BLEServer *pServer = NULL;
        BLEService *pService;
        BLECharacteristic * pTxCharacteristic;
        bool deviceConnected = false;
        uint8_t txValue = 0;
        
        std::string receiveBuffer;

        friend class BLESerialServerCallbacks;
        friend class BLESerialCharacteristicCallbacks;

};

class BLESerialServerCallbacks: public BLEServerCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onConnect(BLEServer* pServer) {
        // do anything needed on connection
        Serial.println("Device connected!");
        delay(1000); // wait for connection to complete or messages can be lost
    };

    void onDisconnect(BLEServer* pServer) {
        pServer->startAdvertising(); // restart advertising
        Serial.println("Started advertising due to disconnect!");
    }
};

class BLESerialCharacteristicCallbacks: public BLECharacteristicCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onWrite(BLECharacteristic *pCharacteristic) {
 
      bleSerial->receiveBuffer = bleSerial->receiveBuffer + pCharacteristic->getValue();
      Serial.print("Received data: ");
      Serial.println(pCharacteristic->getValue().c_str());
    }

};

// Constructor

BLESerial::BLESerial()
{
  // create instance  
  receiveBuffer = "";
}

// Destructor

BLESerial::~BLESerial(void)
{
    // clean up
}

// Begin bluetooth serial

bool BLESerial::begin(char* localName)
{
    // Create the BLE Device
    BLEDevice::init(localName);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    if (pServer == nullptr)
        return false;
    
    BLESerialServerCallbacks* bleSerialServerCallbacks =  new BLESerialServerCallbacks(); 
    bleSerialServerCallbacks->bleSerial = this;      
    pServer->setCallbacks(bleSerialServerCallbacks);

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);
    if (pService == nullptr)
        return false;

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_TX,
                                            BLECharacteristic::PROPERTY_NOTIFY
                                        );
    if (pTxCharacteristic == nullptr)
        return false;                    
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                                BLECharacteristic::PROPERTY_WRITE
                                            );
    if (pRxCharacteristic == nullptr)
        return false; 
    //pRxCharacteristic->addDescriptor(new BLE2902());

    BLESerialCharacteristicCallbacks* bleSerialCharacteristicCallbacks =  new BLESerialCharacteristicCallbacks(); 
    bleSerialCharacteristicCallbacks->bleSerial = this;  
    pRxCharacteristic->setCallbacks(bleSerialCharacteristicCallbacks);

    // Start the service
    pService->start();
    Serial.println("starting service");

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(pService->getUUID()); 
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
    return true;
}

int BLESerial::available(void)
{
    // reply with data available
    return receiveBuffer.length();
}

int BLESerial::peek(void)
{
    // return first character available
    // but don't remove it from the buffer
    if ((receiveBuffer.length() > 0)){
        uint8_t c = receiveBuffer[0];
        return c;
    }
    else
        return -1;
}

bool BLESerial::connected(void)
{
    // true if connected
    if (pServer->getConnectedCount() > 0)
        return true;
    else 
        return false;        
}

int BLESerial::read(void)
{
    // read a character
    if ((receiveBuffer.length() > 0)){
        uint8_t c = receiveBuffer[0];
        receiveBuffer.erase(0,1); // remove it from the buffer
        return c;
    }
    else
        return -1;
}

size_t BLESerial::write(uint8_t c)
{
    // write a character
    uint8_t _c = c;
    pTxCharacteristic->setValue(&_c, 1);
    pTxCharacteristic->notify();
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    return 1;
}

size_t BLESerial::write(const uint8_t *buffer, size_t size)
{
    // write a buffer
    for(int i=0; i < size; i++){
        write(buffer[i]);
  }
  return size;
}

void BLESerial::flush()
{
    // remove buffered data
    receiveBuffer.clear();
}

void BLESerial::end()
{
    // close connection
}

//Instance of the class that handles bt communication
BLESerial bt;

String message = "";

void setup() {
  Serial.begin(115200);
  bt.begin(DEVICE_NAME);
  //pinMode(r1Pin, OUTPUT);
  Serial.println("Device started!");
}

unsigned long counter = 0;

void loop() {
  //If the BT server has given us a message
  if(bt.available()) {
    //Read incoming byte
    char incomingChar = bt.read();

    //if it is the end of a word, we can print and clear it. Else, build the string
    if(incomingChar != ';') {
      message += String(incomingChar);
    } else {
      Serial.println(message); // Print the message to the terminal
      message = "";
    }
  }
    // Every 5000 milliseconds (5 seconds), send a message
  if (millis() - counter > 5000) {
    counter = millis();
    const char* message = "{\"to\":\"cse-in\",\"op\":2,\"rqi\":\"Hi from the ESP32SiP\",\"rvi\":\"3\",\"fr\":\"CAdmin\"}";
    bt.write((uint8_t*)message, strlen(message));
    Serial.print("Sent message: ");
    Serial.println(message);
  }
}