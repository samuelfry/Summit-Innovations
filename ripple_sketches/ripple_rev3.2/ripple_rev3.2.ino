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
which I am already using. The void loop function will not be used, just the tasks.  
*/

void record_task(void* param) {
  int mil;
  while(true) {
    mil = millis();
    record();
    // Serial.println("Finished recording");
    // Wait for audio buffer to be free. At the beginning, it should automatically be free
    xQueueReceive(free_queue, &audio_buffer, portMAX_DELAY);
    // Serial.println("Received freed audio_buffer");
    amp_cov(raw_buffer, audio_buffer);
    // Send filled and converted audio buffer to upload task
    xQueueSend(send_queue, &audio_buffer, portMAX_DELAY);
    // Reviews memory left in task, increase or decrease memory task from this statement
    // Serial.print("Record task stack remaining: ");
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    Serial.print("Record task completed in: ");
    Serial.println(millis()-mil);
  }
}

void send_task(void* param) {
  int num = 0;
  int response;
  int mil;
  while(true) {
    mil = millis();
    xQueueReceive(send_queue, &audio_buffer, portMAX_DELAY);
    // Serial.println("Received filled audio_buffer");
    response = upload_audio(home, url+audio_ext+((String) (num%2))+".wav", audio_buffer, FULL_CHUNK_BYTES, access_key);
    Serial.print("Supabase response: ");
    Serial.println(response);
    xQueueSend(free_queue, &audio_buffer, portMAX_DELAY);
    num++;
    // Serial.print("Send task stack remaining: ");
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    Serial.print("Send task completed in: ");
    Serial.println(millis()-mil);
  }
}

void queue_setup() {
  send_queue = xQueueCreate(1, sizeof(uint8_t*)); // Sending audio buffer after recording and amplifying
  free_queue = xQueueCreate(1, sizeof(uint8_t*)); // Releasing audio buffer after uploading
  xQueueSend(free_queue, &audio_buffer, 0);
  xTaskCreatePinnedToCore(record_task, "Record", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(send_task, "Send", 8192, NULL, 1, NULL, 1);
}

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  // Defined in thingProperties.h
  initProperties();

  queue_setup();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}