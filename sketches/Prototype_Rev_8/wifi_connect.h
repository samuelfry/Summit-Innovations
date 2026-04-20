#pragma once

#include <WiFi.h>
#include <HTTPClient.h>

int status = WL_IDLE_STATUS;

int reconnect(const char* ssid, const char* password) {
  return WiFi.begin(ssid, password);
}

int database_con(const String& path, const char* header_type, const char* header, const String& body){
  HTTPClient http;
  http.begin(path);
  http.addHeader(header_type, header);
  int code = http.POST(body);
  http.end();
  return code;
}
