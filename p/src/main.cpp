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
#define ADDRESSBOOK_TITLE ("broadcast only")
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
void init_qlist() {
  qlist.push_back(Note(10000, 1, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 1, 0, 0, 0, 0, 0, 0, 0));
}

//buttons
const int pin_vol_up = 39;
const int pin_vol_down = 36;
const int pin_mute = 35;
const int pin_previous = 15;
const int pin_pause = 33;
const int pin_next = 2;

//screen task
String screen_cmd = ":Q: ----- --- --- -";

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
    MONITORING_SERIAL.print("# posting a req.# ==> ");
    MONITORING_SERIAL.println(qlist[qselect].to_string());
    //
    uint8_t frm_size = sizeof(Note) + 2;
    uint8_t frm[frm_size];
    frm[0] = '[';
    memcpy(frm + 1, (uint8_t *) &qlist[qselect], sizeof(Note));
    frm[frm_size - 1] = ']';
    //
    // strange but following didn't work as expected. (instead, i have to send one-by-one.)
    // esp_now_send(NULL, frm, frm_size); // to all peers in the list.

    // so, forget about peer list -> just pick a broadcast peer to be sent.
    uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastmac, frm, frm_size);


    //+ fancy stuff
    screen_req_notify_task.restart();

    //+ automatically increase 'song_request'
    qselect = qselect + 1;
    if (qselect > (qlist.size() - 1)) qselect = 0;
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

  //line1 - mode line (playing / stopped) + notify mark
  display.setCursor(0, line);
  display.printf("= transmitting ))) ==");
  line += line_step;

  //line3 - msg. ready to be sent now.
  display.setCursor(0, line);
  display.printf(":Q: %05d %03d %03d %01d", (int)qlist[qselect].id, (int)qlist[qselect].pitch, (int)qlist[qselect].velocity, (int)qlist[qselect].onoff);
  line += line_step;

  //line4 - big file name display
  display.setCursor(0, line);
  display.setTextSize(2);
  //
  char filename[14] = "/NNN.mp3 >";
  int note = qlist[qselect].pitch;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[1] = '0' + (nnn / 100); // N__.MP3
  filename[2] = '0' + (nn / 10);   // _N_.MP3
  filename[3] = '0' + (nn % 10);   // __N.MP3
  //
  if (qlist[qselect].onoff == 0) {
    filename[9] = 'x';
  }
  display.println(filename);
  line += line_step;
  display.setTextSize(1);
  line += 8;

  // //alternative line3+4 - full msg. dump (DEBUG/MONITORING)
  // display.setCursor(0, line);
  // display.println(":Q: " + qlist[qselect].to_string());
  // line += line_step;

  //line5 - song req. tx. notify. (GEN_NOTE_REQ)
  display.setCursor(0, line);
  if (req_notify) {
    display.setCursor(25, line);
    display.println("~~ d[+=+]b ~~ >>>");
  }
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
  Serial.println("- my peer book ==> \"" + String(ADDRESSBOOK_TITLE) + "\"");
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
  //
  // Serial.println("- ! (esp_now_add_peer) ==> add a 'broadcast peer' (FF:FF:FF:FF:FF:FF).");
  // uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //
  // //
  // esp_now_peer_info_t peerInfo = {};
  // memcpy(peerInfo.peer_addr, broadcastmac, 6);
  // peerInfo.channel = 0;
  // peerInfo.encrypt = false;
  // esp_now_add_peer(&peerInfo);

  AddressBook * book = lib.getBookByTitle(ADDRESSBOOK_TITLE);
  if (book == NULL) {
    Serial.println("- ! wrong book !! : \"" + String(ADDRESSBOOK_TITLE) + "\""); while(1);
  }
  for (int idx = 0; idx < book->list.size(); idx++) {
    Serial.println("- ! (esp_now_add_peer) ==> add a '" + book->list[idx].name + "'.");
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, book->list[idx].mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
  // (DEBUG) fetch full peer list
  { PeerLister a; a.print(); }
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
