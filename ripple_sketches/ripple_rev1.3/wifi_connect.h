#pragma once

#include <WiFi.h>
#include <HTTPClient.h>

int status = WL_IDLE_STATUS;

int reconnect(const char* ssid, const char* password) {
  return WiFi.begin(ssid, password);
}

int send_mes(HTTPClient& database, const String& path, const char* header_type, const char* header, const String& mes){
  database.begin(path);
  database.addHeader(header_type, header);
  int code = database.PUT(mes);
  database.end();
  return code;
}

bool listen(HTTPClient& database, const String& path){
  //Find path name
  database.begin(path);
  int app_response = database.GET();
  // Serial.print("PingFromApp response: ");
  // Serial.println(httpCode);
  String payload;
  if (app_response) {
    payload = database.getString();
    // Serial.print("Payload: ");
    // Serial.println(payload);
  }
  database.end();
  Serial.print("PingFromApp response: ");
  Serial.println(payload); 
  //Sense if endpoint has change
  if (payload == "\"pressed\"") {
    return true;
  }
  else {
    return false;
  }
}

void writeWavHeader(uint8_t* header, uint32_t dataSize, uint32_t sampleRate) {
  uint32_t fileSize = dataSize + 36;        // Total file size - 8
  uint16_t bitsPerSample = 16;
  uint16_t numChannels = 1;
  uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
  uint16_t blockAlign = numChannels * (bitsPerSample / 8);
  
  // RIFF header (12 bytes)
  header[0]  = 'R'; header[1]  = 'I'; header[2]  = 'F'; header[3]  = 'F';
  header[4]  = (fileSize & 0xff);       header[5]  = ((fileSize >>  8) & 0xff);
  header[6]  = ((fileSize >> 16) & 0xff); header[7]  = ((fileSize >> 24) & 0xff);
  header[8]  = 'W'; header[9]  = 'A'; header[10] = 'V'; header[11] = 'E';
  
  // fmt chunk (24 bytes)
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1;  header[21] = 0;                    // PCM format
  header[22] = numChannels; header[23] = 0;
  header[24] = (sampleRate & 0xff);       header[25] = ((sampleRate >>  8) & 0xff);
  header[26] = ((sampleRate >> 16) & 0xff); header[27] = ((sampleRate >> 24) & 0xff);
  header[28] = (byteRate & 0xff);         header[29] = ((byteRate >>  8) & 0xff);
  header[30] = ((byteRate >> 16) & 0xff); header[31] = ((byteRate >> 24) & 0xff);
  header[32] = blockAlign; header[33] = 0;
  header[34] = bitsPerSample; header[35] = 0;
  
  // Data chunk header (8 bytes)
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (dataSize & 0xff);       header[41] = ((dataSize >>  8) & 0xff);
  header[42] = ((dataSize >> 16) & 0xff); header[43] = ((dataSize >> 24) & 0xff);
}

int upload_audio(HTTPClient& database, const String& path, int16_t* buffer, size_t size, const String access_key){
  // Serial.print("Entered Audio Upload");
  // Serial.print("Database file path: ");
  // Serial.println(path);
  uint8_t header[44];
  writeWavHeader(header, size, 16000);
  database.begin(path);
  database.addHeader("Content-Type", "audio/wav");
  database.addHeader("Authorization", "Bearer " + access_key);
  database.addHeader("apikey", access_key);
  // database.addHeader("X-Upsert", "true");
  uint8_t* fullfile = (uint8_t*)malloc(44+size);
  memcpy(fullfile, header, 44);
  memcpy(fullfile+44, buffer, size);
  int code = database.POST(fullfile, size+44);
  // Serial.print("Database response: ");
  // Serial.println(code);
  // delay(1000);
  free(fullfile);
  database.end();
  // if (code != 200) {
  //   Serial.printf("HTTP Response %d\n", code);
  //   String response = database.getString();
  //   Serial.printf("Error details: %s\n", response.c_str());
  // }
  return code;
}