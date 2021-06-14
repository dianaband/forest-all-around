#pragma once

//obsolete (@postman|@sampler still ues this)
#define I2C_ADDR 3
#define POST_LENGTH 32
#define POST_BUFF_LEN (POST_LENGTH + 1)

//esp-now
#include <vector>

// 'address'
struct Address {
  String name;
  uint8_t mac[6];
  Address() {
    mac[0] = 0;
    mac[1] = 0;
    mac[2] = 0;
    mac[3] = 0;
    mac[4] = 0;
    mac[5] = 0;
    name = "";
  };
  Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, String n) {
    mac[0] = a;
    mac[1] = b;
    mac[2] = c;
    mac[3] = d;
    mac[4] = e;
    mac[5] = f;
    name = n;
  }
  //
  String to_string() {
    char mac_cstr[18]; // "AA:BB:CC:AA:BB:CC"
    snprintf(mac_cstr, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return (String(mac_cstr) + " ==> " + name);
  }
};

struct AddressBook {
  String title;
  std::vector<Address> list;

  //
  AddressBook (String title_ = "") {
    title = title_;
  }
  //
  void add(Address addr) {
    list.push_back(addr);
  }
};

//
struct AddressLibrary {
  //
  std::vector<AddressBook> lib;
  //
  AddressLibrary() {

    // book #1
    {
      AddressBook book = AddressBook("root");
      //
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x3B, 0x90, "root/osc"));
      book.add(Address(0x68, 0xC6, 0x3A, 0xD7, 0x4D, 0x97, "root(2)/osc"));
      //
      lib.push_back(book);
    }
    // book #2
    {
      AddressBook book = AddressBook("friend");
      //
      // 01 - 10
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB4, 0x47, "green suitcase - cricket/124"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0x32, "green suitcase - cricket/127"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xAA, 0x56, 0x11, "gastank - taak/150"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0xCC, "gastank - cricket/128"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB4, 0x64, "roundlys - roundly/2000"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB8, 0x1E, "roundlys - roundly/2001"));
      book.add(Address(0x80, 0x7D, 0x3A, 0x58, 0x80, 0x30, "buoyfly - cricket/121"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xAA, 0x4A, 0xCC, "buoyfly - cricket/122"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB3, 0xD4, "buoyfly - cricket/123"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB3, 0xE2, "buoyfly - cricket/129"));
      // 11 - 20
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB6, 0xC6, "buoyfly - cricket/130"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0xA3, "buoyfly - cricket/131"));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0xB8, "blue drummer - cricket/120"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0x83, 0x80, "blue drummer - taak/154"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0xCF, "blue drummer - taak/153"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xAA, 0x17, 0x8D, "frog eyes - taak/151"));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB9, 0xB4, "untitled - gonggong/1000"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB4, 0x28, "beak - taak/157"));
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB3, 0xEF, "yellow - cricket/125"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xAA, 0x78, 0x87, "yellow - cricket/126"));
      //
      lib.push_back(book);
    }
    // book #3
    {
      AddressBook book = AddressBook("sampler");
      //
      // samplers don't have ID_KEY, they will just get all messages,
      // then open the content to get **midi** 'key' in the 'note' message.
      book.add(Address(0xBC, 0xDD, 0xC2, 0xB2, 0xAF, 0xD4, "@postman for @sampler"));
      //
      lib.push_back(book);
    }
    // book #4
    {
      AddressBook book = AddressBook("audioooo");
      //
      // book.add(Address(0xAC, 0x67, 0xB2, 0x0B, 0xAE, 0x0C, "audioooo #1 (Q)")); //WROOM <- sender(the Q injector)
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST"));   //a broadcast (ESP32 will ignore this, but ESP8266 will do process this. let's expect them to work!)
      book.add(Address(0xAC, 0x67, 0xB2, 0x0B, 0xAD, 0xB0, "audioooo #2")); //WROOM
      book.add(Address(0xA8, 0x03, 0x2A, 0x6C, 0x88, 0x78, "audioooo #3")); //WROVER
      book.add(Address(0xA8, 0x03, 0x2A, 0x6C, 0x88, 0x5C, "audioooo #4")); //WROVER
      book.add(Address(0xA8, 0x03, 0x2A, 0x75, 0xD0, 0x68, "audioooo #5")); //WROVER
      //
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0xDD, "sampler #1")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x37, 0xAE, "sampler #2")); //TEENSY+ESP8266
      book.add(Address(0xEC, 0xFA, 0xBC, 0x63, 0x19, 0x84, "sampler #3")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0x19, "sampler #4")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xBA, 0x44, "sampler #5")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB5, 0xC2, "sampler #6")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x09, 0x92, "sampler #7")); //TEENSY+ESP8266
      book.add(Address(0x68, 0xC6, 0x3A, 0xD5, 0x3E, 0xF3, "sampler #8")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x45, 0xF5, "sampler #9")); //TEENSY+ESP8266
      book.add(Address(0xBC, 0xDD, 0xC2, 0xB2, 0xAF, 0xD4, "sampler #A")); //TEENSY+ESP8266
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0xA7, 0xB5, "sampler #B")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x11, 0xE6, "sampler #C")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x18, 0xAE, "sampler #D")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x0A, 0x07, "sampler #E")); //TEENSY+ESP8266
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0x21, "sampler #F")); //TEENSY+ESP8266 //20 sets => FULL
      // + we have 5 more un-registered devices. esp8266 devices will broadcast for them. hopely all get to contact w/ msg. on time.
      lib.push_back(book);
    }
  }
  //
  AddressBook* getBookByTitle(String title_) {
    for (uint32_t i = 0; i < lib.size(); i++) {
      if (lib[i].title == title_) {
        return &(lib[i]);
      }
    }
    //
    return NULL;
  }
};

