/*Suumit Innovations JitterBug*/

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
  //Managing WiFi connection
  status = WiFi.status();
  // Serial.println("WiFi status: ");
  // Serial.println(status);
  if (status != WL_CONNECTED) {
    status = reconnect(ssid, password);
    // Serial.println("IP address: ");
    // Serial.println(WiFi.localIP());
  }
  current_state = fsmButton(BUTTON_PIN, current_state);
  Serial.print(millis());
  Serial.print(" ");
  Serial.println(current_state);
  Serial.println(past_state);
  //Finite State Machine for App Control
  fsmAppControl(home, url+"PingFromApp.json", current_state, url+ext+"buzzer_response.json");
  //Only sends signal when current_state goes from down to up
  past_state = current_state;
}


