//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA warehouses, Seoul
//

//
// 2021 02 15
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

//============<list of reserved keys>============
#define CRICKET_A_KEY 120 // geared (esp8266)
#define CRICKET_E_KEY 121 // geared (esp8266)
#define CRICKET_I_KEY 122 // geared (esp8266)
#define CRICKET_O_KEY 123 // geared (esp8266) // servo pin is different (D7)
#define CRICKET_U_KEY 124
#define CRICKET_W_KEY 125
#define CRICKET_Y_KEY 126
#define CRICKET_N_KEY 127 // fishing-fly (esp32)
//============</list of reserved keys>===========

//============<identity key>============
#define ID_KEY 128
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
AddressBook members;

//espnow
#include <ESP8266WiFi.h>
#include <espnow.h>

//task
#include <TaskScheduler.h>
Scheduler runner;

//-*-*-*-*-*-*-*-*-*-*-*-*-
// servo
#define SERVO_PIN 12 //D6
#include <Servo.h>
// my tasks
int speed = 0;
void set_speed() {
  int r = speed;
  analogWrite(SERVO_PIN, r);
  MONITORING_SERIAL.print("set_speed:");
  MONITORING_SERIAL.println(r);
}
Task set_speed_task(0, TASK_ONCE, &set_speed);
//
void rest() {
  analogWrite(SERVO_PIN, 0);
}
Task rest_task(0, TASK_ONCE, &rest);
//*-*-*-*-*-*-*-*-*-*-*-*-*

//
extern Task hello_task;
static int hello_delay = 0;
void hello() {
  //
  Hello hello = {
    ID_KEY,
    speed
  };
  //
  uint8_t frm_size = sizeof(Hello) + 2;
  uint8_t frm[frm_size];
  frm[0] = '{';
  memcpy(frm + 1, (uint8_t *) &hello, sizeof(Hello));
  frm[frm_size - 1] = '}';
  //
  //pseudo-broadcast using peer-list!
  //
  esp_now_send(AddressBook("root").list[0].mac, frm, frm_size);
  //
  MONITORING_SERIAL.write(frm, frm_size);
  MONITORING_SERIAL.println(" ==(esp_now_send/\"root\")==> ");
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
    speed = n.velocity;
    // if (speed < 0) speed = 0;
    //
    if (n.onoff == 1) {
      set_speed_task.restartDelayed(10);
    } else if (n.onoff == 0) {
      rest_task.restartDelayed(10);
    }
    //
  }
}

// on 'receive'
void onDataReceive(uint8_t * mac, uint8_t *incomingData, uint8_t len) {

#if defined(HAVE_CLIENT)
  Serial.write(incomingData, len); // we share it w/ the client.
#endif

  // on 'Note'
  if (incomingData[0] == '[' && incomingData[len - 1] == ']' && len == (sizeof(Note) + 2)) {
    //
    Note note;
    memcpy((uint8_t *) &note, incomingData + 1, sizeof(Note));
    onNoteHandler(note);
    MONITORING_SERIAL.println(note.to_string());
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
  Serial.println("- * addresses >>>");
  for (uint32_t i = 0; i < members.list.size(); i++) {
    Serial.print("-      #" + String(i) + " : ");
    Serial.print(members.list[i].mac[0], HEX);
    for (int j = 1; j < 6; j++) {
      Serial.print(":");
      Serial.print(members.list[i].mac[j], HEX);
    }
    Serial.print(" ==> " + members.list[i].name);
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
  for (uint32_t i = 0; i < members.list.size(); i++) {
    esp_now_add_peer(members.list[i].mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0); // <-- '1' : "Channel does not affect any function" ... *.-a
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

  //random seed
  randomSeed(analogRead(0));

  //tasks
  runner.addTask(set_speed_task);
  runner.addTask(rest_task);
  runner.addTask(hello_task);

  rest_task.restartDelayed(500);
}

void loop() {
  //
  runner.execute();
  //
}
