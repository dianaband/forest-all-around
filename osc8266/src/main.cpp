//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA storage, Seoul
//

//
// 2021 02 15
//
// (part-1) esp8266 : 'osc node on esp8266' (the esp-now network nodes)
//
//   this module will be an esp-now node in a group.
//       like, a bird in a group of birds.
//
//   esp-now @ esp8266 w/ broadcast address (FF:FF:FF:FF:FF:FF)
//       always broadcasting. everyone is 'talkative'.
//

// then, let it save a value in EEPROM (object with memory=mind?)

//============<identities>============
//
#define MY_GROUP_ID   (0)
#define MY_ID         (MY_GROUP_ID + 2)
#define MY_SIGN       ("POSTMAN|OSC(Pd)")
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
//==========</list-of-configurations>==========
//
#define SLIPSERIAL_ACTIVE

//============<parameters>============
//
#define LED_PERIOD (11111)
#define LED_ONTIME (1)
#define LED_GAPTIME (222)
//
#define WIFI_CHANNEL 5
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
#else
#define LED_PIN 2
#endif
//============</board-specifics>===========

//arduino
#include <Arduino.h>

//post & addresses
#include "../../post.h"

//espnow
#include <ESP8266WiFi.h>
#include <espnow.h>

//task
#include <TaskScheduler.h>
Scheduler runner;

//osc
#include <OSCBundle.h>
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial);
void swap_println(String abc) {
  Serial.flush();

  Serial.swap();
  Serial.println(abc);
  Serial.flush();

  Serial.swap();
}

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
    }
  }
}
Task osc_task(0, TASK_FOREVER, &osc, &runner, true); // -> ENABLED, at start-up.

// on 'receive'
void onDataReceive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {

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
void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus != 0) MONITORING_SERIAL.println("Delivery failed!");
}

//
void setup() {

  //led
  pinMode(LED_PIN, OUTPUT);

  //serial
  Serial.begin(57600);
  delay(100);

  //info
  Serial.println();
  Serial.println();
  Serial.println("\"hi, i m your postman.\"");
  Serial.println("-");
  Serial.println("- my id: " + String(MY_ID) + ", gid: " + String(MY_GROUP_ID) + ", call me ==> \"" + String(MY_SIGN) + "\"");
  Serial.println("- mac address: " + WiFi.macAddress() + ", channel: " + String(WIFI_CHANNEL));
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
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);
  //
  Serial.println("- ! (esp_now_add_peer) ==> add a 'broadcast peer' (FF:FF:FF:FF:FF:FF).");
  uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_add_peer(broadcastmac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

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
