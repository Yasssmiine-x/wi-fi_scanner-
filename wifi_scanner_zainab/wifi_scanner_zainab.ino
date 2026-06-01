// ===================== PIN DEFINITIONS =====================
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
enum UIState { UI_INTRO, UI_SCAN_LIST, UI_DETAILS, UI_PASSWORD, UI_STATUS };
UIState state = UI_INTRO;

// ===================== BUTTONS =====================
#define BTN_UP      16
#define BTN_DOWN    17
#define BTN_LEFT    23
#define BTN_RIGHT   22
#define BTN_SELECT  21
#define BTN_BACK     5

#define DEBOUNCE_DELAY 20

volatile bool     f_up=false, f_down=false, f_left=false, f_right=false, f_select=false, f_back=false;
volatile uint32_t lastUp=0,   lastDown=0,   lastLeft=0,   lastRight=0,   lastSelect=0,   lastBack=0;

void IRAM_ATTR isr_up()     { uint32_t n=millis(); if(n-lastUp>DEBOUNCE_DELAY)    {f_up=true;    lastUp=n;}    }
void IRAM_ATTR isr_down()   { uint32_t n=millis(); if(n-lastDown>DEBOUNCE_DELAY)  {f_down=true;  lastDown=n;}  }
void IRAM_ATTR isr_left()   { uint32_t n=millis(); if(n-lastLeft>DEBOUNCE_DELAY)  {f_left=true;  lastLeft=n;}  }
void IRAM_ATTR isr_right()  { uint32_t n=millis(); if(n-lastRight>DEBOUNCE_DELAY) {f_right=true; lastRight=n;} }
void IRAM_ATTR isr_select() { uint32_t n=millis(); if(n-lastSelect>DEBOUNCE_DELAY){f_select=true;lastSelect=n;}}
void IRAM_ATTR isr_back()   { uint32_t n=millis(); if(n-lastBack>DEBOUNCE_DELAY)  {f_back=true;  lastBack=n;}  }

// ===================== WIFI DATA =====================
#define MAX_NETS 20
String ssidList[MAX_NETS];
String encList[MAX_NETS];
int    rssiList[MAX_NETS];
int    channelList[MAX_NETS];
int    networkCount=0, selectedIndex=0, scrollOffset=0;

// ===================== KEYBOARD =====================
#define KEY_ROWS 6
#define KEY_COLS 12

// Each cell is 20px wide, 26px tall. Keyboard starts at x=0, y=38
// Total width  = 12 * 20 = 240 (fills screen exactly)
// Total height = 6  * 26 = 156, ends at y=194
#define KEY_X_START  0
#define KEY_Y_START  38
#define KEY_CELL_W   20
#define KEY_CELL_H   26

const char* keys[KEY_ROWS][KEY_COLS] = {
  {"A","B","C","D","E","F","G","H","I","J","K","L"},
  {"M","N","O","P","Q","R","S","T","U","V","W","X"},
  {"Y","Z","a","b","c","d","e","f","g","h","i","j"},
  {"k","l","m","n","o","p","q","r","s","t","u","v"},
  {"w","x","y","z","0","1","2","3","4","5","6","7"},
  {"8","9","_","@",".","-","!","#","$","OK","*","%"}
};

int    kbRow=0, kbCol=0;
String password = "";

// ===================== LIST GEOMETRY =====================
#define LIST_VISIBLE  6
#define LIST_ROW_H   25
#define LIST_Y0      20

