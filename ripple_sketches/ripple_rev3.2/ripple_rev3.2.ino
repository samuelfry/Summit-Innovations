/*Suumit Innovations RippleGlasses*/

#pragma once

#include "thingProperties.h"
#include "wifi_connect.h"

/*
NEW TASK PROCESS
I want to use two separate tasks in order to make recording and sending more
seamless. I want to have one task that will record half-second clips and another 
that will send those clips. THIS ONLY WORKS IF THE SECOND TASK TAKES LESS THAN HALF
A SECOND TO SEND. 
I need to design each task, one using a record and amplify function (right now in
fsmRecordandUpload()) and the other using the upload function. I need two buffers, 
*/

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
  static int response;
  status = WiFi.status();
  // Serial.println("WiFi status: ");
  // Serial.println(status);
  if (status != WL_CONNECTED) {
    status = reconnect(ssid, password);
    // Serial.println("IP address: ");
    // Serial.println(WiFi.localIP());
  }
  response = fsmrecordAndUpload();
  if (response != -1) {
    Serial.print("Supabase response: ");
    Serial.println(response);
  }
}

