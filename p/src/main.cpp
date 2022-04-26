//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// 0set performance 'Georidugi'/Distancing
// @ 2021 Jun 15 ~ 17
//

//
// 2021 june
//
// esp32 based sampler + 'note' sequence generator!
//

//============<identities>============
//
#define MY_GROUP_ID   (80000)
#define MY_ID         (MY_GROUP_ID + 1)
#define MY_SIGN       ("P")
//
#define ADDRESSBOOK_TITLE ("1st floor")
//============</identities>============

//==========<list-of-configurations>===========
//
// 'DISABLE_AP'
// --> (questioning)...
//
// 'GEN_NOTE_REQ'
// --> this will generate 'note' msg.
//
//==========</list-of-configurations>==========
//
#define DISABLE_AP
#define GEN_NOTE_REQ

//============<parameters>============
//
#define SCREEN_PERIOD (200) //200ms = 5hz
//
#define WIFI_CHANNEL 1
//
// 'MONITORING_SERIAL'
//
// --> sometimes, the 'Serial' is in use (for example, 'osc' node)
//     then,      use 'Serial1' - D4/GPIO2/TDX1 @ nodemcu (this is TX only.)
//
// --> otherwise, MONITORING_SERIAL == Serial.
//
#if defined(SERIAL_SWAP)
#define MONITORING_SERIAL (Serial1)
#else
#define MONITORING_SERIAL (Serial)
#endif
//
//============</parameters>===========

//============<board-specifics>============
//============</board-specifics>===========

//arduino
#include <Arduino.h>

//post & addresses
#include "../../post.h"

//espnow
#include <esp_now.h>
#include <WiFi.h>
AddressLibrary lib;

//task
#include <TaskScheduler.h>
Scheduler runner;

//screen
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define MAKEPYTHON_ESP32_SDA 4
#define MAKEPYTHON_ESP32_SCL 5
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//-*-*-*-*-*-*-*-*-*-*-*-*-
// my tasks

// q list
std::vector<Note> qlist;
std::vector<String> qlist_desc;
void init_qlist() {
  //
  qlist.push_back(Note(20000, 40, 60, 1, 0, 0, 0, 0, 0)); //guide A: start song
  qlist_desc.push_back("guide A: start song");
  //
  qlist.push_back(Note(20000, 30, 0,  0, 0, 0, 0, 0, 0)); //stop all.
  qlist_desc.push_back("stop all.");
  //
  qlist.push_back(Note(20000, 30, 60, 1, 0, 0, 0, 0, 0)); //tour start
  qlist_desc.push_back("tour start");
  //
  qlist.push_back(Note(20000, 30, 0,  0, 0, 0, 0, 0, 0)); //stop all.
  qlist_desc.push_back("stop all.");
  //
  qlist.push_back(Note(20000, 31, 70, 1, 0, 0, 0, 0, 0)); //tour end
  qlist_desc.push_back("tour end");
  //
  qlist.push_back(Note(20000, 30, 0,  0, 0, 0, 0, 0, 0)); //stop all.
  qlist_desc.push_back("stop all.");
  //
  qlist.push_back(Note(20000, 41, 60, 1, 0, 0, 0, 0, 0)); //guide A: end song
  qlist_desc.push_back("guide A: end song");
  //
  qlist.push_back(Note(20000, 40, 60, 1, 0, 0, 0, 0, 0)); //guide B: start song
  qlist_desc.push_back("guide B: start song");
  //
  qlist.push_back(Note(20000, 30, 0,  0, 0, 0, 0, 0, 0)); //stop all.
  qlist_desc.push_back("stop all.");
  //
  qlist.push_back(Note(20000, 30, 60, 1, 0, 0, 0, 0, 0)); //tour start
  qlist_desc.push_back("tour start");
  //
  qlist.push_back(Note(20000, 30, 0,  0, 0, 0, 0, 0, 0)); //stop all.
  qlist_desc.push_back("stop all.");
  //
  qlist.push_back(Note(20000, 31, 70, 1, 0, 0, 0, 0, 0)); //tour end
  qlist_desc.push_back("tour end");
  //
  qlist.push_back(Note(20000, 30, 0,  0, 0, 0, 0, 0, 0)); //stop all.
  qlist_desc.push_back("stop all.");
  //
  qlist.push_back(Note(20000, 51, 60, 1, 0, 0, 0, 0, 0)); //guide B: end song
  qlist_desc.push_back("guide B: end song");
}

