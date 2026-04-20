#include "thingProperties.h"

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  // ArduinoCloud.begin(ArduinoIoTPreferredConnection,false);
  
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information youâll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  // setDebugMessageLevel(2);
  // ArduinoCloud.printDebugInfo();
}

void loop() {
  // ArduinoCloud.update();
  Serial.print(millis());
  Serial.print(" ");
  Serial.println(current_state);
  // Your code here
  current_state = fsm(BUTTON_PIN, current_state);
  if ((current_state != past_state) && (current_state == UP || current_state == DOWN)){
    digitalWrite(BUZZER_PIN, current_state);
    digitalWrite(LED_PIN, current_state);
    Serial.println("Inside digital write");
    Serial.print(millis());
    Serial.print(" ");
    Serial.println(current_state);
    past_state = current_state;
  }
}