// ===================== DRAW: single keyboard ROW (not cell) =====================
// Redraws all 12 cells in one row — eliminates ghost pixels completely
void drawKbRow(int r) {
  int y = KEY_Y_START + r * KEY_CELL_H;
  // Clear entire row strip
  tft.fillRect(0, y, 240, KEY_CELL_H, TFT_BLACK);

  tft.setTextSize(2);
  for (int c = 0; c < KEY_COLS; c++) {
    int x = KEY_X_START + c * KEY_CELL_W;
    bool sel = (r == kbRow && c == kbCol);
    if (sel) {
      tft.fillRect(x, y, KEY_CELL_W - 1, KEY_CELL_H - 1, TFT_WHITE);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    // Center the char in the cell (size-2 char = ~12px wide, ~16px tall)
    tft.drawString(keys[r][c], x + 2, y + 4);
  }
}

// ===================== DRAW: password bar =====================
#define PWD_BAR_Y  200
void drawPasswordBar() {
  tft.fillRect(0, PWD_BAR_Y, 240, 20, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("PWD:", 0, PWD_BAR_Y);

  // Show at most ~9 trailing chars so it fits
  String disp = password;
  if (disp.length() > 9) disp = "..." + disp.substring(disp.length() - 9);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(disp, 65, PWD_BAR_Y);
}

// ===================== DRAW: full keyboard screen =====================
void drawKeyboardFull() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("ENTER PASSWORD", 20, 8);

  for (int r = 0; r < KEY_ROWS; r++) drawKbRow(r);

  // Bottom hint
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("SELECT=type  OK key=connect  BACK=del", 2, 222);

  drawPasswordBar();
}

// ===================== DRAW: single list row =====================
void drawListRow(int visIdx) {
  int dataIdx = visIdx + scrollOffset;
  int y = LIST_Y0 + visIdx * LIST_ROW_H;

  if (dataIdx >= networkCount) {
    tft.fillRect(0, y, 240, LIST_ROW_H, TFT_BLACK);
    return;
  }

  bool sel = (dataIdx == selectedIndex);
  tft.fillRect(0, y, 240, LIST_ROW_H, sel ? TFT_WHITE : TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(sel ? TFT_BLACK : TFT_WHITE, sel ? TFT_WHITE : TFT_BLACK);

  String line = ssidList[dataIdx] + " (" + String(rssiList[dataIdx]) + ")";
  tft.drawString(line, 5, y + 2);
}

// ===================== DRAW: full list screen =====================
void showList() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  for (int i = 0; i < LIST_VISIBLE; i++) drawListRow(i);
}

// ===================== INTRO =====================
void showIntro() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(5, 5, 230, 310, TFT_WHITE);

  tft.setTextSize(3);
  tft.setTextColor(TFT_CYAN);
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
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Scanning WiFi...", 40, 100);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  networkCount = WiFi.scanNetworks();
  if (networkCount > MAX_NETS) networkCount = MAX_NETS;

  for (int i = 0; i < networkCount; i++) {
    ssidList[i]    = WiFi.SSID(i);
    rssiList[i]    = WiFi.RSSI(i);
    channelList[i] = WiFi.channel(i);
    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_OPEN:          encList[i]="OPEN";     break;
      case WIFI_AUTH_WEP:           encList[i]="WEP";      break;
      case WIFI_AUTH_WPA_PSK:       encList[i]="WPA";      break;
      case WIFI_AUTH_WPA2_PSK:      encList[i]="WPA2";     break;
      case WIFI_AUTH_WPA_WPA2_PSK:  encList[i]="WPA/WPA2"; break;
      default:                      encList[i]="UNKNOWN";  break;
    }
  }

  selectedIndex = 0;
  scrollOffset  = 0;
}

// ===================== DETAILS =====================
void showDetails() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  tft.setTextColor(TFT_CYAN);
  tft.drawString("NETWORK INFO", 40, 10);
  tft.drawLine(10, 30, 230, 30, TFT_BLUE);

  tft.setTextColor(TFT_WHITE);
  tft.drawString("SSID:", 10, 40);  tft.drawString(ssidList[selectedIndex],     80, 40);
  tft.drawString("RSSI:", 10, 65);  tft.drawString(String(rssiList[selectedIndex]), 80, 65);
  tft.drawString("CH:",   10, 90);  tft.drawString(String(channelList[selectedIndex]), 80, 90);
  tft.drawString("SEC:",  10,115);  tft.drawString(encList[selectedIndex],       80,115);

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("SELECT = CONNECT", 10, 200);
  tft.drawString("BACK = RETURN",    10, 225);
}

// ===================== CONNECT =====================
void connectWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Connecting...", 50, 100);

  WiFi.begin(ssidList[selectedIndex].c_str(), password.c_str());

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    timeout++;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN);
    tft.drawString("CONNECTED!", 55, 90);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("IP:", 10, 120);
    tft.drawString(WiFi.localIP().toString(), 50, 120);
  } else {
    tft.setTextColor(TFT_RED);
    tft.drawString("FAILED!", 75, 90);
  }

  tft.setTextColor(TFT_YELLOW);
  tft.drawString("BACK = list",    10, 200);
  tft.drawString("SELECT = retry", 10, 225);
}

