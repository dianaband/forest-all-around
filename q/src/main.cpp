//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// 0set performance 'Georidugi'/Distancing
// @ 2021 Jun 15 ~ 17
//

//
// 2021 june
//
// esp32 based sampler + 'note' sequence generator!
//

//============<identities>============
//
#define MY_GROUP_ID         (70000)
#define MY_ID               (MY_GROUP_ID + 1)
#define MY_SIGN             ("Q")
#define ADDRESSBOOK_TITLE   ("broadcast only")
//
//============</identities>============

//==========<list-of-configurations>===========
//
// 'HAVE_CLIENT'
// --> i have a client. enable the client task.
//
// 'DISABLE_AP'
// --> (questioning)...
//
// 'REPLICATE_NOTE_REQ' (+ N_SEC_BLOCKING_NOTE_REQ)
// --> for supporting wider area with simple esp_now protocol,
//     all receipents will replicate NOTE msg. when they are newly appeared.
//   + then, network would be flooded by infinite duplicating msg.,
//     unless they stop reacting to 'known' req. for some seconds. (e.g. 3 seconds)
//
// 'GEN_NOTE_REQ'
// --> this will generate 'note' msg.
//
// 'ADDRESSBOOK_TITLE'
// --> peer list limited max. 20.
//     so, we might use different address books for each node to cover a network of more than 20 nodes.
//
//==========</list-of-configurations>==========
//
#define DISABLE_AP
#define GEN_NOTE_REQ

//============<parameters>============
//
#define LED_PERIOD (11111)
#define LED_ONTIME (1)
#define LED_GAPTIME (222)
//
#define SCREEN_PERIOD (200) //200ms = 5hz
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
#elif defined(ARDUINO_ESP32_DEV) // esp32dev (MakePython ESP32 => Have NO LED)
#define LED_PIN 2
#else
#define LED_PIN 2
#endif
//============</board-specifics>===========

//arduino
#include <Arduino.h>

//post & addresses
#include "../../post.h"

//espnow
#include <esp_now.h>
#include <WiFi.h>
AddressLibrary lib;

//task
#include <TaskScheduler.h>
Scheduler runner;

//screen
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define MAKEPYTHON_ESP32_SDA 4
#define MAKEPYTHON_ESP32_SCL 5
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//-*-*-*-*-*-*-*-*-*-*-*-*-
// my tasks
// audio & sd & filesystem
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
//sdcard
#define SD_CS 22
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
//digital i/o used (for) makerfabs audio v2.0
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25
Audio audio;

// q list
std::vector<Note> qlist;
void init_qlist() {
  // tech. rehearsal @ May 21
  qlist.push_back(Note(10000,   1, 0, 1, 0, 0, 0, 0, 0)); //velocity == 0 ~~> don't change volume.
  qlist.push_back(Note(10000, 101, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 102, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 103, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 104, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 105, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 106, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 107, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 108, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 109, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 110, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 111, 0, 1, 0, 0, 0, 0, 0));
  qlist.push_back(Note(10000, 112, 0, 1, 0, 0, 0, 0, 0));

}

//buttons
const int pin_vol_up = 39;
const int pin_vol_down = 36;
const int pin_mute = 35;
const int pin_previous = 15;
const int pin_pause = 33;
const int pin_next = 2;

//sample #
int sample_now = 0; //0~999
extern Task sample_player_start_task;
extern Task sample_player_stop_task;

//repeat
#define NEW_NOTE_TIMEOUT (3000)
static unsigned long new_note_time = (-1*NEW_NOTE_TIMEOUT);

//screen task
String screen_cmd = ":Q: ----- --- --- -";
String screen_filename = "***.mp3";

//
extern Task screen_req_notify_task;
bool req_notify = false;
void screen_req_notify() {
  if (screen_req_notify_task.isFirstIteration()) req_notify = true;
  else if (screen_req_notify_task.isLastIteration()) req_notify = false;
  else req_notify = !req_notify;
}
Task screen_req_notify_task(500, 8, &screen_req_notify, &runner, false);