//buttons
const int pin_vol_up = 39;
const int pin_vol_down = 36;
const int pin_mute = 35;
const int pin_previous = 15;
const int pin_pause = 33;
const int pin_next = 2;

//screen task
String screen_cmd = "XXX..composing..XXX";
String screen_filename = "***.mp3";

//
extern Task screen_req_notify_task;
bool req_notify = false;
void screen_req_notify() {
  if (screen_req_notify_task.isFirstIteration()) req_notify = true;
  else if (screen_req_notify_task.isLastIteration()) req_notify = false;
  else req_notify = !req_notify;
}
Task screen_req_notify_task(500, 8, &screen_req_notify, &runner, false);

//
extern Task screen_task;
void screen() {

  //
  if (screen_task.isFirstIteration()) {
    init_qlist();
  }

  // button job!
  static int btn_vol_up = 0;
  static int btn_vol_down = 0;
  static int btn_mute = 0;
  static int btn_previous = 0;
  static int btn_pause = 0;
  static int btn_next = 0;
  //
  int a;
  static int qselect = 0;
  bool nowsend = false;
  //
  a = digitalRead(pin_vol_up);
  if(btn_vol_up != a) {
    btn_vol_up = a;
    if(a == 0) {
      // 'vol_up' button pressed.

      qselect = qselect + 1;
      if (qselect > (qlist.size() - 1)) qselect = 0;
    }
  }
  //
  a = digitalRead(pin_vol_down);
  if(btn_vol_down != a) {
    btn_vol_down = a;
    if(a == 0) {
      // 'vol_down' button pressed.

      qselect = qselect - 1;
      if (qselect < 0) qselect = (qlist.size() - 1);
    }
  }
  //
  a = digitalRead(pin_mute);
  if(btn_mute != a) {
    btn_mute = a;
    if(a == 0) {
      // 'mute' button pressed.

      nowsend = true;
    }
  }
  //
  a = digitalRead(pin_previous);
  if(btn_previous != a) {
    btn_previous = a;
    if(a == 0) {
      // 'previous' button pressed.

      qselect = qselect - 1;
      if (qselect < 0) qselect = (qlist.size() - 1);
    }
  }
  //
  a = digitalRead(pin_pause);
  if(btn_pause != a) {
    btn_pause = a;
    if(a == 0) {
      // 'pause' button pressed.

      nowsend = true;
    }
  }
  //
  a = digitalRead(pin_next);
  if(btn_next != a) {
    btn_next = a;
    if(a == 0) {
      // 'next' button pressed.

      qselect = qselect + 1;
      if (qselect > (qlist.size() - 1)) qselect = 0;
    }
  }

  if (nowsend == true) {
    nowsend = false;
    // create a NOTE req. and send it out.
    //
    uint8_t frm_size = sizeof(Note) + 2;
    uint8_t frm[frm_size];
    frm[0] = '[';
    memcpy(frm + 1, (uint8_t *) &qlist[qselect], sizeof(Note));
    frm[frm_size - 1] = ']';
    //
    esp_now_send(NULL, frm, frm_size); // to all peers in the list.
    //
    MONITORING_SERIAL.print("# posting a req.# ==> ");
    MONITORING_SERIAL.println(qlist[qselect].to_string());
    //

    //+ fancy stuff
    screen_req_notify_task.restart();
  }

  //clear screen + a
  int line_step = 12;
  int line = 0;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  //button status (DEBUG)
  // int b = digitalRead(pin_vol_up);
  // b = b*10 + digitalRead(pin_vol_down);
  // b = b*10 + digitalRead(pin_mute);
  // b = b*10 + digitalRead(pin_previous);
  // b = b*10 + digitalRead(pin_pause);
  // b = b*10 + digitalRead(pin_next);

  //line1 - q description.
  display.setTextSize(1);
  display.setCursor(0, line);
  display.println("[" + String(qselect) + "] " + qlist_desc[qselect]);
  line += line_step;

  //line2 - q sent notify
  display.setCursor(0, line);
  if (req_notify) {
    display.setCursor(25, line);
    display.println("~~ d[+=+]b ~~ >>>");
  }
  line += line_step;

  //line3 - note dump
  display.setTextSize(1);
  display.setCursor(0, line);
  display.println(qlist[qselect].to_string());
  line += line_step;

  //
  display.display();
  //
}
Task screen_task(SCREEN_PERIOD, TASK_FOREVER, &screen, &runner, true);
//*-*-*-*-*-*-*-*-*-*-*-*-*

