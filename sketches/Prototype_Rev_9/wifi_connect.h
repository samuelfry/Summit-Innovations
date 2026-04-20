#pragma once

#include <WiFi.h>
#include <HTTPClient.h>

int status = WL_IDLE_STATUS;

int reconnect(const char* ssid, const char* password) {
  return WiFi.begin(ssid, password);
}

int send_mes(const String& path, const char* header_type, const char* header, const String& mes){
  HTTPClient http;
  http.begin(path);
  http.addHeader(header_type, header);
  int code = http.PUT(mes);
  http.end();
  return code;
}

bool listen(const String& path){
  //Find path name
  HTTPClient http;
  http.begin(path);
  int httpCode = http.GET();
  // Serial.print("PingFromApp response: ");
  // Serial.println(httpCode);
  String payload;
  if (httpCode) {
    payload = http.getString();
    // Serial.print("Payload: ");
    // Serial.println(payload);
  }
  http.end();
  //Sense if endpoint has change
  if (payload == "\"pressed\"") {
    return true;
  }
  else {
    return false;
  }
}