//
extern Task screen_task;
void screen() {

  //
  if (screen_task.isFirstIteration()) {
    init_qlist();
  }

  // button job!
  static int btn_vol_up = 0;
  static int btn_vol_down = 0;
  static int btn_mute = 0;
  static int btn_previous = 0;
  static int btn_pause = 0;
  static int btn_next = 0;
  //
  int a;
  static int qselect = 0;
  bool nowsend = false;
  //
  a = digitalRead(pin_vol_up);
  if(btn_vol_up != a) {
    btn_vol_up = a;
    if(a == 0) {
      // 'vol_up' button pressed.

      qselect = qselect + 1;
      if (qselect > (qlist.size() - 1)) qselect = 0;
    }
  }
  //
  a = digitalRead(pin_vol_down);
  if(btn_vol_down != a) {
    btn_vol_down = a;
    if(a == 0) {
      // 'vol_down' button pressed.

      qselect = qselect - 1;
      if (qselect < 0) qselect = (qlist.size() - 1);
    }
  }
  //
  a = digitalRead(pin_mute);
  if(btn_mute != a) {
    btn_mute = a;
    if(a == 0) {
      // 'mute' button pressed.

      nowsend = true;
    }
  }
  //
  a = digitalRead(pin_previous);
  if(btn_previous != a) {
    btn_previous = a;
    if(a == 0) {
      // 'previous' button pressed.

      qselect = qselect - 1;
      if (qselect < 0) qselect = (qlist.size() - 1);
    }
  }
  //
  a = digitalRead(pin_pause);
  if(btn_pause != a) {
    btn_pause = a;
    if(a == 0) {
      // 'pause' button pressed.

      nowsend = true;
    }
  }
  //
  a = digitalRead(pin_next);
  if(btn_next != a) {
    btn_next = a;
    if(a == 0) {
      // 'next' button pressed.

      qselect = qselect + 1;
      if (qselect > (qlist.size() - 1)) qselect = 0;
    }
  }

  if (nowsend == true) {
    nowsend = false;
    // create a NOTE req. and send it out.
    //
    MONITORING_SERIAL.print("# posting a req.# ==> ");
    MONITORING_SERIAL.println(qlist[qselect].to_string());
    //
    uint8_t frm_size = sizeof(Note) + 2;
    uint8_t frm[frm_size];
    frm[0] = '[';
    memcpy(frm + 1, (uint8_t *) &qlist[qselect], sizeof(Note));
    frm[frm_size - 1] = ']';
    //
    // strange but following didn't work as expected. (instead, i have to send one-by-one.)
    // esp_now_send(NULL, frm, frm_size); // to all peers in the list.

    // so, forget about peer list -> just pick a broadcast peer to be sent.
    uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastmac, frm, frm_size);

    // (DEBUG) fetch full peer list
    { PeerLister a; a.print(); }

    //+ fancy stuff
    screen_req_notify_task.restart();

    //+ play start for myself
    sample_now = qlist[qselect].pitch;
    sample_player_start_task.restartDelayed(10);
    new_note_time = millis(); // also, block for some time.

    //+ automatically increase 'song_request'
    qselect = qselect + 1;
    if (qselect > (qlist.size() - 1)) qselect = 0;
  }

  //clear screen + a
  int line_step = 12;
  int line = 0;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  //button status (DEBUG)
  // int b = digitalRead(pin_vol_up);
  // b = b*10 + digitalRead(pin_vol_down);
  // b = b*10 + digitalRead(pin_mute);
  // b = b*10 + digitalRead(pin_previous);
  // b = b*10 + digitalRead(pin_pause);
  // b = b*10 + digitalRead(pin_next);

  //line1 - mode line (playing / stopped) + notify mark
  display.setCursor(0, line);
  if (audio.isRunning()) {
    // display.printf("%06d= playing ===", b); // (DEBUG)
    display.printf("= playing ===");
  } else {
    // display.printf("%06d* stopped !:.", b); // (DEBUG)
    display.printf("* stopped !:.");
  }
  line += line_step;

  //line2 - filename
  display.setCursor(0, line);
  display.println(screen_filename.c_str());
  line += line_step;

  //line3 - msg. ready to be sent now.
  display.setCursor(0, line);
  display.printf(":Q: %05d %03d %03d %01d", (int)qlist[qselect].id, (int)qlist[qselect].pitch, (int)qlist[qselect].velocity, (int)qlist[qselect].onoff);
  line += line_step;

  //line4 - big file name display
  display.setCursor(0, line);
  display.setTextSize(2);
  //
  char filename[14] = "/NNN.mp3";
  int note = qlist[qselect].pitch;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[1] = '0' + (nnn / 100); // N__.MP3
  filename[2] = '0' + (nn / 10);   // _N_.MP3
  filename[3] = '0' + (nn % 10);   // __N.MP3
  //
  display.println(filename);
  line += line_step;
  display.setTextSize(1);
  line += 8;

  // //alternative line3+4 - full msg. dump (DEBUG/MONITORING)
  // display.setCursor(0, line);
  // display.println(":Q: " + qlist[qselect].to_string());
  // line += line_step;

  //line5 - song req. tx. notify. (GEN_NOTE_REQ)
  display.setCursor(0, line);
  if (req_notify) {
    display.setCursor(25, line);
    display.println("~~ d[+=+]b ~~ >>>");
  }
  line += line_step;

  //
  display.display();
  //
}
Task screen_task(SCREEN_PERIOD, TASK_FOREVER, &screen, &runner, true);

