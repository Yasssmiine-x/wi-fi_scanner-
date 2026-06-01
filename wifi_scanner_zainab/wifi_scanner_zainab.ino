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

#include <TFT_eSPI.h>
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();

// ===================== UI STATES =====================
enum UIState {
  UI_INTRO,
  UI_SCAN_LIST,
  UI_DETAILS,
  UI_PASSWORD,
  UI_STATUS
};

UIState state = UI_INTRO;

// ===================== BUTTONS =====================
#define BTN_UP      16
#define BTN_DOWN    17
#define BTN_LEFT    23
#define BTN_RIGHT   22
#define BTN_SELECT  21
#define BTN_BACK    5

bool pressed(int pin) {
  return digitalRead(pin) == LOW;
}

// ===================== WIFI DATA =====================
#define MAX_NETS 20

String ssidList[MAX_NETS];
String encList[MAX_NETS];
int rssiList[MAX_NETS];
int channelList[MAX_NETS];

int networkCount = 0;
int selectedIndex = 0;
int scrollOffset = 0;

// ===================== PASSWORD INPUT =====================
const char *keys[5][9] = {
  {"A","B","C","D","E","F","G","H","I"},
  {"J","K","L","M","N","O","P","Q","R"},
  {"S","T","U","V","W","X","Y","Z","0"},
  {"1","2","3","4","5","6","7","8","9"},
  {"_","@",".","-"," "," "," "," "," "}
};

int row = 0, col = 0;
String password = "";

// ===================== INTRO =====================
void showIntro() {
  tft.fillScreen(TFT_BLACK);

  tft.drawRect(5, 5, 230, 310, TFT_WHITE);
  tft.setTextColor(TFT_CYAN);

  tft.setTextSize(3);
  tft.drawString("WiFi", 85, 60);
  tft.drawString("Debugger", 45, 100);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Final Year Project", 25, 150);

  tft.drawLine(20, 180, 220, 180, TFT_RED);

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Press SELECT", 55, 210);
}

// ===================== WIFI SCAN =====================
void scanWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Scanning WiFi...", 40, 100);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  networkCount = WiFi.scanNetworks();
  if (networkCount > MAX_NETS) networkCount = MAX_NETS;

  for (int i = 0; i < networkCount; i++) {
    ssidList[i] = WiFi.SSID(i);
    rssiList[i] = WiFi.RSSI(i);
    channelList[i] = WiFi.channel(i);

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

// ===================== LIST =====================
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

    String line = ssidList[idx] + " (" + String(rssiList[idx]) + ")";
    tft.drawString(line, 5, 20 + i * 25);
  }
}

// ===================== DETAILS =====================
void showDetails() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  int i = selectedIndex;

  tft.setTextColor(TFT_CYAN);
  tft.drawString("NETWORK INFO", 40, 10);
  tft.drawLine(10, 30, 230, 30, TFT_BLUE);

  tft.setTextColor(TFT_WHITE);

  tft.drawString("SSID:", 10, 40);
  tft.drawString(ssidList[i], 80, 40);

  tft.drawString("RSSI:", 10, 65);
  tft.drawString(String(rssiList[i]), 80, 65);

  tft.drawString("CH:", 10, 90);
  tft.drawString(String(channelList[i]), 80, 90);

  tft.drawString("SEC:", 10, 115);
  tft.drawString(encList[i], 80, 115);

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("SELECT = CONNECT", 10, 200);
  tft.drawString("BACK = RETURN", 10, 220);
}

// ===================== PASSWORD UI =====================
void drawKeyboard() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  tft.setTextColor(TFT_WHITE);
  tft.drawString("ENTER PASSWORD", 20, 5);

  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 9; c++) {

      int x = 10 + c * 25;
      int y = 40 + r * 30;

      if (r == row && c == col) {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
      } else {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }

      tft.drawString(keys[r][c], x, y);
    }
  }

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("PWD:", 10, 200);
  tft.drawString(password, 60, 200);
}

// ===================== CONNECT =====================
void connectWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  tft.drawString("Connecting...", 50, 100);

  WiFi.begin(ssidList[selectedIndex].c_str(), password.c_str());

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    timeout++;
  }

  tft.fillScreen(TFT_BLACK);

  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN);
    tft.drawString("CONNECTED!", 60, 100);
    tft.drawString(WiFi.localIP().toString(), 40, 140);
  } else {
    tft.setTextColor(TFT_RED);
    tft.drawString("FAILED", 80, 100);
  }
}

// ===================== INPUT =====================
void handleInput() {

  // ---------- INTRO ----------
  if (state == UI_INTRO) {
    if (pressed(BTN_SELECT)) {
      delay(200);
      scanWiFi();
      state = UI_SCAN_LIST;
      showList();
    }
  }

  // ---------- LIST ----------
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

  // ---------- DETAILS ----------
  else if (state == UI_DETAILS) {

    if (pressed(BTN_SELECT)) {
      password = "";
      row = 0;
      col = 0;
      state = UI_PASSWORD;
      drawKeyboard();
      delay(200);
    }

    if (pressed(BTN_BACK)) {
      state = UI_SCAN_LIST;
      showList();
      delay(200);
    }
  }

  // ---------- PASSWORD ----------
  else if (state == UI_PASSWORD) {

    if (pressed(BTN_UP)) {
      row = (row - 1 + 5) % 5;
      drawKeyboard();
      delay(150);
    }

    if (pressed(BTN_DOWN)) {
      row = (row + 1) % 5;
      drawKeyboard();
      delay(150);
    }

    if (pressed(BTN_LEFT)) {
      col = (col - 1 + 9) % 9;
      drawKeyboard();
      delay(150);
    }

    if (pressed(BTN_RIGHT)) {
      col = (col + 1) % 9;
      drawKeyboard();
      delay(150);
    }

    if (pressed(BTN_SELECT)) {
      String ch = keys[row][col];
      if (ch != " ") password += ch;
      drawKeyboard();
      delay(200);
    }

    if (pressed(BTN_BACK)) {
      if (password.length() > 0)
        password.remove(password.length() - 1);

      drawKeyboard();
      delay(200);
    }

    // LONG ACTION: connect
    if (pressed(BTN_SELECT) && password.length() > 0) {
      state = UI_STATUS;
      connectWiFi();
      delay(500);
    }
  }

  // ---------- STATUS ----------
  else if (state == UI_STATUS) {
    if (pressed(BTN_BACK)) {
      state = UI_SCAN_LIST;
      showList();
      delay(200);
    }
  }
}

// ===================== SETUP =====================
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

// ===================== LOOP =====================
void loop() {
  handleInput();
}