#define BUTTON_PIN 4
#define BUZZER_PIN 6
#define LED_PIN 20

typedef enum {UP=0, DOWN, PRESS, RELEASE} ButtonState;

ButtonState current_state = UP;
ButtonState past_state = UP;

// const char DEVICE_LOGIN_NAME[]  = "3dabd68e-de6b-4b71-980b-18357ee4a025";

// const char SSID[]               = SECRET_SSID;    // Network SSID (name)
// const char PASS[]               = SECRET_OPTIONAL_PASS;    // Network password (use for WPA, or use as key for WEP)
// const char DEVICE_KEY[]  = SECRET_DEVICE_KEY;    // Secret device password



void initProperties(){

  // ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  // ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInt, CHANGE);

}

// WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

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
