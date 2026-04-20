
/*Suumit Innovations RippleGlasses*/

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
  // float mic_v = analog_voltage(MIC_PIN);
  // Serial.println(mic_v);
  
  current_state = fsmButton(BUTTON_PIN, current_state);

  // if (sample && sample != -1 && sample != 1) {
  //   Serial.println(sample);
  // }
  // Serial.print(millis());
  // Serial.print(" ");
  // Serial.println(current_state);
  // Serial.println(past_state);
  //recording fsm (contains uploading logic)
  int response = fsmrecordAndUpload(current_state);
  if (response != -1) {
    Serial.print("Supabase response: ");
    Serial.println(response);
  }
  past_state = current_state;
}

