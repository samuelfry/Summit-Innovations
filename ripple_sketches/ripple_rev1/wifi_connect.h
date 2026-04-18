#pragma once

#include <WiFi.h>
#include <HTTPClient.h>

int status = WL_IDLE_STATUS;

int reconnect(const char* ssid, const char* password) {
  return WiFi.begin(ssid, password);
}

int send_mes(HTTPClient& database, const String& path, const char* header_type, const char* header, const String& mes){
  database.begin(path);
  database.addHeader(header_type, header);
  int code = database.PUT(mes);
  database.end();
  return code;
}

bool listen(HTTPClient& database, const String& path){
  //Find path name
  database.begin(path);
  int app_response = database.GET();
  // Serial.print("PingFromApp response: ");
  // Serial.println(httpCode);
  String payload;
  if (app_response) {
    payload = database.getString();
    // Serial.print("Payload: ");
    // Serial.println(payload);
  }
  database.end();
  Serial.print("PingFromApp response: ");
  Serial.println(payload); 
  //Sense if endpoint has change
  if (payload == "\"pressed\"") {
    return true;
  }
  else {
    return false;
  }
}