#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include "wifi_connect.h"

#define MIC_PIN 2

#define V_REF 3.3

// typedef enum {UP=0, DOWN, PRESS, RELEASE} ButtonState;
// typedef enum {S1=0, S2, S3} AppState;

const char* ssid = "TP-LINK_AB77";
const char* password = "21940521";
// const String url = "https://flamingjitterbug-ae3ab-default-rtdb.firebaseio.com/";
// const String ext = "devices/xiao01/";

// ButtonState current_state = UP;
// ButtonState past_state = UP;

// HTTPClient home;

// const char DEVICE_LOGIN_NAME[]  = "3dabd68e-de6b-4b71-980b-18357ee4a025";

// const char SSID[]               = SECRET_SSID;    // Network SSID (name)
// const char PASS[]               = SECRET_OPTIONAL_PASS;    // Network password (use for WPA, or use as key for WEP)
// const char DEVICE_KEY[]  = SECRET_DEVICE_KEY;    // Secret device password



void initProperties(){

  pinMode(MIC_PIN, INPUT);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // home.setReuse(true);
  // String package = "{\"status\":\"online\"}";
  // Serial.println("Connecting to database...");
  // Serial.println("Status: ");
  // Serial.print(send_mes(home, url+ext+"online.json", "Content-Type", "application/json", package));
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInt, CHANGE);
}

// ButtonState fsmButton(int i_pin, ButtonState state_current){
//   //Look for button press
//   const static unsigned long debounce_t = 50;
//   static unsigned long last_bounce = millis();
//   bool reading = digitalRead(i_pin);
//   static bool last_reading = LOW;
//   // Guard rails on button press
//   if (reading!=last_reading) {
//     last_bounce = millis();
//   }

//   if (millis() - last_bounce >= debounce_t){
//     switch(state_current){
//       case UP:
//         if (reading == DOWN) {state_current = PRESS;}
//         break;
//       case DOWN:
//         if (reading == UP) {state_current = RELEASE;}
//         break;
//       case PRESS:
//         state_current = DOWN;
//         break;
//       case RELEASE:
//         state_current = UP;
//         break;
//     } 
//   }
//   last_reading = reading;
//   return state_current;
// }

// void fsmAppControl(HTTPClient& http, const String& listen_path, const ButtonState& button_state, const String& response_path){
//   /* 3 states. 
//   S1, the buzzer is off, the PingFromApp is unpressed, and the buzzer_response is off.
//   Transition S1->S2, PingFromApp goes to pressed. 
//   S2, the buzzer is on, the PingFromApp is pressed, and the buzzer_response is on.
//   Transition S2->S3, button_state is down.
//   S3, the buzzer is off, the PingFromApp is pressed, and the buzzer_response is off.
//   Transition S3->S1, PingFromApp goes to unpressed.*/
//   bool ping = listen(http, listen_path);
//   static AppState app_state = S1;
//   static long counter = 0;
//   Serial.print("App State: ");
//   Serial.println(app_state);

//   switch (app_state) {
//     case S1:
//       if (!counter) {
//         digitalWrite(BUZZER_PIN, LOW);
//         digitalWrite(LED_PIN, LOW);
//         send_mes(home, response_path, "Content-Type", "application/json", "{\"buzzer_response\":\"OFF\"}");
//       }
//       counter++;
//       if (ping) {
//         app_state = S2;
//         counter=0;
//       }
//       break;
//     case S2:
//       if (!counter) {
//         digitalWrite(BUZZER_PIN, HIGH);
//         digitalWrite(LED_PIN, HIGH);
//         send_mes(home, response_path, "Content-Type", "application/json", "{\"buzzer_response\":\"ON\"}");
//       }
//       counter++;
//       if (button_state == DOWN) {
//         app_state = S3;
//         counter=0;
//       }
//       break;
//     case S3:
//       if (!counter) {
//         digitalWrite(BUZZER_PIN, LOW);
//         digitalWrite(LED_PIN, LOW);
//         send_mes(home, response_path, "Content-Type", "application/json", "{\"buzzer_response\":\"OFF\"}");
//       }
//       counter++;
//       if (!ping) {
//         app_state = S1;
//         counter=0;
//       }
//       break;
//     default:
//       send_mes(home, response_path, "Content-Type", "application/json", "{\"buzzer_response\":\"ERROR\"}");
//       app_state = S1;
//       break;
//   }
// }

float analog_voltage(int pin){
  int voltage = analogRead(pin);
  // Serial.println(voltage);
  return (float) (V_REF*((float) voltage/4096));
}

