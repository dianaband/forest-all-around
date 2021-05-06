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
#define MY_GROUP_ID   (20000)
#define MY_ID         (MY_GROUP_ID + 100)
#define MY_SIGN       ("ROUNDLY")
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
#include <AccelStepper.h>
#define STEP_MODE_CONSTANT_VEL  (0xDE00 + 0x01)
#define STEP_MODE_ACCELERATING  (0xDE00 + 0x02)
#define STEP_MODE               STEP_MODE_CONSTANT_VEL
// #define STEP_MODE               STEP_MODE_ACCELERATING
// NOTE: --> well.. acceleration enabled mode.. is a bit worse. (less torque)
#define STEPS_PER_REV           (2048.0)
// speed (rpm) * steps-per-revolution == speed (steps per minute)
//  --> speed (steps per minute) / 60 == speed (steps per second)
//  --> speed (steps per second) * 60 / steps-per-revolution == speed (rpm)
#define STEPS_PER_SEC_TO_RPM    (60.0 / STEPS_PER_REV)
#define RPM_TO_STEPS_PER_SEC    (STEPS_PER_REV / 60.0)
// parameter (torque-speed trade-off)
#define STEPS_PER_SEC_MAX       (500)
#define RPM_MAX                 (STEPS_PER_SEC_MAX * STEPS_PER_SEC_TO_RPM)
#define ACCELERATION_MAX        (500)
//
AccelStepper stepper(AccelStepper::FULL4WIRE, D5, D6, D7, D8, false); // N.B. - @esp8266, NEVER use "5, 6, 7, 8" -> do "D5, D6, D7, D8" !!

// my tasks
extern Task stepping_task;
extern Task moving_task;
extern Task rest_task;

// 'move' task --> relative movements
int move_target = 0;
int move_duration = 10000;
void moving() {
  //
  float velocity = move_target / move_duration * 1000;   // unit conv.: (steps/msec) --> (steps/sec)
  float speed = fabs(velocity);
  //
  MONITORING_SERIAL.print("move_target --> "); MONITORING_SERIAL.println(move_target);
  MONITORING_SERIAL.print("move_duration --> "); MONITORING_SERIAL.println(move_duration);
  //
  if (speed > STEPS_PER_SEC_MAX) {
    MONITORING_SERIAL.println("oh.. it might be TOO FAST for me..");
  } else {
    MONITORING_SERIAL.println("okay. i m going.");
  }

  //
  stepper.enableOutputs();
  stepper.moveTo(move_target + stepper.currentPosition());
  stepper.setSpeed(velocity);
  //NOTE: 'setSpeed' should come LATER than 'moveTo'!
  //  --> 'moveTo' re-calculate the velocity.
  //  --> so we need to re-override it.
  //
  // }
}
Task moving_task(0, TASK_ONCE, &moving);

// 'step' task --> absolute movements
int step_target = 0;
int step_duration = 10000;
void stepping() {
  //
  // if (stepper.distanceToGo() == 0) {
  //
  float cur_step = stepper.currentPosition();
  float target_step = step_target;
  float dur = step_duration;
  // float target_step = notes[score_now][note_idx][0];
  // float dur = notes[score_now][note_idx][1];
  float steps = target_step - cur_step;
  float velocity = steps / dur * 1000;   // unit conv.: (steps/msec) --> (steps/sec)
  float speed = fabs(velocity);
  //
  MONITORING_SERIAL.print("target_step --> "); MONITORING_SERIAL.println(target_step);
  MONITORING_SERIAL.print("dur --> "); MONITORING_SERIAL.println(dur);
  MONITORING_SERIAL.print("cur_step --> "); MONITORING_SERIAL.println(cur_step);
  //
  if (speed > STEPS_PER_SEC_MAX) {
    MONITORING_SERIAL.println("oh.. it might be TOO FAST for me..");
  } else {
    MONITORING_SERIAL.println("okay. i m going.");
  }

  //
  stepper.enableOutputs();
  stepper.moveTo(target_step);
  stepper.setSpeed(velocity);
  //NOTE: 'setSpeed' should come LATER than 'moveTo'!
  //  --> 'moveTo' re-calculate the velocity.
  //  --> so we need to re-override it.
  //
  // }
}
Task stepping_task(0, TASK_ONCE, &stepping);

//
void rest() {
  if (stepper.distanceToGo() == 0) {
    stepper.disableOutputs();
  }
}
Task rest_task(1000, TASK_FOREVER, &rest);
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
  hello.h2 = stepper.currentPosition();
  hello.h3 = stepper.distanceToGo();
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

// on 'Note'
void onNoteHandler(Note & n) {
  //is it for me?
  if (n.id == MY_GROUP_ID || n.id == MY_ID) {
    //
    if (n.pitch == 0) {
      //
      if (n.onoff == 1) {
        //
        step_target = n.x1;
        step_duration = n.x2;
        if (step_duration < 1000) step_duration = 1000;
        stepping_task.restartDelayed(10);
        //
      } else if (n.onoff == 0) {
        //
        move_target = 0;
        move_duration = 1000;
        moving_task.restartDelayed(10);
      }
      //
    } else if (n.pitch == 1) {
      //
      if (n.onoff == 1) {
        //
        move_target = n.x1;
        move_duration = n.x2;
        if (move_duration < 1000) move_duration = 1000;
        moving_task.restartDelayed(10);
        //
      } else if (n.onoff == 0) {
        move_target = 0;
        move_duration = 1000;
        moving_task.restartDelayed(10);
      }
      //
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

  //stepper
  //  "The fastest motor speed that can be reliably supported is about 4000 steps per
  //  second at a clock frequency of 16 MHz on Arduino such as Uno etc."
  //    @ AccelStepper.h
  stepper.setMaxSpeed(STEPS_PER_SEC_MAX); //steps per second (trade-off between speed vs. torque)
#if (STEP_MODE == STEP_MODE_ACCELERATING)
  stepper.setAcceleration(ACCELERATION_MAX);
#endif

  //tasks
  runner.addTask(stepping_task);
  runner.addTask(moving_task);
  runner.addTask(rest_task);
  rest_task.restartDelayed(500);
}

void loop() {
  //
  runner.execute();
  //

  //stepper
  if (stepper.distanceToGo() != 0) {
#if (STEP_MODE == STEP_MODE_CONSTANT_VEL)
    stepper.runSpeed();
#elif (STEP_MODE == STEP_MODE_ACCELERATING)
    stepper.run();
#endif
  }
}