//on 'start'
void sample_player_start()
{
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  // + '/' root symbol 13+1 = 14 (ESP32 specific?)
  char filename[14] = "/NNN.mp3";
  //search for the sound file
  int note = sample_now;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[1] = '0' + (nnn / 100); // N__.MP3
  filename[2] = '0' + (nn / 10);   // _N_.MP3
  filename[3] = '0' + (nn % 10);   // __N.MP3
  //TEST
  Serial.println(filename);
  screen_filename = String(filename);
  bool test = SD.exists(filename);
  if (!test) {
    Serial.println("... does not exist.");
    screen_filename = screen_filename + "_!NEXIST!";
    return;
  }
  //start the player!
  audio.connecttoSD(filename);
  delay(10);
}
Task sample_player_start_task(0, TASK_ONCE, &sample_player_start, &runner, false);

//on 'stop'
void sample_player_stop() {
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  // + '/' root symbol 13+1 = 14 (ESP32 specific?)
  char filename[14] = "/NNN.mp3";
  //search for the sound file
  int note = sample_now;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[1] = '0' + (nnn / 100); // N__.MP3
  filename[2] = '0' + (nn / 10);   // _N_.MP3
  filename[3] = '0' + (nn % 10);   // __N.MP3
  //TEST
  Serial.println(filename);
  screen_filename = String(filename);
  bool test = SD.exists(filename);
  if (!test) {
    Serial.println("... does not exist.");
    screen_filename = screen_filename + "_!NEXIST!";
    return;
  }
  //stop the player.
  audio.stopSong();
}
Task sample_player_stop_task(0, TASK_ONCE, &sample_player_stop, &runner, false);

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
void repeat() {
  //
  uint8_t frm_size = sizeof(Note) + 2;
  uint8_t frm[frm_size];
  frm[0] = '[';
  memcpy(frm + 1, (uint8_t *) &note_now, sizeof(Note));
  frm[frm_size - 1] = ']';
  //
  // strange but following didn't work as expected. (instead, i have to send one-by-one.)
  // esp_now_send(NULL, frm, frm_size); // to all peers in the list.

  // so, forget about peer list -> just pick a broadcast peer to be sent.
  uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastmac, frm, frm_size);

  // (DEBUG) fetch full peer list
  { PeerLister a; a.print(); }

  //
  MONITORING_SERIAL.print("repeat! ==> ");
  MONITORING_SERIAL.println(note_now.to_string());
}
Task repeat_task(0, TASK_ONCE, &repeat, &runner, false);
#endif
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
  hello.h2 = sample_now;
  // hello.h3 = 0;
  // hello.h4 = 0;
  //
  uint8_t frm_size = sizeof(Hello) + 2;
  uint8_t frm[frm_size];
  frm[0] = '{';
  memcpy(frm + 1, (uint8_t *) &hello, sizeof(Hello));
  frm[frm_size - 1] = '}';
  //
  // strange but following didn't work as expected. (instead, i have to send one-by-one.)
  // esp_now_send(NULL, frm, frm_size); // to all peers in the list.

  // so, forget about peer list -> just pick a broadcast peer to be sent.
  uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastmac, frm, frm_size);
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
Task blink_task(0, TASK_FOREVER, &blink, &runner, false); // makepython esp32 has NO led => disabled.

