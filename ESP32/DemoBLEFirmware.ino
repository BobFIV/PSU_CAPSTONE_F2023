#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER

#include "Arduino.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_jpg_decode.h"

// GPIO pins configuration for camera (AI-THINKER Model)
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
#define LIGHT_PIN 4  

// Define your button pin
#define BUTTON_PIN 13 // Replace with your actual button GPIO pin
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define DEVICE_NAME "ESP32"   //The 9160-DK is looking for this value.

#define firmwareVersion "1.0" // Firmware version

void sendFirmwareVersion() {
    String message = "ESP32 firmware version is: " + String(firmwareVersion);
    uint8_t dataPacket[message.length() + 1];
    dataPacket[0] = 0x03; // Header byte for firmware version message
    memcpy(&dataPacket[1], message.c_str(), message.length());

    pTxCharacteristic->setValue(dataPacket, message.length() + 1);
    pTxCharacteristic->notify();
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
 
      bleSerial->receiveBuffer = bleSerial->receiveBuffer + pCharacteristic->getValue();
      Serial.print("Received data: ");
      Serial.println(pCharacteristic->getValue().c_str());
      if (receivedData == "Firmware Version Request") {
        // Call the method to send firmware version
        sendFirmwareVersion();
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

size_t BLESerial::write(const uint8_t* buffer, size_t size) {
    // Create a non-const copy of the buffer
    uint8_t tempBuffer[size];
    memcpy(tempBuffer, buffer, size);

    // Write the non-const buffer as a single BLE notification
    pTxCharacteristic->setValue(tempBuffer, size);
    pTxCharacteristic->notify();
    delay(10); // Delay to prevent congestion

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
unsigned long counter = 0;
int colorCounter = 0;

void initCamera();
void captureAndProcessImage();
void computeAverageColor(const uint8_t *buf, size_t len, uint8_t *avgColor);
void startServer();
void handleRoot();
void YUVToRGB(uint8_t Y, uint8_t U, uint8_t V, int *R, int *G, int *B);

void setup() {
  Serial.begin(115200);
  bt.begin(DEVICE_NAME);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LIGHT_PIN, OUTPUT);
  initCamera();
}

void handleRoot() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  esp_camera_fb_return(fb);
}


void loop() {
    delay(10); 
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

  
  // if (digitalRead(BUTTON_PIN) == LOW) {
    delay(415); // Debounce delay
    captureAndProcessImage();
    
  }
// }

void initCamera() {
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_YUV422;
  config.frame_size = FRAMESIZE_HVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void captureAndProcessImage() {
  // digitalWrite(LIGHT_PIN, HIGH); // Turn on the light
 // delay(10); // Wait for 0.5 seconds

  camera_fb_t *fb = esp_camera_fb_get();
  // digitalWrite(LIGHT_PIN, LOW); // Turn off the light
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Compute average color (your existing functionality)
  uint8_t avgColor[3];
  computeAverageColor(fb->buf, fb->len, avgColor);
  Serial.print("Average Color - R: ");
  Serial.print(avgColor[0]);
  Serial.print(", G: ");
  Serial.print(avgColor[1]);
  Serial.print(", B: ");
  Serial.println(avgColor[2]);
  

  // Create the RGB string
  String rgbString = "R: " + String(avgColor[0]) + ", G: " + String(avgColor[1]) + ", B: " + String(avgColor[2]);

  bool connected = bt.connected();
  if (connected) {
      char header = 0x04; // Example header byte in hex
      char headerString[3]; // Buffer to store hexadecimal string
      sprintf(headerString, "%02X", header); // Convert to a 2-digit hexadecimal string

      // Concatenate header string with color string
      String sendMessage = String(headerString) + rgbString;

      // Send the message
      bt.write((uint8_t*)sendMessage.c_str(), sendMessage.length());
      Serial.print("Sent message: ");
      Serial.println(sendMessage);
  }
  esp_camera_fb_return(fb);
}


void computeAverageColor(const uint8_t *buf, size_t len, uint8_t *avgColor) {
    long sumR = 0, sumG = 0, sumB = 0;
    int pixelCount = len / 8; // Because in YUV422, two bytes (Y and either U or V) represent a pixel

    for (size_t i = 0; i < len; i += 16) {
        // Extract YUV components
        uint8_t Y1 = buf[i];
        uint8_t U  = buf[i + 1];
        uint8_t Y2 = buf[i + 2];
        uint8_t V  = buf[i + 3];
        // Convert YUV to RGB for the first pixel
        int R1, G1, B_1;
        YUVToRGB(Y1, U, V, &R1, &G1, &B_1);
        // Convert YUV to RGB for the second pixel
        int R2, G2, B2;
        YUVToRGB(Y2, U, V, &R2, &G2, &B2);
        // Sum up the RGB values
        sumR += (R1 + R2);
        sumG += (G1 + G2);
        sumB += (B_1 + B2);
    }

    // Calculate average RGB values
    avgColor[0] = sumR / pixelCount;
    avgColor[1] = sumG / pixelCount;
    avgColor[2] = sumB / pixelCount;
}

void YUVToRGB(uint8_t Y, uint8_t U, uint8_t V, int *R, int *G, int *B) {
    *R = Y + 1.402 * (V - 128);
    *G = Y - 0.344136 * (U - 128) - 0.714136 * (V - 128);
    *B = Y + 1.772 * (U - 128);
    // Clamp to 0-255
    *R = min(max(*R, 0), 255);
    *G = min(max(*G, 0), 255);
    *B = min(max(*B, 0), 255);
}
