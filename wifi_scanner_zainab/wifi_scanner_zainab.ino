/*--------------------------------------------------------------------------------------
 * CODE TO TEST ALL BUTTONS FOR INPUT
 * We will use a state machine not interrupt
/*-------------------------------------------------------------------------------------*/

// Button pin definitions
#define BTN_UP      12
#define BTN_DOWN    13
#define BTN_LEFT    14
#define BTN_RIGHT   16
#define BTN_SELECT  17
#define BTN_BACK    18

#define DEBOUNCE_DELAY 20

int prevState[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
const int buttonPins[6] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_SELECT, BTN_BACK};
const char* buttonNames[6] = {"UP", "DOWN", "LEFT", "RIGHT", "SELECT", "BACK"};

void setup() {
  Serial.begin(115200);

  // Internal pullup: pin rests HIGH, goes LOW when button is pressed
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  Serial.println("Button monitor ready...");
}

void loop() {
  for (int i = 0; i < 6; i++) {
    int currentState = digitalRead(buttonPins[i]);

    // Detect falling edge (HIGH -> LOW = button pressed)
    if (currentState == LOW && prevState[i] == HIGH) {
      // Perform action.
      Serial.println(buttonNames[i]);
    }
    prevState[i] = currentState;
  }

  delay(DEBOUNCE_DELAY);
}