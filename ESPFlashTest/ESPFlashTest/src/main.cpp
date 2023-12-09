#include "Arduino.h"
#include <esp_ota_ops.h>

const size_t BUFFER_SIZE = 119088;  // Adjust the buffer size as needed
uint8_t dataBuffer[BUFFER_SIZE];
size_t bufferIndex = 0;
int conEnter = 0; //consecutive line feeds

void updateFirmware() {
  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
  
  if (update_partition != NULL && update_partition->type == ESP_PARTITION_TYPE_APP) {
    Serial.println("Updating firmware...");

    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
      Serial.printf("OTA Begin Error: %d\n", err);
      return;
    }
    

    err = esp_ota_write(update_handle, (const void *)dataBuffer, bufferIndex);
    if (err != ESP_OK) {
      Serial.printf("OTA Write Error: %d\n", err);
      return;
    }

    Serial.printf("Binary file length:%d", bufferIndex);

    err = esp_ota_end(update_handle);
    if (err == ESP_OK) {
      Serial.println("Firmware update successful!");
      ESP.restart();
    } else {
      Serial.printf("OTA End Error: %d\n", err);
      Serial.printf("OTA End Error: %s\n", esp_err_to_name(err));
      for (int i = 0; i < bufferIndex; i++) {
          Serial.printf("%02X", dataBuffer[i]);
      }
      Serial.println();  // Print a newline to separate lines

    }
  } else {
    Serial.println("No valid update partition found.");
  }

  // Reset buffer for the next update
  bufferIndex = 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Yes we are reading");   
}

void loop() {
  // Check if there's data available to read
  if (Serial.available() > 0) {
    // Read the incoming byte
    char incomingByte = Serial.read();
    
      // Process or store the received byte as needed
      if (bufferIndex < BUFFER_SIZE) {
        dataBuffer[bufferIndex] = incomingByte;
        bufferIndex++;
      }
    

    if (incomingByte == '\n') {
      conEnter++;
    } else {
      conEnter = 0;
    }

    if (conEnter >= 4) {
      bufferIndex-=4;
      Serial.println("Received full binary data. Updating firmware...");      
      updateFirmware();
    }
  }

  // Add any other code you need to run in the loop
}
