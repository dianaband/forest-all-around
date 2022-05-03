//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// `hing @ dianaband studio, Seoul
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
#define MY_GROUP_ID       (11000)
#define MY_ID             (MY_GROUP_ID + 1)
#define MY_SIGN           ("BELL")
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
// 'ADDRESSBOOK_TITLE'
// --> peer list limited max. 20.
//     so, we might use different address books for each node to cover a network of more than 20 nodes.
//
//==========</list-of-configurations>==========
//
// (EMPTY)
#define DISABLE_AP
// #define REPLICATE_NOTE_REQ

//============<bell>============
#define BELL_HIT_KEY 105
//============</bell>===========

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

//vector
#include <vector>
std::vector<Note> recentNotes;

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
#define RECENT_NOTES_TIMEOUT (3000)
static unsigned long last_note_time = 0;
void recent_clear() {
  //
  if (millis() - last_note_time > RECENT_NOTES_TIMEOUT) {
    recentNotes.clear();
    Serial.println("recent list cleared");
    last_note_time = millis();
  }
  //
}
Task recent_clear_task(100, TASK_FOREVER, &recent_clear, &runner, true);
#endif
//-*-*-*-*-*-*-*-*-*-*-*-*-
// servo
#include <Servo.h>

#define SERVO_PIN D6
Servo myservo;
int hitting_angle = 90;
int release_angle = 60;
int stabilize_angle = 53;

//
extern Task hit_task;

//
extern Task pcontrol_task;
bool pcontrol_new = false;
int pcontrol_start = 0;
int pcontrol_target = 0;
int control_count = 0;

//
extern Task servo_release_task;

// my tasks
// hit!
void hit() {
  static int count = 0;
  if (hit_task.isFirstIteration()) {
    count = 0;
    Serial.println("hit! start.");
  }
  if (count % 3 == 0) {
    //
    myservo.attach(SERVO_PIN);
    myservo.write(release_angle);
    // servo_release_task.restartDelayed(200);
    //
  } else if (count % 3 == 1) {
    //
    myservo.attach(SERVO_PIN);
    myservo.write(hitting_angle);
    // servo_release_task.restartDelayed(200);
    //
    Serial.print("bell, bell, bell! : ");
    Serial.print(hitting_angle);
    Serial.println(" deg.");
    //
  } else {
    //
    myservo.attach(SERVO_PIN);
    myservo.write(release_angle);
    servo_release_task.restartDelayed(200);
    //
    Serial.print("release to .. : ");
    Serial.print(release_angle);
    Serial.println(" deg.");
    // start stablizing..
    pcontrol_new = true;
    pcontrol_start = release_angle;
    pcontrol_target = stabilize_angle;
    pcontrol_task.restartDelayed(80);
    //
    control_count = 0;
  }
  //
  count++;
}
Task hit_task(100, 3, &hit);

// pcontrol
void pcontrol() {
  static int angle;
  if (pcontrol_new == true) {
    pcontrol_new = false;
    angle = pcontrol_start;
  }
  int error = pcontrol_target - angle;
  int sign = (error >= 0 ? 1 : -1);
  //
  Serial.print("step-by-step to.. : ");
  Serial.println(sign);
  //
  if (error != 0) {
    angle = angle + sign; // most gentle move : 1 step each time.
    //
    Serial.print("stablizing in action ==> next angle : ");
    Serial.print(angle);
    Serial.println(" deg.");
    //
    myservo.attach(SERVO_PIN);
    myservo.write(angle);
    servo_release_task.restartDelayed(50);
    pcontrol_task.restartDelayed(400);
  }
  else {
    // stand-by processes
    if (control_count % 2 == 0) {
      pcontrol_new = true;
      pcontrol_start = stabilize_angle;
      pcontrol_target = release_angle;
      pcontrol_task.restartDelayed(300);
    } else if (control_count % 2 == 1) {
      pcontrol_new = true;
      pcontrol_start = release_angle;
      pcontrol_target = stabilize_angle;
      pcontrol_task.restartDelayed(300);
    }
    //
    control_count++;
  }
}
Task pcontrol_task(0, TASK_ONCE, &pcontrol); // hit -> 100ms -> step back -> 50ms -> slowly move to rest pos.

// pcontrol release
void servo_release() {
  myservo.detach();
}
Task servo_release_task(0, TASK_ONCE, &servo_release);
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

// on 'Note'
void onNoteHandler(Note & n) {
  //is it for me?
  if (n.id == MY_GROUP_ID || n.id == MY_ID) {
    //
    if (n.pitch == BELL_HIT_KEY) {
      if (n.onoff == 1) {
        hitting_angle = n.velocity;
        release_angle = n.x1;
        stabilize_angle = n.x2;
        hit_task.restartDelayed(10);
      }
    }
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

    #if defined(REPLICATE_NOTE_REQ)
    last_note_time = millis(); //clear timer reset : the recent list holding (re)started
    // check if this note is in the list?
    bool check = false;
    for (uint32_t idx = 0; idx < recentNotes.size(); idx++) {
      if (recentNotes[idx].pitch == note.pitch && recentNotes[idx].id == note.id) {
        check = true;
      }
    }
    // if not, add this into the list and repeat!
    if (check == false) {
      //
      recentNotes.push_back(note);
      //
      uint8_t frm_size = sizeof(Note) + 2;
      uint8_t frm[frm_size];
      frm[0] = '[';
      memcpy(frm + 1, (uint8_t *) &note, sizeof(Note));
      frm[frm_size - 1] = ']';
      //
      esp_now_send(NULL, frm, frm_size); // to all peers in the list.
      //
      MONITORING_SERIAL.print("repeat! ==> ");
      MONITORING_SERIAL.println(note.to_string());
      //
    }

    //EMERGENCY PATCH.HACK:
    // original code is not intended for a BURST of notes.
    // so, only 1 msg. will be repeated in 3 sec. all others will be simply ignored.
    // to make a burst of msgs repeatible:
    // --> make a list of recent 'pitches'
    //     if there is any new msg. check if this is in the list, if not, add it & repeat, if yes, skip it.
    //     after 3sec no new msg., the list will be flushed. every new msg. will reset timeout + save the list for extra. 3 sec.
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
  // esp_now_add_peer(broadcastmac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

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

  //tasks
  runner.addTask(hit_task);
  runner.addTask(pcontrol_task);
  runner.addTask(servo_release_task);
}

void loop() {
  //
  runner.execute();
  //
}