// on 'receive'
void onDataReceive(const uint8_t * mac, const uint8_t *incomingData, int32_t len) {

  //
  // MONITORING_SERIAL.write(incomingData, len);

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
    //onNoteHandler(note);

    //is it for me?
    if (note.id == MY_GROUP_ID || note.id == MY_ID) {
      hello_delay = note.ps;
      if (hello_delay > 0 && hello_task.isEnabled() == false) {
        hello_task.restart();
      }
    }

    MONITORING_SERIAL.println(note.to_string());

    #if defined(REPLICATE_NOTE_REQ)
    if (millis() - new_note_time > NEW_NOTE_TIMEOUT) {
      note_now = note;
      repeat_task.restart();
      new_note_time = millis();
    }
    #endif
  }
}

// on 'sent'
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  // well, i think this cb should be called once for EVERY single TX attempts,
  //   but in reality, it doesn't get called that often.
  //   i think this is sorta bug. but have no clear clue.
  MONITORING_SERIAL.printf("* delivery attempt! ~~~>>> %02X:%02X:%02X:%02X:%02X:%02X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  if (sendStatus != 0) MONITORING_SERIAL.printf("* ==>> FAILED :(\n\n");
}

// SD TEST
void printDirectory(File dir, int numTabs) {
  // char filename[256] = "";
  while(true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i=0; i<numTabs; i++) {
      Serial.print('\t');
    }
    // entry.getName(filename, 256);
    // Serial.print(filename);
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs+1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void lcd_text(String str) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(str.c_str());
  display.display();
}

//
File root;
void setup() {

  //led
  pinMode(LED_PIN, OUTPUT);

  //buttons
  pinMode(pin_vol_up, INPUT_PULLUP);
  pinMode(pin_vol_down, INPUT_PULLUP);
  pinMode(pin_mute, INPUT_PULLUP);
  pinMode(pin_previous, INPUT_PULLUP);
  pinMode(pin_pause, INPUT_PULLUP);
  pinMode(pin_next, INPUT_PULLUP);

  //serial
  Serial.begin(115200);
  delay(100);

  //screen
  Wire.begin(MAKEPYTHON_ESP32_SDA, MAKEPYTHON_ESP32_SCL);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;         // Don't proceed, loop forever
  }
  display.clearDisplay();

  //SD(SPI)
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1000000);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("Card Mount Failed");
    lcd_text("SD ERR!\nCard Mount Failed!");
    while (1)
      ;
  } else {
    // lcd_text("SD OK");
  }
  root = SD.open("/");
  printDirectory(root, 0);

  //audio(I2S)
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // audio.setVolume(21);   // 0...21
  audio.setVolume(14.88);   // 90 * 21 / 127 == 14.88
  // audio.connecttoFS(SD, filename.c_str());

  //info
  Serial.println();
  Serial.println();
  Serial.println("\"hi, i m your postman.\"");
  Serial.println("-");
  Serial.println("- my id: " + String(MY_ID) + ", gid: " + String(MY_GROUP_ID) + ", call me ==> \"" + String(MY_SIGN) + "\"");
  Serial.println("- mac address: " + WiFi.macAddress() + ", channel: " + String(WIFI_CHANNEL));
  Serial.println("- my peer book ==> \"" + String(ADDRESSBOOK_TITLE) + "\"");
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
  // system_phy_set_max_tpw(0);
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm); // Set WiFi RF power output to lowest level
  node_type = WIFI_STA;
#endif
  WiFi.mode(node_type);

  //esp-now
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceive);
  //
  // Serial.println("- ! (esp_now_add_peer) ==> add a 'broadcast peer' (FF:FF:FF:FF:FF:FF).");
  // uint8_t broadcastmac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //
  // //
  // esp_now_peer_info_t peerInfo = {};
  // memcpy(peerInfo.peer_addr, broadcastmac, 6);
  // peerInfo.channel = 0;
  // peerInfo.encrypt = false;
  // esp_now_add_peer(&peerInfo);

  AddressBook * book = lib.getBookByTitle(ADDRESSBOOK_TITLE);
  if (book == NULL) {
    Serial.println("- ! wrong book !! : \"" + String(ADDRESSBOOK_TITLE) + "\""); while(1);
  }
  for (int idx = 0; idx < book->list.size(); idx++) {
    Serial.println("- ! (esp_now_add_peer) ==> add a '" + book->list[idx].name + "'.");
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, book->list[idx].mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
  // (DEBUG) fetch full peer list
  { PeerLister a; a.print(); }
  //
  Serial.println("-");
  Serial.println("\".-.-.-. :)\"");
  Serial.println();
}

void loop() {
  //
  audio.loop();
  //
  runner.execute();
  //
}
