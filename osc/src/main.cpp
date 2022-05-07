//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN)
//

//============<identities>============
//
#define MY_GROUP_ID       (0)
#define MY_ID             (MY_GROUP_ID + 2)
#define MY_SIGN           ("POSTMAN|OSC(Pd)")
#define ADDRESSBOOK_TITLE ("broadcast only")
//
//============</identities>============

//==========<list-of-configurations>===========
//
// 'HAVE_CLIENT'
// --> i have a client. enable the client task.
//
// 'SERIAL_SWAP'
// --> UART pin swapped.
//     you want this, when you want a bi-directional comm. to external client boards (e.g. teensy).
//
// 'SLIPSERIAL_ACTIVE'
// --> UART pin will be busy.
//     use other serial for monitoring.
//
// 'DISABLE_AP'
// --> (questioning)...
//
// 'HAVE_CLIENT_I2C'
// --> i have a client w/ I2C i/f. enable the I2C client task.
//
// 'ADDRESSBOOK_TITLE'
// --> peer list limited max. 20.
//     so, we might use different address books for each node to cover a network of more than 20 nodes.
//
//==========</list-of-configurations>==========
//
#define SLIPSERIAL_ACTIVE

//============<parameters>============
//
#define LED_PERIOD (11111)
#define LED_ONTIME (1)
#define LED_GAPTIME (222)
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
#if defined(SERIAL_SWAP) || defined(SLIPSERIAL_ACTIVE)
#define MONITORING_SERIAL (Serial1)
#else
#define MONITORING_SERIAL (Serial)
#endif
//
//============</parameters>===========

//============<board-specifics>============
#if defined(ARDUINO_FEATHER_ESP32) // featheresp32
#define LED_PIN 13
#elif defined(ARDUINO_ESP32_DEV) // esp32dev (MakePython ESP32 => Have NO LED)
#define LED_PIN 2
#else
#define LED_PIN 2
#endif
//============</board-specifics>===========

//arduino
#include <Arduino.h>

//post & addresses
#include "../../post.h"

//espnow
#if defined(ESP8266) // for esp8266 API
#include <ESP8266WiFi.h>
#include <espnow.h>
#elif defined(ESP32) // for esp32 API
#include <WiFi.h>
#include <esp_now.h>
#endif
AddressLibrary lib;

//task
#include <TaskScheduler.h>
Scheduler runner;

//osc
#include <OSCBundle.h>
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial);
void swap_println(String abc) {
#if defined(ESP8266) // for esp8266 API

  Serial.flush();

  Serial.swap();
  Serial.println(abc);
  Serial.flush();

  Serial.swap();
#endif
}

#if defined(ESP32) // for esp32 API
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
//
//screen task
String screen_text = "(.........................................";
extern Task screen_task;
void screen() {
  //clear screen + a
  int line_step = 12;
  int line = 0;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  //line1 - debug line
  display.setCursor(0, line);
  display.println(screen_text.c_str());
  line += line_step;

  //
  display.display();
  //
}
Task screen_task(SCREEN_PERIOD, TASK_FOREVER, &screen, &runner, true);
#endif
//*-*-*-*-*-*-*-*-*-*-*-*-*

//
extern Task hello_task;
static int hello_delay = 0;
void hello() {
  //
  byte mac[6];
  WiFi.macAddress(mac);
  uint32_t mac32 = (((((mac[2] << 8) + mac[3]) << 8) + mac[4]) << 8) + mac[5];
  //
  Hello hello(String(MY_SIGN), MY_ID, mac32); // the most basic 'hello'
  // and you can append some floats
  static int count = 0;
  count++;
  hello.h1 = (count % 1000);
  // hello.h2 = 0;
  // hello.h3 = 0;
  // hello.h4 = 0;
  //
  uint8_t frm_size = sizeof(Hello) + 2;
  uint8_t frm[frm_size];
  frm[0] = '{';
  memcpy(frm + 1, (uint8_t *) &hello, sizeof(Hello));
  frm[frm_size - 1] = '}';
  //
  esp_now_send(NULL, frm, frm_size); // to all peers in the list.
  //
  MONITORING_SERIAL.write(frm, frm_size);
  MONITORING_SERIAL.println(" ==(esp_now_send/0)==> ");
  //
  if (hello_delay > 0) {
    if (hello_delay < 100) hello_delay = 100;
    hello_task.restartDelayed(hello_delay);
  }
}
Task hello_task(0, TASK_ONCE, &hello, &runner, false);

