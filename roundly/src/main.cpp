//
// wirelessly connected cloud (Wireless Mesh Networking)
// MIDI-like
// spacial
//

//
// conversation on the ROOT @ SEMA, Seoul
//

//
// 2020 10 14
//

//==========<configurations>===========
//
// 'DISABLE_AP'
// --> disabling AP is for teensy audio samplers.
//     they need this to reduce noise from AP beacon signals.
//     but, then they cannot build-up net. by themselves.
//     we need who can do AP..
//     ==> TODO! just prepare some 'dummy' postmans around. w/ AP activated.
//
// 'DISABLE_I2C_REQ'
// --> a quirk.. due to bi-directional I2C hardship.
//     ideally, we want to make this sampler node also speak.
//     but, I2C doesn't work. maybe middleware bug.. we later want to change to diff. proto.
//     for example, UART or so.
//     ==> BEWARE! yet, still we need to take off this.. for 'osc' node.
//
// 'SET_ROOT'
// 'SET_CONTAINSROOT'
// --> for the network stability
//     declare 1 root node and branches(constricted to 'contains the root')
//     to improve the stability of the net
//
//==========</configurations>==========

//==========<preset>===========
#define SET_CONTAINSROOT
//==========</preset>==========

//============<list of reserved keys>============
#define ROUNDLY_A_KEY 200 // A-E-I-O-U-W-Y-N (up to 8 roundlys) - KEY 200 ~ 207
#define ROUNDLY_E_KEY 201
#define ROUNDLY_I_KEY 202
#define ROUNDLY_O_KEY 203
#define ROUNDLY_U_KEY 204
#define ROUNDLY_W_KEY 205
#define ROUNDLY_Y_KEY 206
#define ROUNDLY_N_KEY 207
//============</list of reserved keys>===========

//============<identity key>============
#define ID_KEY ROUNDLY_A_KEY
//============</identity key>===========

//============<parameters>============
#define MESH_SSID "forest-all/around"
#define MESH_PASSWORD "cc*vvvv/kkk"
#define MESH_PORT 5555
#define MESH_CHANNEL 5
#define LONELY_TO_DIE    (1000)
//============</parameters>===========

//
// LED status indication
// phase 0
//    - LED => steady on
//    - booted. and running. no connection. scanning.
// phase 1
//    - LED => slow blinking (syncronized)
//    - + connected.
//
#if defined(ARDUINO_ESP8266_NODEMCU) // nodemcuv2
#define LED_PIN 2
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) // d1_mini_pro
#define LED_PIN 2
#elif defined(ARDUINO_ESP8266_ESP12) // huzzah
#define LED_PIN 2
#elif defined(ARDUINO_FEATHER_ESP32) // featheresp32
#define LED_PIN 13
#elif defined(ARDUINO_NodeMCU_32S) // nodemcu-32s
#define LED_PIN 2
#elif defined(ARDUINO_ESP32_DEV) // esp32doit-devkit-v1
#define LED_PIN 2
#endif
#define LED_PERIOD (1111)
#define LED_ONTIME (1)

//arduino
#include <Arduino.h>

//i2c
#include <Wire.h>
#include "../../post.h"

//painlessmesh
#include <painlessMesh.h>
painlessMesh mesh;

//scheduler
Scheduler runner;

//task #0 : connection indicator
bool onFlag = false;
bool isConnected = false;
//prototypes
void taskStatusBlink_steadyOn();
void taskStatusBlink_slowblink_insync();
void taskStatusBlink_steadyOff();
//the task
Task statusblinks(0, 1, &taskStatusBlink_steadyOn); // at start, steady on. default == disabled. ==> setup() will enable.
// when disconnected, and trying, steadyon.
void taskStatusBlink_steadyOn() {
  onFlag = true;
}
// when connected, blink per 1s. sync-ed. (== default configuration)
void taskStatusBlink_slowblink_insync() {
  // toggler
  onFlag = !onFlag;
  // on-time
  statusblinks.delay(LED_ONTIME);
  // re-enable & sync.
  if (statusblinks.isLastIteration()) {
    statusblinks.setIterations(2); //refill iteration counts
    statusblinks.enableDelayed(LED_PERIOD - (mesh.getNodeTime() % (LED_PERIOD*1000))/1000); //re-enable with sync-ed delay
  }
}
// when connected, steadyoff. (== alternative configuration)
void taskStatusBlink_steadyOff() {
  onFlag = false;
}

