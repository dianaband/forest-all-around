#pragma once

//obsolete (@postman|@sampler still ues this)
#define I2C_ADDR 3
#define POST_LENGTH 32
#define POST_BUFF_LEN (POST_LENGTH + 1)

//esp-now
#include <vector>

// (DEBUG) fetch full peer list
#if defined(ARDUINO_ARCH_ESP32) // for esp32 API
#include <esp_now.h>
struct PeerLister {
  void print() {
    Serial.printf("\n# (DEBUG) peer list (NOTE: a broadcast peer won't be listed properly)\n");
    esp_now_peer_num_t num;
    esp_now_get_peer_num(&num);
    esp_now_peer_info_t peer_info = {};
    bool first = true;
    for (uint8_t ii = 0; ii < num.total_num; ii++) {
      esp_now_fetch_peer(first, &peer_info);
      if (first) first = false;
      Serial.printf("# peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                               peer_info.peer_addr[0],
                               peer_info.peer_addr[1],
                               peer_info.peer_addr[2],
                               peer_info.peer_addr[3],
                               peer_info.peer_addr[4],
                               peer_info.peer_addr[5]);
    }
  }
};
#elif defined(ARDUINO_ARCH_ESP8266) // for esp8266 API
#include <espnow.h>
struct PeerLister {
  void print() {
    Serial.printf("\n# (DEBUG) peer list\n");
    bool first = true;
    uint8_t * peer_addr = NULL;
    while((peer_addr = esp_now_fetch_peer(first)) != NULL) {
      first = false;
      Serial.printf("# peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                               peer_addr[0],
                               peer_addr[1],
                               peer_addr[2],
                               peer_addr[3],
                               peer_addr[4],
                               peer_addr[5]);
    }
  }
};
#endif


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

    // book #0 - broadcast only
    {
      AddressBook book = AddressBook("broadcast only");
      //
      // [broadcast only]
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast
      //
      lib.push_back(book);
    }

    // 0set프로젝트 <관람모드-있는 방식>

    // book #1 - slopeway
    {
      AddressBook book = AddressBook("slopeway");
      //
      // [slopeway]        => 5 11 8 12 a    + c 2 13 4 i j e f 19 7 6
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x74, 0xD2, "slopeway 5 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB1, 0xD2, 0x66, "slopeway 11"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0xA7, 0xB5, "slopeway 8 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x77, 0xE6, "slopeway 12"));
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x63, 0x8E, "slopeway a "));
      //+
      book.add(Address(0x5C, 0xCF, 0x7F, 0xB8, 0xB6, 0x80, "slopeway+ c "));
      book.add(Address(0xEC, 0xFA, 0xBC, 0x63, 0x19, 0x84, "slopeway+ 2 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x6C, 0x7B, "slopeway+ 13"));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0xDD, "slopeway+ 4 "));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x09, 0x92, "slopeway+ i ")); //10
      book.add(Address(0x5C, 0xCF, 0x7F, 0xB7, 0x55, 0x98, "slopeway+ j "));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB5, 0xC2, "slopeway+ e "));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x37, 0xAE, "slopeway+ f "));
      book.add(Address(0x80, 0x7D, 0x3A, 0x58, 0x87, 0x2D, "slopeway+ 19"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x45, 0xF5, "slopeway+ 7 "));
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x75, 0x6F, "slopeway+ 6 "));
      //+
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast //17
      //
      lib.push_back(book);
    }
    // book #2 - 1st floor
    {
      AddressBook book = AddressBook("1st floor");
      //
      // [1st floor]       => c 2 10 b 14 3  + slopeway(5 11 8 12 a)
      book.add(Address(0x5C, 0xCF, 0x7F, 0xB8, 0xB6, 0x80, "1st floor c"));
      book.add(Address(0xEC, 0xFA, 0xBC, 0x63, 0x19, 0x84, "1st floor 2"));
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x29, 0x76, "1st floor 10"));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xBA, 0x44, "1st floor b"));
      book.add(Address(0x60, 0x01, 0x94, 0x38, 0x20, 0x5B, "1st floor 14"));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0x19, "1st floor 3")); //6
      //+
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x74, 0xD2, "slopeway 5 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB1, 0xD2, 0x66, "slopeway 11"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0xA7, 0xB5, "slopeway 8 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x77, 0xE6, "slopeway 12"));
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x63, 0x8E, "slopeway a "));
      //+
      book.add(Address(0xE0, 0xE2, 0xE6, 0xCD, 0x0A, 0xCC, "TEST"));
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast //12
      //
      lib.push_back(book);
    }
    // book #3 - 2nd floor
    {
      AddressBook book = AddressBook("2nd floor");
      //
      // [2nd floor]       => 4 j i 13 e f h + slopeway(5 11 8 12 a) + bridge(15 16 1 17)
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0xDD, "2nd floor 4"));
      book.add(Address(0x5C, 0xCF, 0x7F, 0xB7, 0x55, 0x98, "2nd floor j"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x09, 0x92, "2nd floor i"));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x6C, 0x7B, "2nd floor 13"));
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB5, 0xC2, "2nd floor e"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x37, 0xAE, "2nd floor f"));
      book.add(Address(0x68, 0xC6, 0x3A, 0xD5, 0x3E, 0xF3, "2nd floor h")); //7
      //+
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x74, 0xD2, "slopeway 5 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB1, 0xD2, 0x66, "slopeway 11"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0xA7, 0xB5, "slopeway 8 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x77, 0xE6, "slopeway 12"));
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x63, 0x8E, "slopeway a ")); //12
      //+
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x47, 0x84, "bridge 15"));
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x43, 0xA7, "bridge 16"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x0A, 0x07, "bridge 1"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x3B, 0x90, "bridge 17")); //16
      //+
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast //17
      //
      lib.push_back(book);
    }
    // book #4 - bridge
    {
      AddressBook book = AddressBook("bridge");
      //
      // [bridge]          => 15 16 1 17     + h 13 18 9
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x47, 0x84, "bridge 15"));
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x43, 0xA7, "bridge 16"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x0A, 0x07, "bridge 1"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x3B, 0x90, "bridge 17")); //4
      //+
      book.add(Address(0x68, 0xC6, 0x3A, 0xD5, 0x3E, 0xF3, "bridge+ h"));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x6C, 0x7B, "bridge+ 13"));
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x1C, 0x53, "bridge+ 18"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x11, 0xE6, "bridge+ 9")); //8
      //+
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast //9
      //
      lib.push_back(book);
    }
    // book #5 - 2nd floor annex
    {
      AddressBook book = AddressBook("2nd floor annex");
      //
      // [2nd floor annex] => 18 g 9         + bridge(15 16 1 17)
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x1C, 0x53, "2nd floor annex 18"));
      book.add(Address(0xBC, 0xDD, 0xC2, 0xB2, 0xAF, 0xD4, "2nd floor annex g"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x11, 0xE6, "2nd floor annex 9"));
      //+
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x47, 0x84, "bridge 15"));
      book.add(Address(0xA8, 0x48, 0xFA, 0xCD, 0x43, 0xA7, "bridge 16"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x0A, 0x07, "bridge 1"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x3B, 0x90, "bridge 17"));
      //+
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast //8
      //
      lib.push_back(book);
    }
    // book #6 - 3rd floor
    {
      AddressBook book = AddressBook("3rd floor");
      //
      // [3rd floor]       => 7 19 20 6 d    + slopeway(5 11 8 12 a)
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x45, 0xF5, "3rd floor 7"));
      book.add(Address(0x80, 0x7D, 0x3A, 0x58, 0x87, 0x2D, "3rd floor 19"));
      book.add(Address(0x30, 0x83, 0x98, 0xB1, 0x18, 0xB4, "3rd floor 20"));
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x75, 0x6F, "3rd floor 6"));
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x18, 0xAE, "3rd floor d"));
      //+
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x74, 0xD2, "slopeway 5 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB1, 0xD2, 0x66, "slopeway 11"));
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0xA7, 0xB5, "slopeway 8 "));
      book.add(Address(0x30, 0x83, 0x98, 0xB2, 0x77, 0xE6, "slopeway 12"));
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x63, 0x8E, "slopeway a "));
      //+
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST")); //a broadcast //11
      //
      lib.push_back(book);
    }

    // -- ARCHIVED
    // // 0set프로젝트 <거리두기> [살아갈, 사라진, 사람들: 2021 세월호]
    //
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
      book.add(Address(0xAC, 0x67, 0xB2, 0x0B, 0xAE, 0x0C, "audioooo #1 (Q)")); //WROOM <- sender(the Q injector)
      book.add(Address(0xAC, 0x67, 0xB2, 0x0B, 0xAD, 0xB0, "audioooo #2")); //WROOM
      book.add(Address(0xA8, 0x03, 0x2A, 0x6C, 0x88, 0x78, "audioooo #3")); //WROVER (==> audioooo alt)
      book.add(Address(0xA8, 0x03, 0x2A, 0x6C, 0x88, 0x5C, "audioooo #4")); //WROVER (==> audioooo alt)
      book.add(Address(0xA8, 0x03, 0x2A, 0x75, 0xD0, 0x68, "audioooo #5")); //WROVER (==> audioooo alt)
      //
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0xDD, "sampler #1")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x37, 0xAE, "sampler #2")); //TEENSY+ESP8266
      book.add(Address(0xEC, 0xFA, 0xBC, 0x63, 0x19, 0x84, "sampler #3")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0x19, "sampler #4")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xBA, 0x44, "sampler #5")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB5, 0xC2, "sampler #6")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x09, 0x92, "sampler #7")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0x68, 0xC6, 0x3A, 0xD5, 0x3E, 0xF3, "sampler #8")); //TEENSY+ESP8266 (==> audioooo alt)
      // (alternative list)
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x45, 0xF5, "sampler #9")); //TEENSY+ESP8266
      book.add(Address(0xBC, 0xDD, 0xC2, 0xB2, 0xAF, 0xD4, "sampler #A")); //TEENSY+ESP8266
      book.add(Address(0x84, 0xCC, 0xA8, 0xA3, 0xA7, 0xB5, "sampler #B")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x11, 0xE6, "sampler #C")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x18, 0xAE, "sampler #D")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x0A, 0x07, "sampler #E")); //TEENSY+ESP8266
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST"));   //a broadcast (ESP32 will ignore this, but ESP8266 will do process this. let's expect them to work!)
      // book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0x21, "sampler #F")); //TEENSY+ESP8266 //20 sets => FULL
      // + we have 5 more un-registered devices. esp8266 devices will broadcast for them. hopely all get to contact w/ msg. on time.
      lib.push_back(book);
    }
    // book #5
    {
      AddressBook book = AddressBook("audioooo alt");
      //
      book.add(Address(0xAC, 0x67, 0xB2, 0x0B, 0xAE, 0x0C, "audioooo #1 (Q)")); //WROOM <- sender(the Q injector)
      book.add(Address(0xAC, 0x67, 0xB2, 0x0B, 0xAD, 0xB0, "audioooo #2")); //WROOM
      book.add(Address(0xA8, 0x03, 0x2A, 0x6C, 0x88, 0x78, "audioooo #3")); //WROVER
      book.add(Address(0xA8, 0x03, 0x2A, 0x6C, 0x88, 0x5C, "audioooo #4")); //WROVER
      book.add(Address(0xA8, 0x03, 0x2A, 0x75, 0xD0, 0x68, "audioooo #5")); //WROVER
      //
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0xDD, "sampler #1")); //TEENSY+ESP8266
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x37, 0xAE, "sampler #2")); //TEENSY+ESP8266
      book.add(Address(0xEC, 0xFA, 0xBC, 0x63, 0x19, 0x84, "sampler #3")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB4, 0x19, "sampler #4")); //TEENSY+ESP8266
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xBA, 0x44, "sampler #5")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0x98, 0xF4, 0xAB, 0xB3, 0xB5, 0xC2, "sampler #6")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0xB4, 0xE6, 0x2D, 0x37, 0x09, 0x92, "sampler #7")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0x68, 0xC6, 0x3A, 0xD5, 0x3E, 0xF3, "sampler #8")); //TEENSY+ESP8266 (==> audioooo alt)
      // (alternative list)
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x75, 0x6F, "huzzah #F")); //TEENSY+HUZZAH (==> audioooo alt)
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x63, 0x8E, "huzzah #G")); //TEENSY+HUZZAH (==> audioooo alt)
      book.add(Address(0xBC, 0xDD, 0xC2, 0x14, 0x74, 0xD2, "huzzah #H")); //TEENSY+HUZZAH (==> audioooo alt)
      book.add(Address(0x5C, 0xCF, 0x7F, 0xB8, 0xB6, 0x80, "sampler #I")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB6, 0xEC, "sampler #J")); //TEENSY+ESP8266 (==> audioooo alt)
      book.add(Address(0x5C, 0xCF, 0x7F, 0xB7, 0x55, 0x98, "sampler #K")); //TEENSY+ESP8266 (==> audioooo alt)
      //
      book.add(Address(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, "BROADCAST"));  //a broadcast ... (unstable.. especially esp32)
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
  Note() {
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
  Note(int32_t id_, float pitch_, float velocity_, float onoff_, float x1_, float x2_, float x3_, float x4_, float ps_)
  {
    id = id_;
    pitch = pitch_;
    velocity = velocity_;
    onoff = onoff_;
    x1 = x1_;
    x2 = x2_;
    x3 = x3_;
    x4 = x4_;
    ps = ps_;
  }
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
