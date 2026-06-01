// used pins
// #define TFT_CS   33
// #define TFT_DC   15
// #define TFT_RST  32

// #define TFT_WR    4
// #define TFT_RD    2

// #define TFT_D0   12
// #define TFT_D1   13
// #define TFT_D2   26
// #define TFT_D3   25
// #define TFT_D4   19
// #define TFT_D5   18
// #define TFT_D6   27
// #define TFT_D7   14


// #define DEBOUNCE_DELAY 20

// int prevState[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
// const int buttonPins[6] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_SELECT, BTN_BACK};
// const char* buttonNames[6] = {"UP", "DOWN", "LEFT", "RIGHT", "SELECT", "BACK"};

// void setup() {
//   Serial.begin(115200);

//   // Internal pullup: pin rests HIGH, goes LOW when button is pressed
//   for (int i = 0; i < 6; i++) {
//     pinMode(buttonPins[i], INPUT_PULLUP);
//   }

//   Serial.println("Button monitor ready...");
// }

// void loop() {
//   for (int i = 0; i < 6; i++) {
//     int currentState = digitalRead(buttonPins[i]);

//     // Detect falling edge (HIGH -> LOW = button pressed)
//     if (currentState == LOW && prevState[i] == HIGH) {
//       // Perform action.
//       Serial.println(buttonNames[i]);
//     }
//     prevState[i] = currentState;
//   }

//   delay(DEBOUNCE_DELAY);
// }

#include <TFT_eSPI.h>
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();

// ---------------- UI STATE ----------------
enum UIState {
  UI_INTRO,
  UI_SCAN_LIST,
  UI_DETAILS
};

UIState state = UI_INTRO;

// ---------------- BUTTONS ----------------
#define BTN_UP      34
#define BTN_DOWN    33
#define BTN_LEFT    23
#define BTN_RIGHT   22
#define BTN_SELECT  21
#define BTN_BACK    5

bool pressed(int pin) {
  return digitalRead(pin) == LOW;
}

// ---------------- WIFI DATA ----------------
#define MAX_NETS 20

String ssidList[MAX_NETS];
int rssiList[MAX_NETS];
int channelList[MAX_NETS];
String encList[MAX_NETS];

int networkCount = 0;
int selectedIndex = 0;
int scrollOffset = 0;

// ---------------- INTRO SCREEN ----------------
void showIntro() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  tft.drawString("WiFi Inspector", 30, 60);
  tft.drawString("Press SELECT", 40, 100);
}

// ---------------- SCAN WIFI ----------------
void scanWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Scanning...", 40, 80);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  networkCount = WiFi.scanNetworks();

  if (networkCount > MAX_NETS) networkCount = MAX_NETS;

  for (int i = 0; i < networkCount; i++) {
    ssidList[i] = WiFi.SSID(i);
    rssiList[i] = WiFi.RSSI(i);
    channelList[i] = WiFi.channel(i);

    wifi_auth_mode_t auth = WiFi.encryptionType(i);
    encList[i] = (auth == WIFI_AUTH_OPEN) ? "OPEN" : "SECURED";
  }

  selectedIndex = 0;
  scrollOffset = 0;
}

// ---------------- LIST SCREEN ----------------
void showList() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  int maxVisible = 6;

  for (int i = 0; i < maxVisible; i++) {
    int idx = i + scrollOffset;
    if (idx >= networkCount) break;

    if (idx == selectedIndex) {
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    String line = ssidList[idx];
    line += " (" + String(rssiList[idx]) + ")";

    tft.drawString(line, 5, 20 + i * 25);
  }
}

// ---------------- DETAILS SCREEN ----------------
void showDetails() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  tft.drawString("SSID:", 10, 20);
  tft.drawString(ssidList[selectedIndex], 80, 20);

  tft.drawString("RSSI:", 10, 50);
  tft.drawString(String(rssiList[selectedIndex]), 80, 50);

  tft.drawString("CH:", 10, 80);
  tft.drawString(String(channelList[selectedIndex]), 80, 80);

  tft.drawString("SEC:", 10, 110);
  tft.drawString(encList[selectedIndex], 80, 110);

  tft.drawString("< BACK", 10, 200);
}

// ---------------- INPUT HANDLER ----------------
void handleInput() {

  if (state == UI_INTRO) {
    if (pressed(BTN_SELECT)) {
      delay(200);
      scanWiFi();
      state = UI_SCAN_LIST;
      showList();
    }
  }

  else if (state == UI_SCAN_LIST) {

    if (pressed(BTN_DOWN)) {
      selectedIndex++;
      if (selectedIndex >= networkCount) selectedIndex = 0;

      if (selectedIndex >= scrollOffset + 6) scrollOffset++;

      showList();
      delay(150);
    }

    if (pressed(BTN_UP)) {
      selectedIndex--;
      if (selectedIndex < 0) selectedIndex = networkCount - 1;

      if (selectedIndex < scrollOffset) scrollOffset--;

      showList();
      delay(150);
    }

    if (pressed(BTN_SELECT)) {
      state = UI_DETAILS;
      showDetails();
      delay(200);
    }

    if (pressed(BTN_BACK)) {
      state = UI_INTRO;
      showIntro();
      delay(200);
    }
  }

  else if (state == UI_DETAILS) {
    if (pressed(BTN_BACK)) {
      state = UI_SCAN_LIST;
      showList();
      delay(200);
    }
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);

  showIntro();
}

// ---------------- LOOP ----------------
void loop() {
  handleInput();
}