<<<<<<< HEAD
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
  // wifi_scan();
    
  // current_state = fsmButton(BUTTON_PIN, current_state);

  // int32_t sample = I2S.read();
  // Serial.println(sample);
  
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


=======
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
    
  current_state = fsmButton(BUTTON_PIN, current_state);

  // int32_t sample = I2S.read();
  // Serial.println(sample);
  
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


>>>>>>> 70987b2780577ce3cfd13da38e0b4a7ef04ad0c5