//task #0 : blink led
extern Task blink_task;
void blink() {
  //
  static int count = 0;
  count++;
  //
  switch (count % 4) {
  case 0:
    digitalWrite(LED_PIN, LOW); // first ON
    blink_task.delay(LED_ONTIME);
    break;
  case 1:
    digitalWrite(LED_PIN, HIGH); // first OFF
    blink_task.delay(LED_GAPTIME);
    break;
  case 2:
    digitalWrite(LED_PIN, LOW); // second ON
    blink_task.delay(LED_ONTIME);
    break;
  case 3:
    digitalWrite(LED_PIN, HIGH); // second OFF
    blink_task.delay(LED_PERIOD - 2* LED_ONTIME - LED_GAPTIME);
    break;
  }
}
Task blink_task(0, TASK_FOREVER, &blink, &runner, true); // -> ENABLED, at start-up.

//task #1 : osc processing
void route_note(OSCMessage& msg, int offset) {
  //swap_println("got route_note!");
  // matches will happen in the order. that the bundle is packed.
  static Note note;
  // (1) --> /onoff
  if (msg.fullMatch("/onoff", offset)) {
    //
    note.clear();
    //
    note.onoff = msg.getFloat(0);
    if (note.onoff != 0) note.onoff = 1;
  }
  // (2) --> /velocity
  if (msg.fullMatch("/velocity", offset)) {
    note.velocity = msg.getFloat(0);
  }
  // (3) --> /pitch
  if (msg.fullMatch("/pitch", offset)) {
    note.pitch = msg.getFloat(0);
  }
  // (4) --> /id
  if (msg.fullMatch("/id", offset)) {
    note.id = msg.getInt(0);
  }
  // (5) --> /x
  if (msg.fullMatch("/x", offset)) {
    note.x1 = msg.getFloat(0);
    note.x2 = msg.getFloat(1);
    note.x3 = msg.getFloat(2);
    note.x4 = msg.getFloat(3);
    note.ps = msg.getFloat(4);
    //
    uint8_t frm_size = sizeof(Note) + 2;
    uint8_t frm[frm_size];
    frm[0] = '[';
    memcpy(frm + 1, (uint8_t *) &note, sizeof(Note));
    frm[frm_size - 1] = ']';
    //
    esp_now_send(NULL, frm, frm_size); // to all peers in the list.
    //
    MONITORING_SERIAL.write(frm, frm_size);
    MONITORING_SERIAL.println(" ==(esp_now_send/0)==> ");
    //
  }
}
extern Task osc_task;
void osc()
{
  //osc
  OSCBundle bundleIN;
  int size;
  if (SLIPSerial.available()) {
    while(!SLIPSerial.endofPacket()) {
      if( (size = SLIPSerial.available()) > 0) {
        while(size--) {
          bundleIN.fill(SLIPSerial.read());
        }
      }
    }
    if(!bundleIN.hasError()) {
      // on '/note'
      bundleIN.route("/note", route_note);
#if defined(ESP32)
      static int a = 0;
      screen_text = String(a) + " => \n" + String(bundleIN.timetag.seconds-2208988800UL) + "\n" + String(bundleIN.timetag.fractionofseconds);
      a++;
#endif
    }
  }
}
Task osc_task(0, TASK_FOREVER, &osc, &runner, true); // -> ENABLED, at start-up.

// on 'receive'
#if defined(ESP32)
void onDataReceive(const uint8_t * mac, const uint8_t *incomingData, int32_t len) {
#elif defined(ESP8266)
void onDataReceive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
#endif

  //
  //MONITORING_SERIAL.write(incomingData, len);

  // open => identify => use.
  if (incomingData[0] == '{' && incomingData[len - 1] == '}' && len == (sizeof(Hello) + 2)) {
    Hello hello("");
    memcpy((uint8_t *) &hello, incomingData + 1, sizeof(Hello));
    //
    MONITORING_SERIAL.println(hello.to_string());
    //
    OSCMessage osc("/hello");
    osc.add(hello.id);
    osc.add(hello.h1);
    osc.add(hello.h2);
    osc.add(hello.h3);
    osc.add(hello.h4);
    //
    SLIPSerial.beginPacket();
    osc.send(SLIPSerial);
    SLIPSerial.endPacket();
    osc.empty();
    //
  }

  // open => identify => use.
  if (incomingData[0] == '[' && incomingData[len - 1] == ']' && len == (sizeof(Note) + 2)) {
    Note note;
    memcpy((uint8_t *) &note, incomingData + 1, sizeof(Note));
    //
    MONITORING_SERIAL.println(note.to_string());
    //
    //-*-*-*-*-*-*-*-*-*-
    // use 'note' here...
    //   ==> N.B.: "callback function runs from a high-priority Wi-Fi task.
    //              So, do not do lengthy operations in the callback function.
    //              Instead, post the necessary data to a queue and handle it from a lower priority task."
    //-*-*-*-*-*-*-*-*-*-
  }
}

