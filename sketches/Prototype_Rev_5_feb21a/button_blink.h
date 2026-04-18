#define BUTTON_PIN 7
#define BUZZER_PIN 5
#define LED_PIN 6

typedef enum {UP=0, DOWN, PRESS, RELEASE} ButtonState;

ButtonState current_state = UP;
ButtonState past_state = UP;

volatile bool buttonChanged = false;

ButtonState fsm(int i_pin, ButtonState state_current);

// void buttonInt() {
//   current_state = fsm(BUTTON_PIN, current_state);
//   if ((current_state != past_state) && (current_state == UP || current_state == DOWN)){
//     digitalWrite(BUZZER_PIN, current_state);
//     Serial.println("Inside digital write");
//     Serial.print(millis());
//     Serial.print(" ");
//     Serial.println(current_state);
//     past_state = current_state;
//   }
//   buttonChanged = false;
// }

void initProperties(){

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInt, CHANGE);

}

ButtonState fsm(int i_pin, ButtonState state_current){
  //Look for button press
  const static unsigned long debounce_t = 50;
  static unsigned long last_bounce = millis();
  bool reading = digitalRead(i_pin);
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

void onButtonControlChange()  {
  // Add your code here to act upon BuzzerControl change
  digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
  digitalWrite(LED_PIN, HIGH);
  delay(500); // Pause for a second 
  digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
  digitalWrite(LED_PIN,LOW);
  delay(250); // Pause for a second 
  digitalWrite(BUZZER_PIN, HIGH); // Turn ON buzzer
  digitalWrite(LED_PIN, HIGH);
  delay(500); // Pause for a second 
  digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
  digitalWrite(LED_PIN,LOW);

}