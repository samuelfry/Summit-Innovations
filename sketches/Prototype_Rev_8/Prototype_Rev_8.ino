#pragma once

#include "thingProperties.h"
#include "wifi_connect.h"

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  // Defined in thingProperties.h
  initProperties();
  // We start by connecting to a WiFi network


}

void loop() {
  // Serial.print(millis());
  // Serial.print(" ");
  // Serial.println(current_state);
  status = WiFi.status();
  Serial.println("WiFi status: ");
  Serial.println(status);
  if (status != WL_CONNECTED) {
    status = reconnect(ssid, password);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  // Your code here
  current_state = fsm(BUTTON_PIN, current_state);
  if ((current_state != past_state) && (current_state == UP || current_state == DOWN)){
    digitalWrite(BUZZER_PIN, current_state);
    digitalWrite(LED_PIN, current_state);
    // Serial.println("Inside digital write");
    // Serial.print(millis());
    // Serial.print(" ");
    // Serial.println(current_state);
    past_state = current_state;
  }
}
