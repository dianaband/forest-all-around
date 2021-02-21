//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversations about the ROOT @ SEMA warehouses, Seoul
//

//
// 2021 02 15
//
// (part-2) teensy35 : 'client:osc' (osc over slip <--> esp-now 'post')
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
void setup() {
  //osc
  SLIPSerial.begin(57600);

  //
  POSTMAN_SERIAL.begin(115200);
}

//
void route_note(OSCMessage& msg, int offset) {
  // matches will happen in the order. that the bundle is packed.
  static Note note;
  // (1) --> /onoff
  if (msg.fullMatch("/onoff", offset)) {
    //
    note.clear();
    //
    note.onoff = msg.getFloat(0);
    if (note.onoff != 0) note.onoff = 1;
  }
  // (2) --> /velocity
  if (msg.fullMatch("/velocity", offset)) {
    note.velocity = msg.getFloat(0);
  }
  // (3) --> /pitch
  if (msg.fullMatch("/pitch", offset)) {
    note.pitch = msg.getFloat(0);
  }
  // (4) --> /id
  if (msg.fullMatch("/id", offset)) {
    note.id = msg.getInt(0);
  }
  // (5) --> /x
  if (msg.fullMatch("/x", offset)) {
    note.x1 = msg.getFloat(0);
    note.x2 = msg.getFloat(1);
    note.x3 = msg.getFloat(2);
    note.x4 = msg.getFloat(3);
    note.ps = msg.getFloat(4);
    //
    POSTMAN_SERIAL.write('['); // start byte of 'Note'
    POSTMAN_SERIAL.write((uint8_t *) &note, sizeof(Note));
    POSTMAN_SERIAL.write(']'); // end byte of 'Note'
    //
  }
}

//
void loop() {

  //osc
  OSCBundle bundleIN;
  int size;
  if (SLIPSerial.available()) {
    while(!SLIPSerial.endofPacket()) {
      if( (size = SLIPSerial.available()) > 0) {
        while(size--) {
          bundleIN.fill(SLIPSerial.read());
        }
      }
    }
    if(!bundleIN.hasError()) {
      // on '/note'
      bundleIN.route("/note", route_note);
    }
  }

  //postman (serial comm.)
  static bool insync = false;
  if (insync == false) {
    while (POSTMAN_SERIAL.available() > 0) {
      // search the last byte
      char last = POSTMAN_SERIAL.read();
      // expectable last of the messages
      if (last == ']' || last == '}') {
        insync = true;
      }
    }
  } else {
    //
    if (POSTMAN_SERIAL.available() > 0) {
      //
      char type = POSTMAN_SERIAL.peek();
      //
      if (type == '{') {
        //expecting a Hello message.
        if (POSTMAN_SERIAL.available() > sizeof(Hello) + 2) {
          POSTMAN_SERIAL.read();
          //
          Hello hello("");
          POSTMAN_SERIAL.readBytes((uint8_t *) &hello, sizeof(Hello));
          char last = POSTMAN_SERIAL.read();
          if (last == '}') {
            //good.
            //
            OSCMessage osc("/hello");
            osc.add(hello.id);
            osc.add(hello.h1);
            osc.add(hello.h2);
            osc.add(hello.h3);
            osc.add(hello.h4);
            //
            SLIPSerial.beginPacket();
            osc.send(SLIPSerial);
            SLIPSerial.endPacket();
            osc.empty();
            //
          } else {
            insync = false; //error!
          }
        }
      } else if (type == '[') {
        //expecting a Note message.
        if (POSTMAN_SERIAL.available() > sizeof(Note) + 2) {
          POSTMAN_SERIAL.read();
          //
          Note note;
          POSTMAN_SERIAL.readBytes((uint8_t *) &note, sizeof(Note));
          char last = POSTMAN_SERIAL.read();
          if (last == ']') {
            //good.
          } else {
            insync = false; //error!
          }
        }
        //
      }
    }
  }
}
