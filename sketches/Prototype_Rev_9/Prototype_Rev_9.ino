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
  Serial.print(millis());
  Serial.print(" ");
  Serial.println(current_state);
  Serial.println(past_state);
  //Managing WiFi connection
  status = WiFi.status();
  // Serial.println("WiFi status: ");
  // Serial.println(status);
  if (status != WL_CONNECTED) {
    status = reconnect(ssid, password);
    // Serial.println("IP address: ");
    // Serial.println(WiFi.localIP());
  }
  //Listening to Database
  bool ping = listen(url+"PingFromApp.json");
  // Serial.print("App Signal: ");
  // Serial.println(ping);
  //If pressed, send digital signal that doesn't stop until button_pin is pressed
  if (ping && (current_state != DOWN)) {
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    String package = "{\"buzzer_response\":\"ON\"}";
    send_mes(url+ext+"buzzer_response.json", "Content-Type", "application/json", package);
  }
  current_state = fsm(BUTTON_PIN, current_state);
  //Only sends signal when current_state goes from down to up
  if ((current_state == DOWN) && (current_state != past_state)){
    Serial.println("Entered button down state");
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    String package = "{\"buzzer_response\":\"OFF\"}";
    send_mes(url+ext+"buzzer_response.json", "Content-Type", "application/json", package);
    // Serial.println("Inside digital write");
    // Serial.print(millis());
    // Serial.print(" ");
    // Serial.println(current_state);
  }
  past_state = current_state;
}