//task #1 : happy or lonely
//   --> automatic reset after some time of 'loneliness (disconnected from any node)'
void nothappyalone() {
  static bool isConnected_prev = false;
  static unsigned long lonely_time_start = 0;
  // oh.. i m lost the signal(==connection)
  if (isConnected_prev != isConnected && isConnected == false) {
    lonely_time_start = millis();
    Serial.println("oh.. i m lost!");
  }
  // .... how long we've been lonely?
  if (isConnected == false) {
    if (millis() - lonely_time_start > LONELY_TO_DIE) {
      // okay. i m fed up. bye the world.
      Serial.println("okay. i m fed up. bye the world.");
      Serial.println();
#if defined(ESP8266)
      ESP.reset();
#else
#error unknown esp.
#endif
    }
  }
  //
  isConnected_prev = isConnected;
}
// Task nothappyalone_task(1000, TASK_FOREVER, &nothappyalone, &runner, true); // by default, ENABLED.
Task nothappyalone_task(100, TASK_FOREVER, &nothappyalone); // by default, ENABLED.

//
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
AccelStepper stepper(AccelStepper::FULL4WIRE, D5, D6, D7, D8); // N.B. - @esp8266, NEVER use "5, 6, 7, 8" -> do "D5, D6, D7, D8" !!

// my tasks
extern Task stepping_task;
extern Task rest_task;
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
  Serial.print("target_step --> "); Serial.println(target_step);
  Serial.print("dur --> "); Serial.println(dur);
  Serial.print("cur_step --> "); Serial.println(cur_step);
  //
  if (speed > STEPS_PER_SEC_MAX) {
    Serial.println("oh.. it might be TOO FAST for me..");
  } else {
    Serial.println("okay. i m going.");
  }

  //
  stepper.moveTo(target_step);
  stepper.setSpeed(velocity);
  //NOTE: 'setSpeed' should come LATER than 'moveTo'!
  //  --> 'moveTo' re-calculate the velocity.
  //  --> so we need to re-override it.
  //
  // }
}
Task stepping_task(0, TASK_ONCE, &stepping);

void rest() {
}
Task rest_task(0, TASK_ONCE, &rest);

// mesh callbacks
void receivedCallback(uint32_t from, String & msg) { // REQUIRED
  Serial.print("got msg.: ");
  Serial.println(msg);
  //parse now.

  //parse letter string.

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

  String str_key = msg.substring(1, 4);
  String str_velocity = msg.substring(4, 7);
  String str_gate = msg.substring(7, 8);

  int key = str_key.toInt();
  int velocity = str_velocity.toInt(); // 0 ~ 127
  int gate = str_gate.toInt();

  //    : [_______X......................]
  //    : X - Extension starter 'X'
  //      .substring(8, 9);
  // Extension (X == 'X')
  //    : [_______X1111222233344455667788]
  //    : 1 - data of 4 letters
  //      .substring(9, 13);
  //    : 2 - data of 4 letters
  //      .substring(13, 17);
  //    : 3 - data of 3 letters
  //      .substring(17, 20);
  //    : 4 - data of 3 letters
  //      .substring(20, 23);
  //    : 5 - data of 2 letters
  //      .substring(23, 25);
  //    : 6 - data of 2 letters
  //      .substring(25, 27);
  //    : 7 - data of 2 letters
  //      .substring(27, 29);
  //    : 8 - data of 2 letters
  //      .substring(29, 31);

  String str_ext = msg.substring(8, 9);
  String str_x1 = msg.substring(9, 13);
  String str_x2 = msg.substring(13, 17);
  String str_x3 = msg.substring(17, 20);
  String str_x4 = msg.substring(20, 23);
  String str_x5 = msg.substring(23, 25);
  String str_x6 = msg.substring(25, 27);
  String str_x7 = msg.substring(27, 29);
  String str_x8 = msg.substring(29, 31);

  if (str_ext == "X") {
    step_target = str_x1.toInt(); // 0 ~ 9999
    step_duration = str_x2.toInt(); // 0 ~ 9999
  }

  //is it for me?
  if (key == ID_KEY) {
    if (gate == 1) {
      stepping_task.restartDelayed(10);
    } else if (gate == 0) {
      rest_task.restartDelayed(10);
    }
  }
}
void changedConnectionCallback() {
  Serial.println(mesh.getNodeList().size());
  // check status -> modify status LED
  if (mesh.getNodeList().size() > 0) {
    // (still) connected.
    onFlag = false; //reset flag stat.
    statusblinks.set(LED_PERIOD, 2, &taskStatusBlink_slowblink_insync);
    // statusblinks.set(0, 1, &taskStatusBlink_steadyOff);
    statusblinks.enable();
    Serial.println("connected!");
    //
    isConnected = true;
    runner.addTask(nothappyalone_task);
    nothappyalone_task.enable();
  }
  else {
    // disconnected!!
    statusblinks.set(0, 1, &taskStatusBlink_steadyOn);
    statusblinks.enable();
    //
    isConnected = false;
  }
  // let I2C device know
  /////
  Serial.println("hi. client, we ve got a change in the net.");
}
void newConnectionCallback(uint32_t nodeId) {
  Serial.println(mesh.getNodeList().size());
  Serial.println("newConnectionCallback.");
  changedConnectionCallback();
}

