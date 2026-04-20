#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include "wifi_connect.h"
#include <ESP_I2S.h>

#define BUTTON_PIN 43

#define V_REF 3.3
#define SAMPLE_RATE 16000 //Samples per second
#define BYTES_PER_SAMPLE 2
#define CHUNK_SECONDS 5
#define SAMPLES_PER_CHUNK (SAMPLE_RATE*CHUNK_SECONDS)
#define CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_SAMPLE)
#define VOLUME_GAIN 3

typedef enum {UP=0, DOWN, PRESS, RELEASE} ButtonState;
typedef enum {IDLE, RECORDING, UPLOADING} MicState;

const char* ssid = "BYU-WiFi"; //TP-LINK_AB77 //BYU-WiFi
const char* password = ""; //21940521
const String url = "https://gctbnsjsridmsilzqtpq.storage.supabase.co/storage/v1/object";
const String audio_ext = "/audio/public/esp32c3";

ButtonState current_state = UP;
ButtonState past_state = UP;

MicState audio_state = IDLE;

HTTPClient home;

const String access_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImdjdGJuc2pzcmlkbXNpbHpxdHBxIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzUwNjM0ODksImV4cCI6MjA5MDYzOTQ4OX0.T7OdeAuH0AM0a8YdzWAcwH1e6rkGFaL4stL4DWttbZg";

I2SClass I2S;

int16_t* audio_buffer = NULL;


void initProperties(){

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  I2S.setPinsPdmRx(42, 41);
  while (!I2S.begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("Failed to initialize I2S!");
    delay (100); // do nothing
  }

  audio_buffer = (int16_t*)ps_malloc(CHUNK_BYTES);
  if (!audio_buffer) {
    Serial.println("PSRAM allocation failed");
    while (1);
  }

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  home.setReuse(true);
  // String package = "{\"status\":\"online\"}";
  // Serial.println("Connecting to database...");
  // Serial.println("Status: ");
  // Serial.print(send_mes(home, url+ext+"online.json", "Content-Type", "application/json", package));
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInt, CHANGE);
}

ButtonState fsmButton(int i_pin, ButtonState state_current){
  //Look for button press
  const static unsigned long debounce_t = 50;
  static unsigned long last_bounce = millis();
  bool reading = !digitalRead(i_pin);
  // Serial.println(reading);
  static bool last_reading = LOW;
  // Guard rails on button press
  if (reading!=last_reading) {
    last_bounce = millis();
  }

  if (millis() - last_bounce >= debounce_t){
    switch(state_current){
      case UP:
        if (reading == DOWN) {state_current = PRESS;}
        break;
      case DOWN:
        if (reading == UP) {state_current = RELEASE;}
        break;
      case PRESS:
        state_current = DOWN;
        break;
      case RELEASE:
        state_current = UP;
        break;
    } 
  }
  last_reading = reading;
  return state_current;
}

void amplify_volume(int16_t* buffer) {
  for(size_t i = 0; i < SAMPLES_PER_CHUNK; i ++) {
    buffer[i] <<= VOLUME_GAIN;
  }
}

int fsmrecordAndUpload(ButtonState current_state) {
  /*Three States:
    -Idle: Not recording
    -Recording: collecting two second samples
    -Uploading: This is the uploading portion
    If at any point, the button goes to up, the recording stops */
  static size_t bytes_collected = 0;
  static int response = -1;
  static String obj_key;
  static size_t bytes_read;
  switch(audio_state){
    case IDLE:
      response = -1;
      if(current_state == DOWN) {
        audio_state = RECORDING;
        bytes_collected = 0;
      }
      break;
    case RECORDING:
      //Actions to take while in state (recording and storing buffer)
      // Serial.println("Entered Recording state");
      while (bytes_collected < CHUNK_BYTES) {
        bytes_read = I2S.readBytes((char*)audio_buffer + bytes_collected, CHUNK_BYTES - bytes_collected);
        bytes_collected += bytes_read;
        // Serial.print("Bytes collected this recording: ");
        // Serial.println(bytes_collected);
      }
      // Serial.print("Upload time (ms): ");
      // Serial.println(upload_time);
      // Serial.print("Chunk limit: ");
      // Serial.println(CHUNK_BYTES);
      audio_state = UPLOADING;
      break;
    case UPLOADING:
      //upload payload
      // Serial.print("This is the current_state: ");
      // Serial.println(current_state);
      // Serial.println("Entered upload if statement ");
      // amplify_volume(audio_buffer);
      obj_key = (String)(millis()) + ".wav";
      response = upload_audio(home, url+audio_ext+obj_key, audio_buffer, CHUNK_BYTES, access_key);
      audio_state = IDLE;
      break;
    default:
      audio_state = IDLE;
      break;
  }

  return response;
  
}



