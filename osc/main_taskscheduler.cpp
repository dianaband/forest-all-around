//
// wirelessly connected cloud (Wireless Mesh Networking)
//

//
// Conversations about the ROOT @ SEMA warehouses, Seoul
//

//
// 2021 02 15
//
// (part-2) teensy35 : 'client:osc' (osc over slip --> mesh post)
//

//arduino
#include <Arduino.h>

//osc (already included in "framework-arduinoteensy")
#include <OSCBundle.h>
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial(Serial);

//post definition
#include "../../post.h"

//postman's uart
#define POSTMAN_SERIAL  (Serial3)

//
void midinote(OSCMessage& msg, int offset) {
  // matches will happen in the order. that the bundle is packed.
  static int pitch = 0;
  static int velocity = 0;
  static int onoff = 0;
  static int x1 = 0;
  static int x2 = 0;
  static int x3 = 0;
  static int x4 = 0;
  static int ps = 0;
  // (1) --> /onoff
  if (msg.fullMatch("/onoff", offset)) {
    //
    pitch = 0;
    velocity = 0;
    onoff = 0;
    //
    onoff = msg.getInt(0);
    if (onoff != 0) onoff = 1;
  }
  // (2) --> /velocity
  if (msg.fullMatch("/velocity", offset)) {
    velocity = msg.getInt(0);
    if (velocity < 0) velocity = 0;
    // if (velocity > 127) velocity = 127;
    if (velocity > 999) velocity = 999;
  }
  // (3) --> /pitch
  if (msg.fullMatch("/pitch", offset)) {
    pitch = msg.getInt(0);
    if (pitch < 0) pitch = 0;
    // if (pitch > 127) pitch = 127;
    if (pitch > 999) pitch = 999;
  }
  // (4) --> /x
  if (msg.fullMatch("/x", offset)) {
    x1 = msg.getInt(0);
    x2 = msg.getInt(1);
    x3 = msg.getInt(2);
    x4 = msg.getInt(3);
    ps = msg.getInt(4);

    char letter[POST_BUFF_LEN] = "";
    snprintf(letter, POST_BUFF_LEN, "[%03d%03d%01dX%05d%05d%05d%05d%02d]",
             pitch,
             velocity,
             onoff,
             x1,
             x2,
             x3,
             x4,
             ps);
    POSTMAN_SERIAL.print(letter);
  }
}

//task
#include <TaskScheduler.h>
Scheduler runner;
void osc_listen () {
  //
  static OSCBundle bundleIN;
  int size = 0;
  while (!SLIPSerial.endofPacket()) {
    if ((size = SLIPSerial.available()) > 0) {
      while(size--) {
        bundleIN.fill(SLIPSerial.read());
      }
    }
  }
  //
  if(!bundleIN.hasError()) {
    bundleIN.route("/note", midinote);
    //
    bundleIN.empty();
  }
}
Task osc_listen_task(0, TASK_FOREVER, osc_listen, &runner, true); // aInterval == 0 >>> immediately re-schedule ?
//
// OSCBundle bundleIN;
// int size;
//
// while(!SLIPSerial.endofPacket())
//   if( (size =SLIPSerial.available()) > 0)
//   {
//     while(size--)
//       bundleIN.fill(SLIPSerial.read());
//   }
//
// if(!bundleIN.hasError())
//   bundleIN.dispatch("/led", LEDcontrol);
//

void postman_talk () {
  //
  static bool insync = false;
  if (insync == false) {
    int size;
    if ((size = POSTMAN_SERIAL.available()) > 0) {
      char last = '.';
      while(size--) {
        // get the last byte
        last = POSTMAN_SERIAL.read();
      }
      // expectable last of the messages
      if (last == ']' || last == '}') {
        insync = true;
      }
    }
  } else if (insync == true) {

    if (POSTMAN_SERIAL.available() > POST_LENGTH) {
      char cstr[POST_BUFF_LEN] = "................................";
      // fetch all the bytes
      POSTMAN_SERIAL.readBytes(cstr, POST_LENGTH);
      // protocol checks
      char first = cstr[0];
      char last = cstr[POST_LENGTH-1];
      if (first != '[' && first != '{') {
        insync = false;
        return;   //error!!
      }
      if (last != ']' && last != '}') {
        insync = false;
        return;   //error!!
      }

      //// OK -> parse && compose & send OSC message!

      String msg = String(cstr);

      // hello frame ( '{' + 30 bytes + '}' )
      //    : {123456789012345678901234567890}

      // hello frame
      //    : {123456789012345678901234567890}
      //    : {IIIA111111222222333333444444__}
      //    : III - ID_KEY
      //      .substring(1, 4);
      //    : 1 - data of 6 letters
      //      .substring(9, 14);
      //    : 2 - data of 6 letters
      //      .substring(14, 19);
      //    : 3 - data of 6 letters
      //      .substring(19, 24);
      //    : 4 - data of 6 letters
      //      .substring(24, 29);

      // received a hello.
      String str_id = msg.substring(1, 4);
      int id = str_id.toInt();

      //
      OSCMessage hello("/hello");
      hello.add(id);

      //
      String str_aa = msg.substring(4, 5);

      //
      if (str_aa == "A") {
        //
        String str_h1 = msg.substring(5, 11);
        String str_h2 = msg.substring(11, 17);
        String str_h3 = msg.substring(17, 23);
        String str_h4 = msg.substring(23, 29);

        //
        hello.add(str_h1.toInt());
        hello.add(str_h2.toInt());
        hello.add(str_h3.toInt());
        hello.add(str_h4.toInt());
      }

      //
      SLIPSerial.beginPacket();
      hello.send(SLIPSerial);
      SLIPSerial.endPacket();
      hello.empty();
    }
  }
}
Task postman_talk_task(0, TASK_FOREVER, postman_talk, &runner, true); // aInterval == 0 >>> immediately re-schedule ?

//
void setup() {
  //osc
  SLIPSerial.begin(57600);

  //
  POSTMAN_SERIAL.begin(115200);
}

//
void loop() {
  runner.execute();
}