// ===================== INPUT HANDLER =====================
void handleInput() {

  // ---------- INTRO ----------
  if (state == UI_INTRO) {
    if (f_select) {
      f_select = false;
      scanWiFi();
      state = UI_SCAN_LIST;
      showList();
    }
  }

  // ---------- SCAN LIST ----------
  else if (state == UI_SCAN_LIST) {

    if (networkCount == 0) {
      if (f_back || f_select) { f_back=false; f_select=false; state=UI_INTRO; showIntro(); }
      return;
    }

    if (f_down) {
      f_down = false;
      int oldIdx = selectedIndex;
      int oldOff = scrollOffset;

      selectedIndex++;
      if (selectedIndex >= networkCount) {
        // Wrap to top
        selectedIndex = 0;
        scrollOffset  = 0;
        showList();
      } else {
        if (selectedIndex >= scrollOffset + LIST_VISIBLE) scrollOffset++;
        if (scrollOffset != oldOff) {
          showList();  // scrolled — full redraw
        } else {
          drawListRow(oldIdx      - scrollOffset);
          drawListRow(selectedIndex - scrollOffset);
        }
      }
    }

    if (f_up) {
      f_up = false;
      int oldIdx = selectedIndex;
      int oldOff = scrollOffset;

      selectedIndex--;
      if (selectedIndex < 0) {
        // Wrap to bottom
        selectedIndex = networkCount - 1;
        scrollOffset  = max(0, networkCount - LIST_VISIBLE);
        showList();
      } else {
        if (selectedIndex < scrollOffset) scrollOffset--;
        if (scrollOffset != oldOff) {
          showList();  // scrolled — full redraw
        } else {
          drawListRow(oldIdx      - scrollOffset);
          drawListRow(selectedIndex - scrollOffset);
        }
      }
    }

    if (f_select) { f_select=false; state=UI_DETAILS; showDetails(); }
    if (f_back)   { f_back=false;   state=UI_INTRO;   showIntro();   }
  }

  // ---------- DETAILS ----------
  else if (state == UI_DETAILS) {
    if (f_select) {
      f_select = false;
      password = "";
      kbRow = 0; kbCol = 0;
      state = UI_PASSWORD;
      drawKeyboardFull();
    }
    if (f_back) { f_back=false; state=UI_SCAN_LIST; showList(); }
  }

  // ---------- PASSWORD ----------
  else if (state == UI_PASSWORD) {

    // --- Movement: only redraw old row and new row ---
    if (f_up) {
      f_up = false;
      int oldRow = kbRow;
      kbRow = (kbRow - 1 + KEY_ROWS) % KEY_ROWS;  // wrap: row 0 -> row 5
      if (oldRow != kbRow) { drawKbRow(oldRow); drawKbRow(kbRow); }
    }
    else if (f_down) {
      f_down = false;
      int oldRow = kbRow;
      kbRow = (kbRow + 1) % KEY_ROWS;              // wrap: row 5 -> row 0
      if (oldRow != kbRow) { drawKbRow(oldRow); drawKbRow(kbRow); }
    }
    else if (f_left) {
      f_left = false;
      int oldRow = kbRow, oldCol = kbCol;
      kbCol--;
      if (kbCol < 0) {
        // Wrap left: go to last col of previous row
        kbRow = (kbRow - 1 + KEY_ROWS) % KEY_ROWS;
        kbCol = KEY_COLS - 1;
      }
      if (oldRow != kbRow) { drawKbRow(oldRow); drawKbRow(kbRow); }
      else                  { drawKbRow(kbRow); }  // same row, redraw it
    }
    else if (f_right) {
      f_right = false;
      int oldRow = kbRow, oldCol = kbCol;
      kbCol++;
      if (kbCol >= KEY_COLS) {
        // Wrap right: go to first col of next row
        kbRow = (kbRow + 1) % KEY_ROWS;
        kbCol = 0;
      }
      if (oldRow != kbRow) { drawKbRow(oldRow); drawKbRow(kbRow); }
      else                  { drawKbRow(kbRow); }  // same row, redraw it
    }
    else if (f_select) {
      f_select = false;
      String ch = String(keys[kbRow][kbCol]);
      if (ch == "OK") {
        if (password.length() > 0) {
          state = UI_STATUS;
          connectWiFi();
        }
      } else {
        password += ch;
        drawPasswordBar();
      }
    }
    else if (f_back) {
      f_back = false;
      if (password.length() > 0) {
        password.remove(password.length() - 1);
        drawPasswordBar();
      } else {
        state = UI_DETAILS;
        showDetails();
      }
    }
  }

  // ---------- STATUS ----------
  else if (state == UI_STATUS) {
    if (f_back) {
      f_back = false;
      state = UI_SCAN_LIST;
      showList();
    }
    if (f_select) {
      f_select = false;
      password = "";
      kbRow = 0; kbCol = 0;
      state = UI_PASSWORD;
      drawKeyboardFull();
    }
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK,   INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_UP),     isr_up,     FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_DOWN),   isr_down,   FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_LEFT),   isr_left,   FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_RIGHT),  isr_right,  FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_SELECT), isr_select, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_BACK),   isr_back,   FALLING);

  tft.init();
  tft.setRotation(1);
  showIntro();
}

// ===================== LOOP =====================
void loop() {
  handleInput();
}