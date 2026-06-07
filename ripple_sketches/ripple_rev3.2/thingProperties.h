#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include "wifi_connect.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

//Mic recording definitions
#define V_REF 3.3
#define SAMPLE_RATE 16000 //Samples per second
#define BYTES_PER_RAW_SAMPLE 4
#define BYTES_PER_FULL_SAMPLE 3
#define CHUNK_SECONDS 0.5
#define SAMPLES_PER_CHUNK (SAMPLE_RATE*CHUNK_SECONDS)
#define RAW_CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_RAW_SAMPLE)
#define FULL_CHUNK_BYTES (SAMPLES_PER_CHUNK * BYTES_PER_FULL_SAMPLE)
#define VOLUME_GAIN 0

//Wifi/Database keys
const char* ssid = "TP-LINK_AB77"; //TP-LINK_AB77 //BYU-WiFi //SamuelF
const char* password = "21940521"; //21940521 //samb@r@y
const String url = "https://gctbnsjsridmsilzqtpq.storage.supabase.co/storage/v1/object";
const String audio_ext = "/audio/public/file_";
const String access_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImdjdGJuc2pzcmlkbXNpbHpxdHBxIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzUwNjM0ODksImV4cCI6MjA5MDYzOTQ4OX0.T7OdeAuH0AM0a8YdzWAcwH1e6rkGFaL4stL4DWttbZg";

HTTPClient home;

int32_t* raw_buffer = NULL;
uint8_t * audio_buffer = NULL;


void initProperties(){
  //Configure button
  
  //Configure I2S protocol
  config_i2s();
  // I2S.setPins(I2S_SCK, I2S_WS, -1, I2S_SD);
  // while (!I2S.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO)) {
  //   Serial.println("Failed to initialize I2S!");
  //   delay (100); // do nothing
  // }

  //Create PSRAM buffer
  Serial.println((int) FULL_CHUNK_BYTES);
  Serial.println((int) RAW_CHUNK_BYTES);
  audio_buffer = (uint8_t*)ps_malloc((int) FULL_CHUNK_BYTES);
  raw_buffer = (int32_t *)ps_malloc((int) RAW_CHUNK_BYTES);
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

int fsmrecordAndUpload() {
  static int response = -1;
  String obj_key;
  size_t bytes_read = 0;
  size_t bytes_collected = 0;
  while (bytes_collected < RAW_CHUNK_BYTES) {
    i2s_read(I2S_PORT, raw_buffer, RAW_CHUNK_BYTES, &bytes_read, portMAX_DELAY);
    bytes_collected += bytes_read;
  }
  amp_cov(raw_buffer, audio_buffer);
  obj_key =   "0.wav"; //(String)(name%2)+
  response = upload_audio(home, url+audio_ext+obj_key, audio_buffer, FULL_CHUNK_BYTES, access_key);
  return response;
  
}

