//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA storage, Seoul
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

// then, let it save a value in EEPROM (object with memory=mind?)

//============<identities>============
//
#define MY_GROUP_ID   (4000)
#define MY_ID         (MY_GROUP_ID + 401)
#define MY_SIGN       ("ROLLER2")
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
// 'HAVE_CLIENT_I2C'
// --> i have a client w/ I2C i/f. enable the I2C client task.
//
//==========</list-of-configurations>==========
//
// (EMPTY)

//============<parameters>============
//
#define LED_PERIOD (11111)
#define LED_ONTIME (1)
#define LED_GAPTIME (222)
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
// servo
#define MOTOR_1A (D6)
#define MOTOR_1B (D5)
// my tasks
int speed = 0;
bool isactive = false;
void set_speed() {
  int r = speed;
  //
  if (r >= 0) {
    digitalWrite(MOTOR_1A, LOW);
    analogWrite(MOTOR_1B, r);
  } else {
    digitalWrite(MOTOR_1B, LOW);
    analogWrite(MOTOR_1A, r*(-1));
  }
  MONITORING_SERIAL.print("set_speed:");
  MONITORING_SERIAL.println(r);
  isactive = true;
}
Task set_speed_task(0, TASK_ONCE, &set_speed, &runner, false);
//
void rest() {
  analogWrite(MOTOR_1A, LOW);
  analogWrite(MOTOR_1B, LOW);
  isactive = false;
}
Task rest_task(0, TASK_ONCE, &rest, &runner, false);
//
uint8_t watch_counter = 0;
void watcher() {
  if (isactive) {
    if (watch_counter > 3) {
      rest_task.restartDelayed(10);
      watch_counter = 0;
    } else {
      watch_counter++;
    }
  }
}
Task watcher_task(1000, TASK_FOREVER, &watcher, &runner, true);
//
#define MOTOR_2A (D3)
#define MOTOR_2B (D2)
// my tasks
int speed2 = 0;
bool isactive2 = false;
void set_speed2() {
  int r = speed2;
  //
  if (r >= 0) {
    digitalWrite(MOTOR_2A, LOW);
    analogWrite(MOTOR_2B, r);
  } else {
    digitalWrite(MOTOR_2B, LOW);
    analogWrite(MOTOR_2A, r*(-1));
  }
  MONITORING_SERIAL.print("set_speed2:");
  MONITORING_SERIAL.println(r);
  isactive2 = true;
}
Task set_speed2_task(0, TASK_ONCE, &set_speed2, &runner, false);
//
void rest2() {
  analogWrite(MOTOR_2A, LOW);
  analogWrite(MOTOR_2B, LOW);
  isactive2 = false;
}
Task rest2_task(0, TASK_ONCE, &rest2, &runner, false);
//
uint8_t watch2_counter = 0;
void watcher2() {
  if (isactive2) {
    if (watch2_counter > 3) {
      rest2_task.restartDelayed(10);
      watch2_counter = 0;
    } else {
      watch2_counter++;
    }
  }
}
Task watcher2_task(1000, TASK_FOREVER, &watcher2, &runner, true);
//*-*-*-*-*-*-*-*-*-*-*-*-*

//
extern Task hello_task;
static int hello_delay = 2000;
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
  hello.h2 = speed;
  hello.h3 = (isactive ? 1 : 0);
  hello.h4 = speed2;
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

// on 'Note'
void onNoteHandler(Note & n) {
  //is it for me?
  if (n.id == MY_GROUP_ID || n.id == MY_ID) {
    //
    if (n.pitch == 0) {
      speed = n.velocity;
      //
      if (n.onoff == 1) {
        set_speed_task.restartDelayed(10);
        watch_counter = 0;
      } else if (n.onoff == 0) {
        rest_task.restartDelayed(10);
      } else if (n.onoff == 2) {
        set_speed_task.restartDelayed(10);
        rest_task.restartDelayed(10 + n.x1);
      }
    }
    //
    else if (n.pitch == 1) {
      speed2 = n.velocity;
      //
      if (n.onoff == 1) {
        set_speed2_task.restartDelayed(10);
        watch2_counter = 0;
      } else if (n.onoff == 0) {
        rest2_task.restartDelayed(10);
      } else if (n.onoff == 2) {
        set_speed2_task.restartDelayed(10);
        rest2_task.restartDelayed(10 + n.x1);
      }
    }
    //
  }
}

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
    onNoteHandler(note);

    //is it for me?
    if (note.id == MY_GROUP_ID || note.id == MY_ID) {
      hello_delay = note.ps;
      if (hello_delay > 0 && hello_task.isEnabled() == false) {
        hello_task.restart();
      }
    }

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

  //pwm freq.
  analogWriteFreq(40000);

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
  rest_task.restartDelayed(500);
  rest2_task.restartDelayed(500);
}

void loop() {
  //
  runner.execute();
  //
}
