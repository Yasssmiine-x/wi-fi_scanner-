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
#define BTN_UP      16
#define BTN_DOWN    17
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
String bssidList[MAX_NETS];
String encList[MAX_NETS];

int rssiList[MAX_NETS];
int channelList[MAX_NETS];

int networkCount = 0;
int selectedIndex = 0;
int scrollOffset = 0;

// ---------------- INTRO SCREEN (STYLISH) ----------------
void showIntro() {
  tft.fillScreen(TFT_BLACK);

  // Outer frame
  tft.drawRect(5, 5, 230, 310, TFT_WHITE);
  tft.drawRect(8, 8, 224, 304, TFT_DARKGREY);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);

  // Title (large bold feel via size 3)
  tft.setTextSize(3);
  tft.drawString("WiFi Debugger", 20, 70);

  // Subtitle
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Final Year Project", 35, 120);

  // Divider line
  tft.drawLine(20, 150, 220, 150, TFT_RED);

  // Instruction
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Press SELECT", 55, 190);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("to start scan", 65, 220);
}

// ---------------- WIFI SCAN ----------------
void scanWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Scanning WiFi...", 40, 80);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  networkCount = WiFi.scanNetworks();

  if (networkCount > MAX_NETS) networkCount = MAX_NETS;

  for (int i = 0; i < networkCount; i++) {
    ssidList[i] = WiFi.SSID(i);
    rssiList[i] = WiFi.RSSI(i);
    channelList[i] = WiFi.channel(i);

    bssidList[i] = WiFi.BSSIDstr(i);

    wifi_auth_mode_t auth = WiFi.encryptionType(i);

    switch (auth) {
      case WIFI_AUTH_OPEN: encList[i] = "OPEN"; break;
      case WIFI_AUTH_WEP: encList[i] = "WEP"; break;
      case WIFI_AUTH_WPA_PSK: encList[i] = "WPA"; break;
      case WIFI_AUTH_WPA2_PSK: encList[i] = "WPA2"; break;
      case WIFI_AUTH_WPA_WPA2_PSK: encList[i] = "WPA/WPA2"; break;
      default: encList[i] = "UNKNOWN"; break;
    }
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

// ---------------- DETAILS SCREEN (EXPANDED) ----------------
void showDetails() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  int i = selectedIndex;

  bool hidden = (ssidList[i].length() == 0);

  // Header
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("NETWORK DETAILS", 20, 10);
  tft.drawLine(10, 30, 230, 30, TFT_RED);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawString("SSID:", 10, 40);
  tft.drawString(hidden ? "<hidden>" : ssidList[i], 90, 40);

  tft.drawString("RSSI:", 10, 65);
  tft.drawString(String(rssiList[i]) + " dBm", 90, 65);

  tft.drawString("CH:", 10, 90);
  tft.drawString(String(channelList[i]), 90, 90);

  tft.drawString("BAND:", 10, 115);
  tft.drawString((channelList[i] <= 14) ? "2.4 GHz" : "5 GHz?", 90, 115);

  tft.drawString("SEC:", 10, 140);
  tft.drawString(encList[i], 90, 140);

  tft.drawString("BSSID:", 10, 165);
  tft.drawString(bssidList[i].substring(0, 11), 90, 165);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("< BACK", 10, 210);
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