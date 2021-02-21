//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA warehouses, Seoul
//

//
// 2021 02 15
//
//   this module will be an esp-now node in a group.
//       like, a bird in a group of birds.
//
//   esp-now @ esp8266 w/ broadcast address (FF:FF:FF:FF:FF:FF)
//       always broadcasting. everyone is 'talkative'.
//

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
#if 1
// (2) osc client (the ROOT)
#elif 0
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

//============<identity key>============
#define ID_KEY 151
//============</identity key>===========

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

//post & addresses
#include "../../post.h"

//espnow
#include <ESP8266WiFi.h>
#include <espnow.h>

//task
#include <TaskScheduler.h>
Scheduler runner;

//-*-*-*-*-*-*-*-*-*-*-*-*-
void taak_on() {
  Serial.println("taak_on!");
  digitalWrite(D6, HIGH);
}
Task taak_on_task(0, TASK_ONCE, &taak_on);

void taak_off() {
  Serial.println("taak_off!");
  digitalWrite(D6, LOW);
}
Task taak_off_task(0, TASK_ONCE, &taak_off);
//*-*-*-*-*-*-*-*-*-*-*-*-*

//
extern Task hello_task;
static int hello_delay = 0;
void hello() {
  //
  Hello hello = {
    ID_KEY
  };
  //
  uint8_t frm_size = sizeof(Hello) + 2;
  uint8_t frm[frm_size];
  frm[0] = '{';
  memcpy(frm + 1, (uint8_t *) &hello, sizeof(Hello));
  frm[frm_size - 1] = '}';
  //
  esp_now_send(NULL, frm, frm_size); // just broadcast.
  //
  MONITORING_SERIAL.write(frm, frm_size);
  MONITORING_SERIAL.println(" ==(esp_now_send/BROADCAST)==> ");
  //
  if (hello_delay > 0) {
    if (hello_delay < 100) hello_delay = 100;
    hello_task.restartDelayed(hello_delay);
  }
}
Task hello_task(0, TASK_ONCE, &hello);

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

// on 'Note'
void onNoteHandler(Note & n) {
  //is it for me?
  if (n.pitch == ID_KEY) {
    //
    if (n.velocity < 0) n.velocity = 0;
    //taak_on && taak_off
    if (n.velocity == 0) {
      if (n.onoff == 1) {
        taak_on_task.restartDelayed(10);
      } else {
        taak_off_task.restartDelayed(10);
      }
    }
    //taak_hit
    else {
      taak_on_task.restartDelayed(10);
      taak_off_task.restartDelayed(10 + n.velocity * 2);
    }
  }
}

// on 'receive'
void onDataReceive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {

  //
#if defined(HAVE_CLIENT)
  Serial.write(incomingData, len); // we share it w/ the client.
#endif

  // on 'Note'
  if (incomingData[0] == '[' && incomingData[len - 1] == ']' && len == (sizeof(Note) + 2)) {
    Note note;
    memcpy((uint8_t *) &note, incomingData + 1, sizeof(Note));
    //
    MONITORING_SERIAL.println(note.to_string());
    //
    onNoteHandler(note);
    //
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
  Serial.begin(115200);
  delay(100);

  //info
  Serial.println();
  Serial.println();
  Serial.println("\"hi, i m your postman.\"");
  Serial.println("-");
  Serial.println("- * info >>>");
#if defined(ID_KEY)
  Serial.println("-      identity (key): " + String(ID_KEY));
#endif
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
  //
  Serial.println("-");
  Serial.println("- we will broadcast everything. ==> add only the 'broadcast peer' (FF:FF:FF:FF:FF:FF).");
  uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_add_peer(broadcastmac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

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

  //pinmode
  pinMode(D6, OUTPUT);

  //tasks
  runner.addTask(hello_task);
  runner.addTask(taak_on_task);
  runner.addTask(taak_off_task);
}

void loop() {
  //
  runner.execute();
  //
}