void setup() {
  //led
  pinMode(LED_PIN, OUTPUT);

  //mesh
  WiFiMode_t node_type = WIFI_AP_STA;
#if defined(DISABLE_AP)
  system_phy_set_max_tpw(0);
  node_type = WIFI_STA;
#endif
  // mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);
  mesh.setDebugMsgTypes( ERROR | STARTUP );
  mesh.init(MESH_SSID, MESH_PASSWORD, &runner, MESH_PORT, node_type, MESH_CHANNEL);

  //
  // void init(String ssid, String password, Scheduler *baseScheduler, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  // void init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  //

#if defined(SET_ROOT)
  mesh.setRoot(true);
#endif
#if defined(SET_CONTAINSROOT)
  mesh.setContainsRoot(true);
#endif
  //callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  Serial.println(mesh.getNodeList().size());

  //tasks
  runner.addTask(statusblinks);
  statusblinks.enable();

  //serial
  Serial.begin(115200);
  delay(100);
  Serial.println("hi, postman ready.");
#if defined(DISABLE_AP)
  Serial.println("!NOTE!: we are in the WIFI_STA mode!");
#endif

  //understanding what is 'the nodeId' ==> last 4 bytes of 'softAPmacAddress'
  // uint32_t nodeId = tcp::encodeNodeId(MAC);
  Serial.print("nodeId (dec) : ");
  Serial.println(mesh.getNodeId(), DEC);
  Serial.print("nodeId (hex) : ");
  Serial.println(mesh.getNodeId(), HEX);
  uint8_t MAC[] = {0, 0, 0, 0, 0, 0};
  if (WiFi.softAPmacAddress(MAC) == 0) {
    Serial.println("init(): WiFi.softAPmacAddress(MAC) failed.");
  }
  Serial.print("MAC : ");
  Serial.print(MAC[0], HEX); Serial.print(", ");
  Serial.print(MAC[1], HEX); Serial.print(", ");
  Serial.print(MAC[2], HEX); Serial.print(", ");
  Serial.print(MAC[3], HEX); Serial.print(", ");
  Serial.print(MAC[4], HEX); Serial.print(", ");
  Serial.println(MAC[5], HEX);

  // for instance,

  // a huzzah board
  // nodeId (dec) : 3256120530
  // nodeId (hex) : C21474D2
  // MAC : BE, DD, C2, 14, 74, D2

  // a esp8266 board (node mcu)
  // nodeId (dec) : 758581767
  // nodeId (hex) : 2D370A07
  // MAC : B6, E6, 2D, 37, A, 7

  //introduction
  Serial.print("my ID Key --> ");
  Serial.println(ID_KEY);

  //i2c master
  Wire.begin();

  //random seed
  randomSeed(analogRead(0));

  //stepper
  // pinMode(D1, OUTPUT);
  // pinMode(D2, OUTPUT);
  // pinMode(D3, OUTPUT);
  // pinMode(D4, OUTPUT);
  /// "
  /// The fastest motor speed that can be reliably supported is about 4000 steps per
  /// second at a clock frequency of 16 MHz on Arduino such as Uno etc.
  /// " @ AccelStepper.h
  stepper.setMaxSpeed(STEPS_PER_SEC_MAX); //steps per second (trade-off between speed vs. torque)
#if (STEP_MODE == STEP_MODE_ACCELERATING)
  stepper.setAcceleration(ACCELERATION_MAX);
#endif

  //tasks
  runner.addTask(stepping_task);
  runner.addTask(rest_task);

  rest_task.restartDelayed(500);
}

void loop() {
  runner.execute();
  mesh.update();
  digitalWrite(LED_PIN, !onFlag); // value == false is ON. so onFlag == true is ON. (pull-up)

  //stepper
  if (stepper.distanceToGo() != 0) {
#if (STEP_MODE == STEP_MODE_CONSTANT_VEL)
    stepper.runSpeed();
#elif (STEP_MODE == STEP_MODE_ACCELERATING)
    stepper.run();
#endif
  }
}
