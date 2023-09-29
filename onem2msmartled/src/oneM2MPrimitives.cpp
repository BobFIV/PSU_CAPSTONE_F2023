#include "oneM2MPrimitives.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

std::pair<int, String> createAE();

static DynamicJsonDocument oneM2Mresource(1024);
static DynamicJsonDocument recievedResDoc(1024);
String originator = "CMyApplication";
const String serverName = "http://10.71.71.170:8080/cse-in";

// declare function pointer for enable and diable leds
static ledHandler enableLed;
static ledHandler disableLed;

// define setter functions that will be called externally
void setEnableLed(ledHandler handler)
{
    enableLed = handler;
}

void setDisableLed(ledHandler handler)
{
    disableLed = handler;
}

std::pair<int, String> sendHttpRequest(String resourceTargetURI, String ri, String contentType)
{
    HTTPClient http;
    String payload;
    int contentLength = serializeJson(oneM2Mresource, payload);
    http.begin(resourceTargetURI);
    http.addHeader("X-M2M-Origin", originator);
    http.addHeader("X-M2M-RI", ri);
    http.addHeader("X-M2M-RVI", "4");
    http.addHeader("Content-Type", contentType);
    http.addHeader("Connection", "close");
    http.addHeader("Content-Length", String(contentLength));
    int httpResponseCode = http.POST(payload);
    String s = http.getString();
    String oneM2MResponseCode = http.header("X-M2M-RSC");
    http.end();
    Serial.println(s);
    return std::make_pair(httpResponseCode, oneM2MResponseCode);
}

std::pair<int, String> createAE(void)
{
    JsonObject resource;
    JsonObject root;

    /* This is the payload that we need to build
    {
        'm2m:ae': {
            'rn': 'CMyApplication',
            'api': 'Nmy-application.example.com',
            'rr': True,
            'srv': ['4']
        }
    */
    oneM2Mresource.clear();
    root = oneM2Mresource.to<JsonObject>(); // Create an empty object
    resource = root.createNestedObject("m2m:ae");

    resource["rn"] = originator;
    resource["api"] = "Nmy-application.example.com";
    resource["rr"] = true;
    JsonArray poa = resource.createNestedArray("poa");
    poa.add("http://10.71.71.103:8080");
    JsonArray levels = resource.createNestedArray("srv");
    levels.add("4");
    levels.add("3");

    String resourceTargetURI = serverName;
    return sendHttpRequest(resourceTargetURI, "riCreateAE", "application/json;ty=2");
}

// This function creates AE and handles errors
bool Register(void)
{
    auto result = createAE();
    if (result.first == 403)
    {
        // most likey response; occurs on each restart
        return true;
    }
    else if (result.first == 201)
    {
        // On first registration
        return true;
    }
    else
    {
        // error scenario - this might happen if the server is not reachable
        // perhaps schedule a retry
        return false;
    }
}

std::pair<int, String> createCnt(String resourceName)
{
    JsonObject resource;
    JsonObject root;

    /* This is the payload that we need to build
    {
        'm2m:cnt': {
                'rn': resourceName   # Set the resource name
                'mni': 1
                }

    */
    oneM2Mresource.clear();
    root = oneM2Mresource.to<JsonObject>(); // Create an empty object
    resource = root.createNestedObject("m2m:cnt");

    resource["rn"] = resourceName;
    resource["mni"] = 1;

    // build the TO field -> serverAddress/cseBasename/aeName
    String resourceTargetURI = serverName;
    resourceTargetURI += "/" + originator;

    return sendHttpRequest(resourceTargetURI, "riCreateCnt", "application/json;ty=3");
}

bool createContainer(String resourceName)
{
    auto result = createCnt(resourceName);
    if (result.first == 403)
    {
        // most likey response; occurs on each restart
        return true;
    }
    else if (result.first == 201)
    {
        // On first registration
        return true;
    }
    else
    {
        // error scenario.  perhaps schedule a retry
        return false;
    }
}

