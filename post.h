#pragma once

//obsolete
#define I2C_ADDR 3
#define POST_LENGTH 32
#define POST_BUFF_LEN (POST_LENGTH + 1)

//esp-now
#define MEMBER_COUNT_MAX (20)
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
};

#include <Vector.h>
struct AddressBook {
  Vector<Address> list;

  //
  AddressBook() {
    //
    list.setStorage(lst);
    //
    // list.push_back(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB7, 0x21, "Enchovy"));
    // list.push_back(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB3, 0xC5, "Schpaarow"));
    //
    list.push_back(Address(0xB4, 0xE6, 0x2D, 0x37, 0x3B, 0x90, "root/osc"));
    list.push_back(Address(0xF4, 0xCF, 0xA2, 0xED, 0xB4, 0x28, "taak/157"));
    //
  }
  //
  AddressBook(String booktitle) {
    //
    // with a 'booktitle' to select which addressebook to get.
    // UNIMPLEMENTED
    //
    //
    list.setStorage(lst);
    //
    if (booktitle == "YELLOW") {
      ;
    }
  }
private:
  Address lst[MEMBER_COUNT_MAX]; //<-- the storage array of 'list'
};

//message type Note : '[' + Note + ']'
struct Note {
  int32_t pitch;
  int32_t velocity;
  int32_t onoff;
  int32_t x1;
  int32_t x2;
  int32_t x3;
  int32_t x4;
  int32_t ps;
  //
  void clear() {
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
    str += "( pitch=" + String(pitch);
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
struct Hello {
  int32_t id;
  int32_t h1;
  int32_t h2;
  int32_t h3;
  int32_t h4;
  //
  void clear() {
    id = 0;
    h1 = 0;
    h2 = 0;
    h3 = 0;
    h4 = 0;
  }
};
