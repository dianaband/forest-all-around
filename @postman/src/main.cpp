//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA storage, Seoul
//

//
// 2021 02 15
//
// (part-1) esp8266 : 'postman' (the esp-now network nodes)
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
#define MY_GROUP_ID   (10000)
#define MY_ID         (MY_GROUP_ID + 1)
#define MY_SIGN       ("@POSTMAN|@SAMPLER")
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
// 'DISABLE_AP'
// --> (questioning)...
//
// 'REPLICATE_NOTE_REQ' (+ N_SEC_BLOCKING_NOTE_REQ)
// --> for supporting wider area with simple esp_now protocol,
//     all receipents will replicate NOTE msg. when they are newly appeared.
//   + then, network would be flooded by infinite duplicating msg.,
//     unless they stop reacting to 'known' req. for some seconds. (e.g. 3 seconds)

// 'HAVE_CLIENT_I2C'
// --> i have a client w/ I2C i/f. enable the I2C client task.
//
//==========</list-of-configurations>==========
//
#define HAVE_CLIENT_I2C
#define DISABLE_AP
#define REPLICATE_NOTE_REQ

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
#if defined(SERIAL_SWAP)
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

//i2c
#include <Wire.h>

//post & addresses
#include "../../post.h"

//espnow
#include <ESP8266WiFi.h>
#include <espnow.h>
AddressLibrary lib;

//task
#include <TaskScheduler.h>
Scheduler runner;

//
#if defined(REPLICATE_NOTE_REQ)
Note note_now = {
  -1, // int32_t id;
  -1, // float pitch;
  -1, // float velocity;
  -1, // float onoff;
  -1, // float x1;
  -1, // float x2;
  -1, // float x3;
  -1, // float x4;
  -1 // float ps;
};
#define NEW_NOTE_TIMEOUT (3000)
static unsigned long new_note_time = (-1*NEW_NOTE_TIMEOUT);
void repeat() {
  //
  uint8_t frm_size = sizeof(Note) + 2;
  uint8_t frm[frm_size];
  frm[0] = '[';
  memcpy(frm + 1, (uint8_t *) &note_now, sizeof(Note));
  frm[frm_size - 1] = ']';
  //
  esp_now_send(NULL, frm, frm_size); // to all peers in the list.
  //
  MONITORING_SERIAL.print("repeat! ==> ");
  MONITORING_SERIAL.println(note_now.to_string());
}
Task repeat_task(0, TASK_ONCE, &repeat, &runner, false);
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
  // MONITORING_SERIAL.write(frm, frm_size);
  // MONITORING_SERIAL.println(" ==(esp_now_send/0)==> ");
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

// on 'receive'
void onDataReceive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {

  //
  //MONITORING_SERIAL.write(incomingData, len);

  //
#if defined(HAVE_CLIENT)
  Serial.write(incomingData, len); // we pass it over to the client.
#endif

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

    #if defined(REPLICATE_NOTE_REQ)
    if (millis() - new_note_time > NEW_NOTE_TIMEOUT) {
      note_now = note;
      repeat_task.restart();
      new_note_time = millis();
    }
    #endif

    #if defined(HAVE_CLIENT_I2C)

    //is this for me & my client?
    if (note.id == MY_GROUP_ID || note.id == MY_ID) {

      // (struct --> obsolete I2C format.)
      //    --> we want to open & re-construct the msg.

      // letter frame ( '[' + 30 bytes + ']' )
      //    : [123456789012345678901234567890]

      // 'MIDI' letter frame
      //    : [123456789012345678901234567890]
      //    : [KKKVVVG.......................]
      //    : KKK - Key
      //      .substring(1, 4);
      //    : VVV - Velocity (volume/amp.)
      //      .substring(4, 7);
      //    : G - Gate (note on/off)
      //      .substring(7, 8);

      char letter_outro[POST_BUFF_LEN] = "................................";
      sprintf(letter_outro, "[%03d%03d%01d.......................]",
              (int32_t)note.pitch,
              (int32_t)note.velocity,
              (int32_t)note.onoff);
      String msg = String(letter_outro);

      // truncate any extra. letters.
      msg = msg.substring(0, POST_LENGTH); // (0) ~ (POST_LENGTH-1)

      // send out
      Wire.beginTransmission(I2C_ADDR);
      Wire.write(msg.c_str(), POST_LENGTH);
      Wire.endTransmission();

      //
      hello_delay = note.ps;
      if (hello_delay > 0 && hello_task.isEnabled() == false) {
        hello_task.restart();
      }
    }
    #endif
  }
}

// on 'sent'
void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  char buff[256] = "";
  sprintf(buff, "Delivery failed! -> %02X:%02X:%02X:%02X:%02X:%02X",  mac_addr[0],  mac_addr[1],  mac_addr[2],  mac_addr[3],  mac_addr[4],  mac_addr[5]);
  if (sendStatus != 0) MONITORING_SERIAL.println(buff);
}

//
void setup() {

  //led
  pinMode(LED_PIN, OUTPUT);

  //serial
  Serial.begin(115200);
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
#if defined(REPLICATE_NOTE_REQ)
  Serial.println("- ======== 'REPLICATE_NOTE_REQ' ========");
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
  // Serial.println("- ! (esp_now_add_peer) ==> add a 'broadcast peer' (FF:FF:FF:FF:FF:FF).");
  // uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //
  // //
  // esp_now_peer_info_t peerInfo;
  // memcpy(peerInfo.peer_addr, broadcastmac, 6);
  // peerInfo.channel = 0;
  // peerInfo.encrypt = false;
  // esp_now_add_peer(&peerInfo);

  AddressBook * book = lib.getBookByTitle("audioooo");
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

#if defined(SERIAL_SWAP)
  Serial.println("-      ======== 'SERIAL_SWAP' ========");
  // a proper say goodbye.
  Serial.println("\"bye, i will do 'swap' in 1 second. find me on alternative pins!\"");
  Serial.println("\"    hint: osc wiring ==> esp8266(serial.swap) <-> teensy(serial3)\"");
  Serial.println("-");
  Serial.println("\".-.-.-. :)\"");
  delay(1000);   // flush out unsent serial messages.

  // moving...
  Serial.swap(); // use RXD2/TXD2 pins, afterwards.
  delay(100);    // wait re-initialization of the 'Serial'
#endif

  //
#if defined(HAVE_CLIENT_I2C)
  //i2c master
  Wire.begin();
#endif
}

void loop() {
  //
  runner.execute();
  //
}
