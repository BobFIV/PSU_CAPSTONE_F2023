#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER

#include "Arduino.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE UUIDs
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define DEVICE_NAME "ESP32" 

#define firmwareVersion "1.0" // Firmware version

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


unsigned long lastCaptureTime = 0;
const unsigned long captureInterval = 5000; // 5 seconds


int mtuSize = 247; // Maximum Transmission Unit (MTU)
int headerSize = 1; // Size of the header
int maxDataSize = mtuSize - headerSize; // Maximum data size per packet


BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
std::string rxValue; // Buffer for received data

void initCamera();
void setCameraParam(int paramInt);
void capture();
void writeBLE(camera_fb_t *fb);


void sendFirmwareVersion() {
    String message = "ESP32 firmware version is: " + String(firmwareVersion);
    uint8_t dataPacket[message.length() + 1];
    dataPacket[0] = 0x03; // Header byte for firmware version message
    memcpy(&dataPacket[1], message.c_str(), message.length());

    pTxCharacteristic->setValue(dataPacket, message.length() + 1);
    pTxCharacteristic->notify();
}


void sendImageData(uint8_t* imageData, int imageSize) {
    int offset = 0; // Offset for slicing the image data

    while (offset < imageSize) {
        // Calculate size of current chunk
        int chunkSize = min(maxDataSize, imageSize - offset);
        
        // Allocate memory for data chunk + header
        uint8_t dataPacket[chunkSize + headerSize];

        // Set the header
        dataPacket[0] = 0x04; // Header byte

        // Copy the chunk of image data into the data packet
        memcpy(&dataPacket[headerSize], &imageData[offset], chunkSize);

        // Send the chunk over BLE
        pTxCharacteristic->setValue(dataPacket, chunkSize + headerSize);
        pTxCharacteristic->notify();

        // Move to the next chunk
        offset += chunkSize;

        // Small delay to prevent BLE congestion
        delay(20);
    }
}

void captureAndSendImage() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // Assuming fb->buf contains the raw grayscale image data
    sendImageData(fb->buf, fb->len);

    esp_camera_fb_return(fb);
}

class BLESerial: public Stream
{
    public:

        BLESerial(void);
        ~BLESerial(void);

        bool begin(char* localName="ESP32");
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
        std::string rxValue = pCharacteristic->getValue();
        if (!rxValue.empty()) {
            Serial.println("Received data: " + String(rxValue.c_str()));
            bleSerial->receiveBuffer += rxValue;

            // Check for firmware version request
            if (rxValue == "Firmware Version Request") {
                sendFirmwareVersion();
            }
        }
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
                                            BLECharacteristic::PROPERTY_READ | 
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
    pRxCharacteristic->addDescriptor(new BLE2902());

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


//Camera Initialization
void initCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

}

void setup() {
  Serial.begin(115200);
  initCamera();
  bt.begin(DEVICE_NAME);
  //pinMode(r1Pin, OUTPUT);
  Serial.println("Device started!");
}

void loop() {
    // Check if it's time to capture and send a photo and if the device is connected
    if (deviceConnected && (millis() - lastCaptureTime >= captureInterval)) {
        lastCaptureTime = millis();
        captureAndSendImage(); // Capture and send the image
    }
}
