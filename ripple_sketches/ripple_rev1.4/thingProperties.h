#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include "wifi_connect.h"
// #include <ESP_I2S.h>
#include "config.h"

//Pin definitions
#define BUTTON_PIN 43
#define I2S_WS 8
#define I2S_SCK 7
#define I2S_SD 9

//Mic recording definitions
#define V_REF 3.3
#define SAMPLE_RATE 16000 //Samples per second
#define BYTES_PER_RAW_SAMPLE 4
#define BYTES_PER_FULL_SAMPLE 3
#define CHUNK_SECONDS 2
#define SAMPLES_PER_CHUNK (SAMPLE_RATE*CHUNK_SECONDS)
#define RAW_CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_RAW_SAMPLE)
#define FULL_CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_FULL_SAMPLE)
#define VOLUME_GAIN 0

//Button states
typedef enum {UP=0, DOWN, PRESS, RELEASE} ButtonState;

//Mic states
typedef enum {IDLE, RECORDING, UPLOADING} MicState;

//Wifi/Database keys
const char* ssid = "SamuelF"; //TP-LINK_AB77 //BYU-WiFi //SamuelF
const char* password = "samb@r@y"; //21940521 //samb@r@y
const String url = "https://gctbnsjsridmsilzqtpq.storage.supabase.co/storage/v1/object";
const String audio_ext = "/audio/public/file_";
const String access_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImdjdGJuc2pzcmlkbXNpbHpxdHBxIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzUwNjM0ODksImV4cCI6MjA5MDYzOTQ4OX0.T7OdeAuH0AM0a8YdzWAcwH1e6rkGFaL4stL4DWttbZg";

ButtonState current_state = UP;
ButtonState past_state = UP;

MicState audio_state = IDLE;

HTTPClient home;

// I2SClass I2S;

int32_t* raw_buffer = NULL;
uint8_t * audio_buffer = NULL;


void initProperties(){
  //Configure button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  //Configure I2S protocol
  config_i2s();
  // I2S.setPins(I2S_SCK, I2S_WS, -1, I2S_SD);
  // while (!I2S.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO)) {
  //   Serial.println("Failed to initialize I2S!");
  //   delay (100); // do nothing
  // }

  //Create PSRAM buffer
  audio_buffer = (uint8_t*)ps_malloc(FULL_CHUNK_BYTES);
  raw_buffer = (int32_t *)ps_malloc(RAW_CHUNK_BYTES);
  if (!audio_buffer && !raw_buffer) {
    Serial.println("PSRAM allocation failed");
    while (1);
  }

  //Connect to wifi
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

  //Set up database socket
  home.setReuse(true);
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

void amp_cov(int32_t* r_buffer, uint8_t* a_buffer) {
  int32_t raw32;
  for(size_t i = 0; i < SAMPLES_PER_CHUNK; i++) {
    // Serial.print("Raw 32 bit integer: ");
    // Serial.print(r_buffer[i], HEX);
    // Serial.println();
    raw32 = r_buffer[i] >> 8;
    raw32 <<= VOLUME_GAIN;
    a_buffer[(i*3)] = (uint8_t)((int32_t)raw32 & 0xFF);
    a_buffer[(i*3)+1] = (uint8_t)(((int32_t)raw32 >> 8) & 0xFF);
    a_buffer[(i*3)+2] = (uint8_t)(((int32_t)raw32 >> 16) & 0xFF);     
    // Serial.print("Full 24 bit integer: ");
    // Serial.print(a_buffer[(i*3)], HEX);
    // Serial.print(a_buffer[(i*3)+1], HEX);
    // Serial.print(a_buffer[(i*3)+2], HEX);
    // Serial.println();
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
  static int name = 0;
  // switch(audio_state){
  //   case IDLE:
  //     response = -1;
  //     if(current_state == DOWN) {
  //       audio_state = RECORDING;
  //       bytes_collected = 0;
  //     }
  //     break;
  //   case RECORDING:
      //Actions to take while in state (recording and storing buffer)
      // Serial.println("Entered Recording state");
      // while (bytes_collected < RAW_CHUNK_BYTES) {
      //   bytes_read = I2S.readBytes((char*)raw_buffer + bytes_collected, RAW_CHUNK_BYTES - bytes_collected);
      //   bytes_collected += bytes_read;
        // Serial.print("Bytes collected this recording: ");
        // Serial.println(bytes_collected);
      // }
  i2s_read(I2S_PORT, raw_buffer, RAW_CHUNK_BYTES, &bytes_read, portMAX_DELAY);
      // Serial.print("Upload time (ms): ");
      // Serial.println(upload_time);
      // Serial.print("Chunk limit: ");
      // Serial.println(CHUNK_BYTES);
  // audio_state = UPLOADING;
    //   break;
    // case UPLOADING:
      //upload payload
      // Serial.print("This is the current_state: ");
      // Serial.println(current_state);
      // Serial.println("Entered upload if statement ");
  amp_cov(raw_buffer, audio_buffer);
  obj_key =   "0.wav"; //(String)(name%2)+
  response = upload_audio(home, url+audio_ext+obj_key, audio_buffer, FULL_CHUNK_BYTES, access_key);
  name++;
  // audio_state = IDLE;
  //     break;
  //   default:
  //     audio_state = IDLE;
  //     break;
  // }

  return response;
  
}



