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
// esp32 based sampler
//

//============<identities>============
//
#define MY_GROUP_ID   (10000)
#define MY_ID         (MY_GROUP_ID + 1)
#define MY_SIGN       ("AUDIOOOO")
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
// 'USE_ALTERNATIVE_ADDRESSES'
// --> peer list limited max. 20.
//     so, we have alternative address book that covers after 20th.
//
//==========</list-of-configurations>==========
//
#define DISABLE_AP
#define REPLICATE_NOTE_REQ
#define USE_ALTERNATIVE_ADDRESSES

//============<audioooo-param>============
#define GAIN_MAX 1.0 // if 1.0 is too loud, give max. limit here.
//============<audioooo-param>============

//============<parameters>============
//
#define LED_PERIOD (11111)
#define LED_ONTIME (1)
#define LED_GAPTIME (222)
//
#define SCREEN_PERIOD (200) //200ms = 5hz
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

//screen task
String screen_cmd = "(.........................................                     my_id:" + String(MY_ID);
String screen_filename = "/___.mp3";

//
extern Task screen_cmd_notify_task;
bool cmd_notify = false;
void screen_cmd_notify() {
  if (screen_cmd_notify_task.isFirstIteration()) cmd_notify = true;
  else if (screen_cmd_notify_task.isLastIteration()) cmd_notify = false;
  else cmd_notify = !cmd_notify;
}
Task screen_cmd_notify_task(500, 10, &screen_cmd_notify, &runner, false);

//
extern Task screen_task;
void screen() {
  //clear screen + a
  int line_step = 12;
  int line = 0;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  //line1 - mode line (playing / stopped) + notify mark
  display.setCursor(0, line);
  if (audio.isRunning()) display.println("= playing ===");
  else display.println("* stopped !:.");
  if (cmd_notify) {
    display.setCursor(120, line);
    display.println("*");
  }
  line += line_step;

  //line2 - filename
  display.setCursor(0, line);
  display.println(screen_filename.c_str());
  line += line_step;

  //line3 - rf. last msg.
  display.setCursor(0, line);
  display.println(screen_cmd.c_str());
  line += line_step;
  //
  display.display();
  //
}
Task screen_task(SCREEN_PERIOD, TASK_FOREVER, &screen, &runner, true);

//sample #
int sample_now = 0; //0~999

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
#define NEW_NOTE_TIMEOUT (3000)
static unsigned long new_note_time = (-1*NEW_NOTE_TIMEOUT);
void repeat() {
  //
  uint8_t frm_size = sizeof(Note) + 2;
  uint8_t frm[frm_size];
  frm[0] = '[';
  memcpy(frm + 1, (uint8_t *) &note_now, sizeof(Note));
  frm[frm_size - 1] = ']';
  //
  esp_now_send(NULL, frm, frm_size); // to all peers in the list.
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
  esp_now_send(NULL, frm, frm_size); // to all peers in the list.
  //
  // MONITORING_SERIAL.write(frm, frm_size);
  // MONITORING_SERIAL.println(" ==(esp_now_send/0)==> ");
  //
  if (hello_delay > 0) {
    if (hello_delay < 100) hello_delay = 100;
    hello_task.restartDelayed(hello_delay);
  }

  // //TEST
  // Note n = {
  //   10001, // int32_t id;
  //   1, // float pitch;
  //   127, // float velocity;
  //   1, // float onoff;
  //   0, // float x1;
  //   0, // float x2;
  //   0, // float x3;
  //   0, // float x4;
  //   5000 // float ps;
  // };
  // note_now = n;
  // repeat_task.restart();
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

// on 'Note'
void onNoteHandler(Note & n) {
  //is it for me?
  if (n.id == MY_GROUP_ID || n.id == MY_ID) {
    //
    screen_cmd = n.to_string();
    screen_cmd_notify_task.restart();
    //
    audio.setVolume(n.velocity * 21 / 127 * GAIN_MAX); // 0...127 ==> 0...21
    //
    if (n.onoff == 1) {
      // filter out re-triggering same note while it is playing.
      if (!audio.isRunning() || sample_now != n.pitch) {
        sample_now = n.pitch;
        sample_player_start_task.restartDelayed(10);
      }
    } else if (n.onoff == 0) {
      sample_now = n.pitch;
      sample_player_stop_task.restartDelayed(10);
    }
    //
  }
}

// on 'receive'
void onDataReceive(const uint8_t * mac, const uint8_t *incomingData, int32_t len) {

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

    //
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
  if (sendStatus != 0) MONITORING_SERIAL.println("Delivery failed!");
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
  audio.setVolume(21);   // 0...21
  // audio.connecttoFS(SD, filename.c_str());

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
  // esp_now_peer_info_t peerInfo;
  // memcpy(peerInfo.peer_addr, broadcastmac, 6);
  // peerInfo.channel = 0;
  // peerInfo.encrypt = false;
  // esp_now_add_peer(&peerInfo);

#if defined(USE_ALTERNATIVE_ADDRESSES)
  AddressBook * book = lib.getBookByTitle("audioooo alt");
#else
  AddressBook * book = lib.getBookByTitle("audioooo");
#endif
  for (int idx = 0; idx < book->list.size(); idx++) {
    Serial.println("- ! (esp_now_add_peer) ==> add a '" + book->list[idx].name + "'.");
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, book->list[idx].mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
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
