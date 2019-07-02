#ifndef __SINRICPRO_H__
#define __SINRICPRO_H__

#include <WebSocketsClient.h>

#include <ArduinoJson.h>
#include <vector>

#include <SinricProDebug.h>
#include <SinricProConfig.h>
#include "Devices/SinricProDevice.h"
#include "Communication/SinricProQueue.h"
#include "Communication/SinricProWebsocket.h"
#include "Request/SinricProRequest.h"
#include "Request/JsonRequests.h"
#include "Events/SinricProEvent.h"

class SinricPro {
  public:
    SinricPro();
    ~SinricPro();

    void begin(const char* api_key);
    void begin(const String& api_key);

    void handle();
    void handleRequest();
    void handleResponse();

    void stop();
    bool isConnected();

    SinricProDevice& add(const char* deviceId);
    SinricProDevice& add(const String& deviceId);

    boolean remove(const char* deviceId);
    boolean remove(const String& deviceId);

    unsigned long getTimestamp() { return _baseTS + (millis() / 1000); }
    void syncTimestamp(unsigned long ts) { _baseTS = ts-(millis()/1000); }

    void raiseEvent(SinricProEvent& event);
    SinricProDevice& operator[] (int index)  { return *devices[index]; }
    SinricProDevice* operator[] (const char* deviceId);
    std::vector<SinricProDevice*>::iterator begin() { return devices.begin(); }
    std::vector<SinricProDevice*>::iterator end() { return devices.end(); }
    int size() { return devices.size(); }

  private:
    const char* _api_key;

    websocketListener _websocketListener;
    std::vector<SinricProDevice*> devices;
    unsigned long _baseTS;
};

SinricPro::SinricPro() {};

SinricPro::~SinricPro() {
  stop();

  for (auto& device : devices) {
    delete device;
    device = nullptr;
  }
  devices.clear();

  if (_api_key) delete _api_key;
}

void SinricPro::begin(const char* api_key) {
  _api_key = strdup(api_key);

  String deviceIds;
  for (auto& device : devices) {
    deviceIds += String(device->getDeviceId()) + ";";
  }
  _websocketListener.begin(api_key, deviceIds.c_str());
}

void SinricPro::begin(const String& api_key) {
  begin(api_key.c_str());
}

void SinricPro::handle() {
  _websocketListener.handle();
  handleRequest();
}

void prepareResponse(const JsonDocument& jsonRequest, JsonDocument& jsonResponse, unsigned long ts) {
  jsonResponse["payloadVersion"] = 1;
  jsonResponse["success"] = false;
  jsonResponse["message"] = "OK";
  jsonResponse["createdAt"] = ts;
  jsonResponse["deviceId"] = jsonRequest["deviceId"];
  jsonResponse["type"] = "response";
  jsonResponse["action"] = jsonRequest["action"];
  jsonResponse.createNestedObject("value");
//  jsonResponse["value"] = jsonRequest["value"];
}

void SinricPro::handleRequest() {

  if (requestQueue.count() > 0) {
    DEBUG_SINRIC("[SinricPro.handleRequest()]: %i requests in queue\r\n", requestQueue.count());
    // POP requests and call device.handle() for each related device
    while (requestQueue.count() > 0) {
      SinricProRequestPayload* requestPayload = requestQueue.pop();
      DynamicJsonDocument jsonRequest(512);
      DeserializationError err = deserializeJson(jsonRequest, requestPayload->getRequest());

      if (err) {
        DEBUG_SINRIC("[SinricPro.handleRequest()]: Error (%s) while parsing json request\r\n", err.c_str());
        return;
      }

      int payloadVersion = jsonRequest["payloadVersion"]; // 1
      const char* clientId = jsonRequest["clientId"]; // "alexa-skill"
      long createdAt = jsonRequest["createdAt"]; // 1562001822
      const char* deviceId = jsonRequest["deviceId"]; // "5d12df23eb7e894a699e0ae8"
      const char* type = jsonRequest["type"]; // "request"
      const char* action = jsonRequest["action"]; // "setPowerState"
      const char* value_state = jsonRequest["value"]["state"]; // "On"

      syncTimestamp(createdAt);

      for (auto& device : devices) {
        if (strcmp(device->getDeviceId(), jsonRequest["deviceId"]) == 0) {
            DynamicJsonDocument jsonResponse(512);
            prepareResponse(jsonRequest, jsonResponse, getTimestamp());

            jsonResponse["success"] = device->handle(jsonRequest, jsonResponse);

            String responseString; 
            serializeJsonPretty(jsonResponse, responseString);
            DEBUG_SINRIC("Response: %s\r\n", responseString.c_str());

            switch (requestPayload->getRequestSource()) {
              case CS_WEBSOCKET:
                _websocketListener.sendResponse(responseString);
                break;
              default:
                break;
          }
        }
      }
      delete requestPayload;
    }
  }
}

void SinricPro::stop() {
  _websocketListener.stop();
}

bool SinricPro::isConnected() {
  return _websocketListener.isConnected();
};


SinricProDevice& SinricPro::add(const char* deviceId) {
  DEBUG_SINRIC("[SinricPro]: Add device %s\r\n", deviceId);

  SinricProDevice* newDevice = new SinricProDevice(deviceId);
  devices.push_back(newDevice);
  return *newDevice;
}

SinricProDevice& SinricPro::add(const String& deviceId) {
  return add(deviceId.c_str());
}

boolean SinricPro::remove(const char* deviceId) {
  boolean success = false;

// To do: Change this for removing device...
  for (auto& device : devices) {
    if (strcmp(device->getDeviceId(), deviceId) == 0) {
      DEBUG_SINRIC("[SinricPro:remove]: Removing device %s\r\n", deviceId);
      success = true;
    }
  }
// To do: end of To do
  if (!success) DEBUG_SINRIC("[SinricPro:remove]: No corresponding device found (id:%s)\r\n", deviceId);
  return success;
}

boolean SinricPro::remove(const String& deviceId) {
  return remove(deviceId.c_str());
}

void SinricPro::raiseEvent(SinricProEvent& event) {
  event.setTS(getTimestamp());
  String tmpString = event.getJsonEventString();
  DEBUG_SINRIC("[SinricPro:raiseEvent]: \r\n%s\r\n", tmpString.c_str());
  _websocketListener.sendEvent(tmpString);
}

SinricProDevice* SinricPro::operator[] (const char* deviceId) {
  for (auto& device : devices) {
    if (strcmp(device->getDeviceId(), deviceId) == 0) return device;
  }
  return nullptr;
}


#ifndef SINRIC_NOINSTANCE
SinricPro SinricPro;
#endif

#endif