std::pair<int, String> createSub(String resourceName)
{
    JsonObject resource;
    JsonObject root;

    /* This is the payload that we need to build
    {
            'm2m:sub': {
                'rn'  : 'Subscription',
                'nu'  : [ notificationURL ],
                'nct' : 1,
                'enc' : {
                    'net': [ 1, 2, 3, 4 ]
                }

    */
    oneM2Mresource.clear();
    root = oneM2Mresource.to<JsonObject>(); // Create an empty object
    resource = root.createNestedObject("m2m:sub");

    String resName = "sub" + resourceName;
    resource["rn"] = resName;
    JsonArray nu = resource.createNestedArray("nu");
    nu.add(originator);
    // nu.add("http://10.71.71.103:8080");

    resource["nct"] = 1;
    JsonObject enc = resource.createNestedObject("enc");
    JsonArray net = enc.createNestedArray("net");
    net.add(3);

    // build the TO field -> serverAddress/cseBasename/aeName/cntName
    String resourceTargetURI = serverName; // serverAddress/cseBasename
    resourceTargetURI += "/";
    resourceTargetURI += originator; // aeName
    resourceTargetURI += "/";
    resourceTargetURI += resourceName; // cntName

    return sendHttpRequest(resourceTargetURI, "riCreateSub", "application/json;ty=23");
}

std::pair<int, String> createCin(String containerResourceName, String content)
{
    JsonObject resource;
    JsonObject root;

    /* This is the payload that we need to build
    {
        {
            'm2m:ContentInstance': {
                'contentInfo': 'text/plain:0',      # Media type of the content
                'content': 'Hello, World!'          # The content itself
            }
        }

    */
    oneM2Mresource.clear();
    root = oneM2Mresource.to<JsonObject>(); // Create an empty object
    resource = root.createNestedObject("m2m:cin");

    resource["cnf"] = "text/plain:0";
    resource["con"] = content;

    // build the TO field -> serverAddress/cseBasename/aeName/cntName
    String resourceTargetURI = serverName; // serverAddress/cseBasename
    resourceTargetURI += "/";
    resourceTargetURI += originator; // aeName
    resourceTargetURI += "/";
    resourceTargetURI += containerResourceName; // cntName

    return sendHttpRequest(resourceTargetURI, "riCreateCin", "application/json;ty=4");
}

bool createContentInstance(String containerResourceName, String content)
{
    auto result = createCin(containerResourceName, content);
    if (result.first == 409)
    {
        String errorMsg = "Resource already exists: " + result.second;
        Serial.println(errorMsg);
        //  unlikey response;
        return false;
    }
    else if (result.first == 201)
    {
        // Most likely
        return true;
    }
    else
    {
        // error scenario
        // perhaps schedule a retry
        String errorMsg = "Error creating resource: " + result.second;
        Serial.println(errorMsg);
        return false;
    }
}

bool subscribeToContainer(String containerResourceName)
{
    auto result = createSub(containerResourceName);
    if (result.first == 403)
    {
        // most likey response; occurs on each restart
        return true;
    }
    else if (result.first == 201)
    {
        // On first registration
        return true;
    }
    else
    {
        // error scenario
        // perhaps schedule a retry
        return false;
    }
}

// This function parses the notification payload. We are using only JSON payloads.
// The con attribute of the notification will have the value of 1 or 0.
// Any other value will be ignored.
bool parseNotification(uint8_t *datas)
{
    recievedResDoc.clear();
    deserializeJson(recievedResDoc, datas);
    String content = recievedResDoc["m2m:sgn"]["nev"]["rep"]["m2m:cin"]["con"];
    if (content == "1")
    {
        enableLed();
    }
    else if (content == "0")
    {
        disableLed();
    }
    else
    {
        // do nothing
    }
    return 1;
}