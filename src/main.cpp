//
// wirelessly connected cloud (Wireless Mesh Networking)
//

//
// Conversation about the ROOT @ SEMA warehouses, Seoul
//

//
// 2021 02 15
//
// (part-1) esp8266 : 'postman' (the mesh network nodes)
//
//   this module will build up a mesh cloud.
//
//   for now, ESP-MESH is out there.
//   which is probably more complete impl. of this kind.
//   but that's only for esp32, not for esp8266
//   we want to use esp8266, so, we will use painlessMesh
//   which is also good.
//
//   for painlessMesh, a node is a JSON 'postman'
//   we can broadcast/unicast/recv. msg. w/ meshID and nodelist
//   so, let's just use it.
//
//   but one specific thing is that we had used I2C comm. to feed this postman.
//   and I2C was a FIXED-length msg.
//
//   but we will use UART instead of I2C, because I2C was not feasible for true bi-directional comm. between teensy <-> esp8266
//   that's a thing of board-specific. in general it should be ok. but this particular case, it was hard to solve.
//
//   yet, we were very satisfied with previous 'fixed length (32 bytes)' comm.
//   so even though we are free to use variable-length messages, since using UART comm., we want to stick to that 'fixed-length' frame defs.
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
// 'DISABLE_I2C_REQ' (DEPRECATED)
// --> DEPRECATED: we are not using I2C anymore.
//
// 'SET_ROOT'
// 'SET_CONTAINSROOT'
// --> for the network stability
//     declare 1 root node and branches(constricted to 'contains the root')
//     to improve the stability of the net
//
//==========</configurations>==========

//==========<preset>===========
// (1) the backbone AP
#if 0
#define SET_CONTAINSROOT
// (2) osc client (the ROOT)
#elif 0
#define SET_ROOT
#define SET_CONTAINSROOT
// (2-1) osc client (non-ROOT)
#elif 0
// #define SET_ROOT
#define SET_CONTAINSROOT
// (3) sampler client
#elif 1
#define DISABLE_AP
//
#endif
//==========</preset>==========

//============<parameters>============
#define MESH_SSID "forest-all/around"
#define MESH_PASSWORD "cc*vvvv/kkk"
#define MESH_PORT 5555
#define MESH_CHANNEL 5
#define LONELY_TO_DIE    (1000)
#define MONITORING_SERIAL (Serial1) //you should use, external uart2usb converter to monitor messages...
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
#endif
#define LED_PERIOD (1111)
#define LED_ONTIME (1)

//arduino
#include <Arduino.h>

//protocol
// #include <Wire.h>
#include "../post.h"

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
    MONITORING_SERIAL.println("oh.. i m lost!");
  }
  // .... how long we've been lonely?
  if (isConnected == false) {
    if (millis() - lonely_time_start > LONELY_TO_DIE) {
      // okay. i m fed up. bye the world.
      MONITORING_SERIAL.println("okay. i m fed up. bye the world.");
      MONITORING_SERIAL.println();
#if defined(ESP8266)
      ESP.reset();
#elif defined(ESP32)
      ESP.restart();
      // esp32 doesn't support 'reset()' yet...
      // (restart() is framework-supported, reset() is more forced hardware-reset-action)
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

//task #2 : regular post collection
void collect_post() {
  //
  static char letter_outro[POST_BUFF_LEN] = "................................";
  // error flag
  bool letter_is_good = false;
  // check the first byte
  char first = '.';
  // automatically match start byte.
  while (Serial.available()) {
    first = Serial.read();
    if (first == '[' || first == '{') {
      // client want to give me a letter or hello.
      letter_outro[0] = first;
      // matched!
      letter_is_good = true;
      break;
    } else if (first == ' ') {
      // client says nothing to send.
      MONITORING_SERIAL.print("."); // nothing to send.
      return;
    }
  }
  //
  if (letter_is_good == false) {
    // no more letters, but no valid char.
    MONITORING_SERIAL.print("?"); // wrong client.
    return;
  } else if (letter_is_good == true) {
    // get more contents
    for (int i = 1; i < (POST_LENGTH-1); i++) {
      if (Serial.available()) {
        letter_outro[i] = Serial.read();
      } else {
        // hmm.. letter is too short.
        letter_outro[i] = '.'; // fill-out with dots.
        MONITORING_SERIAL.print("$"); // too $hort msg.
        letter_is_good = false;
      }
    }
    // the last byte
    char last = '.';
    if (Serial.available()) {
      letter_outro[POST_LENGTH-1] = last = Serial.read();
      if (last != ']' && last != '}') {
        // hmm.. last byte is strange
        MONITORING_SERIAL.print("#"); // last byte error.
        letter_is_good = false;
      }
    } else {
      // hmm.. letter is too short.
      letter_outro[POST_LENGTH-1] = '.'; // fill-out with dots.
      MONITORING_SERIAL.print("$"); // too $hort msg.
      letter_is_good = false;
    }
    // terminal char.
    letter_outro[POST_LENGTH] = '\0';
  }
  // no good letter, we discard.
  if (letter_is_good == false) {
    return;
  }
  // or, post it.
  if (isConnected == true) {
    mesh.sendBroadcast(String(letter_outro));
    MONITORING_SERIAL.print("sendBroadcast: ");
    MONITORING_SERIAL.println(letter_outro);
  } else {
    MONITORING_SERIAL.print("_"); // disconnected.
  }
}
Task collect_post_task(10, TASK_FOREVER, &collect_post, &runner, true); // by default, ENABLED
//MAYBE... 10ms is too fast? move this to the loop() then?

// mesh callbacks
void receivedCallback(uint32_t from, String & msg) { // REQUIRED
  MONITORING_SERIAL.print("got msg.: ");
  MONITORING_SERIAL.println(msg);
  // truncate any extra. letters.
  msg = msg.substring(0, POST_LENGTH); // (0) ~ (POST_LENGTH-1)
  // send whatever letter we postmans trust other postman.
#if defined(ARDUINO_NodeMCU_32S)
  Serial.write((const uint8_t*)msg.c_str(), POST_LENGTH);
#else
  Serial.write(msg.c_str(), POST_LENGTH);
#endif
}
void changedConnectionCallback() {
  MONITORING_SERIAL.println(mesh.getNodeList().size());
  // check status -> modify status LED
  if (mesh.getNodeList().size() > 0) {
    // (still) connected.
    onFlag = false; //reset flag stat.
    statusblinks.set(LED_PERIOD, 2, &taskStatusBlink_slowblink_insync);
    // statusblinks.set(0, 1, &taskStatusBlink_steadyOff);
    statusblinks.enable();
    MONITORING_SERIAL.println("connected!");
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
  //
  MONITORING_SERIAL.println("hi. client, we ve got a change in the net.");
}
void newConnectionCallback(uint32_t nodeId) {
  MONITORING_SERIAL.println(mesh.getNodeList().size());
  MONITORING_SERIAL.println("newConnectionCallback.");
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
  MONITORING_SERIAL.println(mesh.getNodeList().size());

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

  //esp8266(serial.swap) <-> teensy(serial3)
  delay(1000);
  Serial.swap();
  delay(100);
  // Serial.println("hi, uart. ping?");
}

void loop() {
  runner.execute();
  mesh.update();
#if defined(ESP32)
  digitalWrite(LED_PIN, onFlag); // value == true is ON.
#else
  digitalWrite(LED_PIN, !onFlag); // value == false is ON. so onFlag == true is ON. (pull-up)
#endif
}