// on 'sent'
#if defined(ESP32)
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
#elif defined(ESP8266)
void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
#endif
  char buff[256] = "";
  sprintf(buff, "Delivery failed! -> %02X:%02X:%02X:%02X:%02X:%02X",  mac_addr[0],  mac_addr[1],  mac_addr[2],  mac_addr[3],  mac_addr[4],  mac_addr[5]);
  if (sendStatus != 0) MONITORING_SERIAL.println(buff);
}

#if defined(ESP32)
void lcd_text(String str) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(str.c_str());
  display.display();
}
#endif

//
void setup() {

  //led
  pinMode(LED_PIN, OUTPUT);

  //serial
  Serial.begin(57600);
  delay(100);

#if defined(ESP32)
  //screen
  Wire.begin(MAKEPYTHON_ESP32_SDA, MAKEPYTHON_ESP32_SCL);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;         // Don't proceed, loop forever
  }
  display.clearDisplay();
#endif

  //info
  Serial.println();
  Serial.println();
  Serial.println("\"hi, i m your postman.\"");
  Serial.println("-");
  Serial.println("- my id: " + String(MY_ID) + ", gid: " + String(MY_GROUP_ID) + ", call me ==> \"" + String(MY_SIGN) + "\"");
  Serial.println("- mac address: " + WiFi.macAddress() + ", channel: " + String(WIFI_CHANNEL));
  Serial.println("- my peer book ==> \"" + String(ADDRESSBOOK_TITLE) + "\"");
#if defined(HAVE_CLIENT)
  Serial.println("- ======== 'HAVE_CLIENT' ========");
#endif
#if defined(SERIAL_SWAP)
  Serial.println("- ======== 'SERIAL_SWAP' ========");
#endif
#if defined(DISABLE_AP)
  Serial.println("- ======== 'DISABLE_AP' ========");
#endif
#if defined(HAVE_CLIENT_I2C)
  Serial.println("- ======== 'HAVE_CLIENT_I2C' ========");
#endif
#if defined(SLIPSERIAL_ACTIVE)
  Serial.println("- ======== 'SLIPSERIAL_ACTIVE' ========");
#endif
  Serial.println("-");

  //wifi
  WiFiMode_t node_type = WIFI_AP_STA;
#if defined(DISABLE_AP)
  system_phy_set_max_tpw(0);
  node_type = WIFI_STA;
#endif
  WiFi.mode(node_type);

  //esp-now
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
#if defined(ESP8266)
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
#endif
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);
  //

  AddressBook * book = lib.getBookByTitle(ADDRESSBOOK_TITLE);
  if (book == NULL) {
    Serial.println("- ! wrong book !! : \"" + String(ADDRESSBOOK_TITLE) + "\""); while(1);
  }
  for (int idx = 0; idx < book->list.size(); idx++) {
    Serial.println("- ! (esp_now_add_peer) ==> add a '" + book->list[idx].name + "'.");
#if defined(ESP32)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, book->list[idx].mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
#else
    esp_now_add_peer(book->list[idx].mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
#endif
  }
  // (DEBUG) fetch full peer list
  { PeerLister a; a.print(); }
  //
  Serial.println("-");
  Serial.println("\".-.-.-. :)\"");
  Serial.println();

  // a proper say goodbye.
  Serial.println("\"bye, this 'Serial' will be occupied by SLIPSerial. find me @ Serial1 (TX only)\"");
  Serial.println("-");
  Serial.println("\".-.-.-. :)\"");
  delay(1000);   // flush out unsent serial messages.

  //
  SLIPSerial.begin(57600);
  delay(100);
}

void loop() {
  //
  runner.execute();
  //
}