// on 'receive'
void onDataReceive(const uint8_t * mac, const uint8_t *incomingData, int32_t len) {

  // open => identify => use.
  if (incomingData[0] == '{' && incomingData[len - 1] == '}' && len == (sizeof(Hello) + 2)) {
    Hello hello("");
    memcpy((uint8_t *) &hello, incomingData + 1, sizeof(Hello));
    //
    MONITORING_SERIAL.println(hello.to_string());
    //
  }

  // open => identify => use.
  if (incomingData[0] == '[' && incomingData[len - 1] == ']' && len == (sizeof(Note) + 2)) {
    Note note;
    memcpy((uint8_t *) &note, incomingData + 1, sizeof(Note));
    //
    MONITORING_SERIAL.println(note.to_string());
    //
  }
}

// on 'sent'
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  if (sendStatus != 0) MONITORING_SERIAL.println("Delivery failed!");
}

void lcd_text(String str) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(str.c_str());
  display.display();
}

//
void setup() {

  //buttons
  pinMode(pin_vol_up, INPUT_PULLUP);
  pinMode(pin_vol_down, INPUT_PULLUP);
  pinMode(pin_mute, INPUT_PULLUP);
  pinMode(pin_previous, INPUT_PULLUP);
  pinMode(pin_pause, INPUT_PULLUP);
  pinMode(pin_next, INPUT_PULLUP);

  //serial
  Serial.begin(115200);
  delay(100);

  //screen
  Wire.begin(MAKEPYTHON_ESP32_SDA, MAKEPYTHON_ESP32_SCL);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;         // Don't proceed, loop forever
  }
  display.clearDisplay();

  //info
  Serial.println();
  Serial.println();
  Serial.println("\"hi, i m your postman.\"");
  Serial.println("-");
  Serial.println("- my id: " + String(MY_ID) + ", gid: " + String(MY_GROUP_ID) + ", call me ==> \"" + String(MY_SIGN) + "\"");
  Serial.println("- mac address: " + WiFi.macAddress() + ", channel: " + String(WIFI_CHANNEL));
#if defined(DISABLE_AP)
  Serial.println("- ======== 'DISABLE_AP' ========");
#endif
#if defined(GEN_NOTE_REQ)
  Serial.println("- ======== 'GEN_NOTE_REQ' ========");
#endif
  Serial.println("-");

  //wifi
  WiFiMode_t node_type = WIFI_AP_STA;
#if defined(DISABLE_AP)
  // system_phy_set_max_tpw(0);
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm); // Set WiFi RF power output to lowest level
  node_type = WIFI_STA;
#endif
  WiFi.mode(node_type);

  //esp-now
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);

  //fetch & read addressbook
  String addressbook_title = ADDRESSBOOK_TITLE;
// #if defined(ADDRESSBOOK_TITLE_CLI)
//   addressbook_title = ADDRESSBOOK_TITLE_CLI;
// #endif
//
// NOTE: there is a way to give a define value here like:
//   export PLATFORMIO_SRC_BUILD_FLAGS="'-DADDRESSBOOK_TITLE_CLI=\"broadcast only\"'" && pio run
// but, everytime i change this, whole arduino framework + libraries rebuild.
// PLATFORMIO_SRC_BUILD_FLAGS supposed to work only to src/ but strange.
// this takes up too much time, not really haptic. later, investigate the issues.
//
  AddressBook * book = lib.getBookByTitle(addressbook_title);
  if (book == NULL) {
    Serial.println("- ! wrong book !! :" + addressbook_title); while(1);
  } else {
    Serial.println("- ! reading book ....");
    Serial.println("    -----------------");
    Serial.println("    { " + addressbook_title + " }");
    Serial.println("    -----------------");
    Serial.println();
  }
  for (int idx = 0; idx < book->list.size(); idx++) {
    Serial.println("- ! (esp_now_add_peer) ==> add a '" + book->list[idx].name + "'.");
#if defined(ESP32)
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, book->list[idx].mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
#else
    esp_now_add_peer(book->list[idx].mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
#endif
  }
  //
  Serial.println("-");
  Serial.println("\".-.-.-. :)\"");
  Serial.println();
}

void loop() {
  //
  runner.execute();
  //
}