//message type Note : '[' + Note + ']'
struct Note {
  //
  int32_t id;
  float pitch;
  float velocity;
  float onoff;
  float x1;
  float x2;
  float x3;
  float x4;
  float ps;
  //
  void clear() {
    id = 0;
    pitch = 0;
    velocity = 0;
    onoff = 0;
    x1 = 0;
    x2 = 0;
    x3 = 0;
    x4 = 0;
    ps = 0;
  }
  //
  String to_string() {
    String str = "";
    str += "( id=" + String(id);
    str += ", pitch=" + String(pitch);
    str += ", velocity=" + String(velocity);
    str += ", onoff=" + String(onoff);
    str += ", x1=" + String(x1);
    str += ", x2=" + String(x2);
    str += ", x3=" + String(x3);
    str += ", x4=" + String(x4);
    str += ", ps=" + String(ps);
    str += " )";
    return str;
  }
};

//message type Hello : '{' + Hello + '}'
#define SIGNATURE_LENGTH   (20)
#define SIGNATURE_BUFF_LEN (SIGNATURE_LENGTH + 1)
struct Hello {
  char sign[SIGNATURE_BUFF_LEN];
  int32_t id;
  uint32_t mac32;
  float h1;
  float h2;
  float h3;
  float h4;
  //
  Hello(String sign_, int32_t id_ = 0, uint32_t mac32_ = 0, float h1_ = 0, float h2_ = 0, float h3_ = 0, float h4_ = 0) {
    id = id_;
    mac32 = mac32_;
    h1 = h1_;
    h2 = h2_;
    h3 = h3_;
    h4 = h4_;
    sign_.toCharArray(sign, SIGNATURE_BUFF_LEN);
  }
  void clear() {
    id = 0;
    mac32 = 0;
    h1 = 0;
    h2 = 0;
    h3 = 0;
    h4 = 0;
    sign[0] = '\0';
  }
  //
  String to_string() {
    String str = "";
    str += "( id=" + String(id);
    str += ", mac32=0x" + String(mac32, HEX);
    str += ", sign=\"" + String(sign) + "\"";
    str += ", h1=" + String(h1);
    str += ", h2=" + String(h2);
    str += ", h3=" + String(h3);
    str += ", h4=" + String(h4);
    str += " )";
    return str;
  }
};
