//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA warehouses, Seoul
//

//
// 2021 02 15
//
// (part-1) esp8266 : 'postman' (the esp-now network nodes)
//
//   this module will be an esp-now node in a group.
//       like, a bird in a group of birds.
//
//   esp-now @ esp8266 DO support broadcast address (FF:FF:FF:FF:FF:FF)
//       w/ NONOS-SDK of espressif
//   and you can enable that w/ Platformio, applying some special build flags.
//       --> https://github.com/esp8266/Arduino/issues/6174#issuecomment-509115454
//           (yet, w/ Arduino, this is not available yet.)
//
//   so, at first, we will simply/stably go w/o broadcasting.
//   and, if broadcast is really needed we can activate (@Platformio)
//


// we want to first osc -> esp-now
// then, esp-now based taak
// then, let is save a value in EEPROM (object with memory)
// no broadcast for now. if needed we can achieve that too.



//==========<configurations>===========
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
//==========</configurations>==========

//==========<preset>===========
//
// (1) standalone
#if 0
// (2) osc client (the ROOT)
#elif 1
#define SERIAL_SWAP
#define HAVE_CLIENT
// (3) sampler client
#elif 0
#define SERIAL_SWAP
#define HAVE_CLIENT
#define DISABLE_AP
//
#endif
//
//==========</preset>==========

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

//post
#include "../../post.h"

//addresses
#include <Vector.h>
Vector<Address> members;
Address __members[MEMBER_COUNT_MAX]; //<-- the storage array of 'members'

//espnow
#include <ESP8266WiFi.h>
#include <espnow.h>

// on 'sent'
void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus != 0) MONITORING_SERIAL.println("Delivery failed!");
}

// on 'receive'
void onDataReceive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {

  //
#if defined(HAVE_CLIENT)
  Serial.write(incomingData, len); // we pass it over to the client.
#endif

  // open => identify => use.
  if (incomingData[0] == '[' && incomingData[len - 1] == ']' && len == (sizeof(Note) + 2)) {
    Note note;
    memcpy((uint8_t *) &note, incomingData + 1, sizeof(Note));
    //
    MONITORING_SERIAL.println(note.to_string());

    //-*-*-*-*-*-*-*-*-*-
    // use 'note' here...
    //-*-*-*-*-*-*-*-*-*-
  }
}

//task
#include <TaskScheduler.h>
Scheduler runner;

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

//task #1 : regular post collection
#if defined(HAVE_CLIENT)
void collect_post() {
  //
  //postman (serial comm.)
  static bool insync = false;
  if (insync == false) {
    while (Serial.available() > 0) {
      // search the last byte
      char last = Serial.read();
      // expectable last of the messages
      if (last == ']' || last == '}') {
        insync = true;
      }
    }
  } else {
    //
    if (Serial.available() > 0) {
      //
      char type = Serial.peek();
      //
      if (type == '[') {
        //expecting a Note message.
        uint8_t frm_size = sizeof(Note) + 2;
        //
        if (Serial.available() >= frm_size) {
          //
          uint8_t frm[frm_size];
          //
          Serial.readBytes(frm, frm_size);
          char first = frm[0];
          char last = frm[frm_size - 1];
          if (first == '[' && last == ']') {
            //
            //good. ==> ok, post it.
            //
            //pseudo-broadcast using peer-list!
            //
            esp_now_send(NULL, frm, frm_size);
            //
            MONITORING_SERIAL.write(frm, frm_size);
            MONITORING_SERIAL.print(" ==(esp_now_send/0)==> ");
            //
          } else {
            insync = false; //error!
          }
        }
      }
    }
  }
}
Task collect_post_task(1, TASK_FOREVER, &collect_post, &runner, true); // by default, ENABLED
#endif

//
void setup() {

  //led
  pinMode(LED_PIN, OUTPUT);

  //serial
  Serial.begin(115200);
  delay(100);

  //members
  members.setStorage(__members);

  //
  members.push_back(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0x21, "Enchovy"));
  members.push_back(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB3, 0xC5, "Schpaarow"));
  members.push_back(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB4, 0x28, "Taak157"));

  //info
  Serial.println();
  Serial.println();
  Serial.println("\"hi, i m your postman.\"");
  Serial.println("-");
  Serial.println("- * info >>>");
  Serial.println("-      mac address: " + WiFi.macAddress());
  Serial.println("-      wifi channel: " + String(WIFI_CHANNEL));
  Serial.println("-");
  Serial.println("- * conf >>>");
#if defined(HAVE_CLIENT)
  Serial.println("-      ======== 'HAVE_CLIENT' ========");
#endif
#if defined(SERIAL_SWAP)
  Serial.println("-      ======== 'SERIAL_SWAP' ========");
#endif
#if defined(DISABLE_AP)
  Serial.println("-      ======== 'DISABLE_AP' ========");
#endif
  Serial.println("-");
  Serial.println("- * addresses >>>");
  for (uint32_t i = 0; i < members.size(); i++) {
    Serial.print("-      #" + String(i) + " : ");
    Serial.print(members[i].mac[0], HEX);
    for (int j = 1; j < 6; j++) {
      Serial.print(":");
      Serial.print(members[i].mac[j], HEX);
    }
    Serial.print(" ==> " + members[i].name);
    Serial.println();
  }
  Serial.println("-");
  Serial.println("\".-.-.-. :)\"");
  Serial.println();

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
  for (uint32_t i = 0; i < members.size(); i++) {
    esp_now_add_peer(members[i].mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0); // <-- '1' : "Channel does not affect any function" ... *.-a
    //
    // int esp_now_add_peer(u8 *mac_addr, u8 role, u8 channel, u8 *key, u8 key_len)
    //     - https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
    //
    // "Channel does not affect any function, but only stores the channel information
    // for the application layer. The value is defined by the application layer. For
    // example, 0 means that the channel is not defined; 1 ~ 14 mean valid
    // channels; all the rest values can be assigned functions that are specified
    // by the application layer."
    //     - https://www.espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf
  }

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
}

void loop() {
  //
  runner.execute();
  //
}
