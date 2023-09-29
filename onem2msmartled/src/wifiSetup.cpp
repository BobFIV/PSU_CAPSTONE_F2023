
#include <WiFiManager.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer  server(8080);
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
// Variable to store the HTTP request
String header;
const String reqIDHeader = "X-M2M-RI";  

// create a function pointer that takes a pointer to a uint8_t
typedef bool (*notificationHandler)(uint8_t *datas);

// create a setter function for the notification handler
void setNotificationHandler(notificationHandler handler);

// defined setNotificationHandler
static notificationHandler notHandler;




void setNotificationHandler(notificationHandler handler)
{
  // set the handler
  notHandler = handler;
}


void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

bool handleNotifications(uint8_t *datas) {

  Serial.printf("[REQUEST]\t%s\r\n", (const char*)datas);
    
  return 1;
}

// This is a nice tutorial -> https://dronebotworkshop.com/wifimanager/
void setupWifi(void)
{
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing.  these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials: Three options
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("ESPConnectAP"); // anonymous ap
  res = wm.autoConnect("ESPConnectAP", "easyToRemember"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else
  {
    Serial.println("connected...)");
    // if you get here you have connected to the WiFi

    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                         {
    if (request->url() == "/CMyApplication") 
      {
        AsyncWebHeader* reqId;
        int responseCode = 200;
        if (request->hasHeader(reqIDHeader)) {
          reqId = request->getHeader(reqIDHeader);
        } else {
          responseCode = 400;
          Serial.println("Not a oneM2M message");
        }
        AsyncWebServerResponse *response = request->beginResponse(responseCode);
        response->addHeader(reqIDHeader,reqId->value().c_str());
        request->send(response);
        if (responseCode == 200)
        {
          notHandler(data);
        }
      } });

    server.begin();
  }
}
