//
// wirelessly connected cloud (based on ESP-NOW, a kind of LPWAN?)
//

//
// Conversation about the ROOT @ SEMA storage, Seoul
//

//
// 2021 02 15
//
// (part-3) teensy35 : 'client:sampler' (mesh post --> play sounds)
//

//==========<list-of-configurations>===========
//
// 'USE_EXTERNAL_ANALOG_REFERENCE'
// --> this was used probably to make output swing greater Vpp == 3.3v
//     but, with this + small headphones, i cannot hear anything.
//     and, this might be dangerous in some case -> this is suspicous of 1 broken teensy 3.6
//     so, unless you are so sure, don't enable this.
// !WARNING! : teensy36 could be destroyed! don't enable it.
//
// 'USE_IDLE_NOISE'
// --> HACK: let auto-poweroff speakers stay turned ON - (creative muvo mini)
//     .NOTE. : creative muvo 2 doesn't need this. they just stay on!
//
// 'USE_LED_INDICATOR'
// --> a light indicator for playing/stopped
//
//==========</list-of-configurations>===========
#define USE_EXTERNAL_ANALOG_REFERENCE
#define USE_IDLE_NOISE
#define USE_LED_INDICATOR

//============<parameters>============
//
#define IDLE_FREQ 50
#define IDLE_AMP 0.05
#define IDLE_OFFTIME 60 //sec
#define IDLE_ONTIME 1 //sec
//
// #define IDLE_FREQ 50
// #define IDLE_AMP 0.05
// #define IDLE_OFFTIME 300 //sec
// #define IDLE_ONTIME 30 //sec
//
// #define IDLE_FREQ 50
// #define IDLE_AMP 0.1
// #define IDLE_OFFTIME 150 //sec
// #define IDLE_ONTIME 15 //sec

//
#define GAIN_FACTOR 1.0 // this is a private multiplier for this module.
//
//============</parameters>============

//teensy audio
#include <Audio.h>
// #include <SPI.h>
// #include <SD.h>
#include <SdFat.h>
SdFatSdioEX SD;
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav playSdWav1;               //xy=224,265
AudioSynthWaveformSine sine1;            //xy=236,361
AudioMixer4 mixer2;                      //xy=497,328
AudioMixer4 mixer1;                      //xy=499,245
AudioAmplifier amp1;                     //xy=633,245
AudioAmplifier amp2;                     //xy=634,328
AudioOutputAnalogStereo dacs1;           //xy=788,284
AudioConnection patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection patchCord2(playSdWav1, 1, mixer2, 0);
AudioConnection patchCord3(sine1, 0, mixer1, 1);
AudioConnection patchCord4(sine1, 0, mixer2, 1);
AudioConnection patchCord5(mixer2, amp2);
AudioConnection patchCord6(mixer1, amp1);
AudioConnection patchCord7(amp1, 0, dacs1, 0);
AudioConnection patchCord8(amp2, 0, dacs1, 1);
// GUItool: end automatically generated code

//task
#include <TaskScheduler.h>
Scheduler runner;
extern Task idle_noise_task;
//sample #
int sample_now = 0; //0~99
void sample_player_start() {
  //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
  char filename[13] = "NNN.WAV";
  //search for the sound file
  int note = sample_now;
  int nnn = (note % 1000);  // 0~999
  int nn =  (note % 100);   // 0~99
  filename[0] = '0' + (nnn / 100); // N__.WAV
  filename[1] = '0' + (nn / 10);   // _N_.WAV
  filename[2] = '0' + (nn % 10);   // __N.WAV
  //TEST
  Serial.println(filename);
  AudioNoInterrupts();
  bool test = SD.exists(filename);
  AudioInterrupts();
  if (!test) {
    Serial.println("... does not exist.");
    return;
  }
  //start the player!
  playSdWav1.play(filename);
  //to wait a bit for updating isPlaying()
  delay(10);
}
void sample_player_stop() {
  //if note == 0, just stop immediately w/o checking
  // + and this way of checking is not nice (AudioNoInterrupts + test + AudioInterrupts)
  //   what if sound files list could be generated @ setup time and later just react on that list?
  //   i think that way is much more stable than stopping interrupts. (-> breaks audio engine sometimes.)
  if (sample_now != 0) {
    //filename buffer - 8.3 naming convension! 8+1+3+1 = 13
    char filename[13] = "NNN.WAV";
    //search for the sound file
    int note = sample_now;
    int nnn = (note % 1000);  // 0~999
    int nn =  (note % 100);   // 0~99
    filename[0] = '0' + (nnn / 100); // N__.WAV
    filename[1] = '0' + (nn / 10);   // _N_.WAV
    filename[2] = '0' + (nn % 10);   // __N.WAV
    //TEST
    Serial.println(filename);
    AudioNoInterrupts();
    bool test = SD.exists(filename);
    AudioInterrupts();
    if (!test) {
      Serial.println("... does not exist.");
      return;
    }
  }
  //stop the player.
  if (playSdWav1.isPlaying() == true) {
    playSdWav1.stop();
  }
}
void sample_player_check() {
  //
  static bool wasplaying = false;
  bool isplaying = playSdWav1.isPlaying();
  // Serial.println("isplaying: " + String(isplaying));
  //
#if defined(USE_LED_INDICATOR)
  if (isplaying == true) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }
#endif
  //
#if defined(USE_IDLE_NOISE)
  if (wasplaying == false && isplaying == true) { //rising edge event
    idle_noise_task.disable();
    sine1.amplitude(0);
  }
  else if (wasplaying == true && isplaying == false) { //falling edge event
    idle_noise_task.restart();
  }
#endif
  //
  wasplaying = isplaying;
}
#if defined(USE_IDLE_NOISE)
void idle_noise() {
  //
  static int elapsed_sec = 0;
  static bool active = false;
  if (idle_noise_task.isFirstIteration()) {
    elapsed_sec = 0;
    active = false;
    sine1.amplitude(0);
  } else {
    elapsed_sec++;
  }
  //
  if (active == false && elapsed_sec >= IDLE_OFFTIME) {
    active = true;
    elapsed_sec = 0;
    sine1.amplitude(IDLE_AMP);
  } else if (active == true && elapsed_sec >= IDLE_ONTIME) {
    active = false;
    elapsed_sec = 0;
    sine1.amplitude(0);
  }
}
Task idle_noise_task(1000, TASK_FOREVER, idle_noise, &runner, true);
#endif
//
Task sample_player_start_task(0, TASK_ONCE, sample_player_start, &runner, false);
Task sample_player_stop_task(0, TASK_ONCE, sample_player_stop, &runner, false);
Task sample_player_check_task(0, TASK_FOREVER, sample_player_check, &runner, true);

//i2c
#include <Wire.h>
#include "../../post.h"
// DISABLED.. due to bi-directional I2C hardship. ==> use UART.
// void requestEvent() {
//   Wire.write(" "); // no letter to send
// }
void receiveEvent(int numBytes) {
  //numBytes : how many bytes received(==available)
  static char letter_intro[POST_BUFF_LEN] = "................................";

  // Serial.println("[i2c] on receive!");
  int nb = Wire.readBytes(letter_intro, POST_LENGTH);

  if (POST_LENGTH == nb) {

    //convert to String
    String msg = String(letter_intro);
    Serial.println(msg);

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
    // Serial.println(str_key);
    // Serial.println(str_velocity);
    // Serial.println(str_gate);

    //
    int key = str_key.toInt();
    // sample_now = key;
    //
    int velocity = str_velocity.toInt();   // 0 ~ 127
    float amp_gain = (float)velocity / 127.0;
    //
    amp_gain = amp_gain * GAIN_FACTOR;
    //
    amp1.gain(amp_gain);
    amp2.gain(amp_gain);
    //
    int gate = str_gate.toInt();
    if (gate == 1) { //on 'start'
      // filter out re-triggering same note while it is playing.
      if (!playSdWav1.isPlaying() || sample_now != key) {
        sample_now = key;
        sample_player_start_task.restart();
        Serial.println("sample_player_start_task");
      }
    } else if (gate == 0) { //on 'stop'
      sample_now = key;
      sample_player_stop_task.restart();
      Serial.println("sample_player_stop_task");
    }
  }
}

// SD TEST
void printDirectory(File dir, int numTabs) {
  char filename[256] = "";
  while(true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i=0; i<numTabs; i++) {
      Serial.print('\t');
    }
    entry.getName(filename, 256);
    Serial.print(filename);
    // Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      //non-recursive listing...
      // printDirectory(entry, numTabs+1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

//
File root;
void setup() {

  //serial monitor
  Serial.begin(115200);
  //
  delay(50);
  // <-- strange? but, this was needed !!
  // w/o ==> get killed by watchdog.. :(
  //
  // while (!Serial) {}
  //  --> enable this.. to use Serial. otherwise, very jerky/unstable..

  //i2c
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveEvent);
  // DISABLED.. due to bi-directional I2C hardship. ==> use UART.
  // Wire.onRequest(requestEvent);

  //SD - AudioPlaySdWav @ teensy audio library needs SD.begin() first. don't forget/ignore!
  //+ let's additionally check contents of SD.
  if (!SD.begin()) {
    // if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("[sd] initialization failed!");
    return;
  }
  Serial.println("[sd] initialization done.");
  root = SD.open("/");
  printDirectory(root, 0);

  //audio
  AudioMemory(20);

  #if defined(USE_EXTERNAL_ANALOG_REFERENCE) && !defined(TEENSY36)
  Serial.println("- ======== 'USE_EXTERNAL_ANALOG_REFERENCE' ========");
  dacs1.analogReference(EXTERNAL);
  // <-- this was used probably to make output swing greater Vpp == 3.3v
  //     but, with this + small headphones, i cannot hear anything.
  //     and, this might be dangerous in some case -> this is suspicous of 1 broken teensy 3.6
  //     so, unless you are so sure, don't enable this.
  #endif

  mixer1.gain(0,1.0);
  mixer1.gain(1,1.0);
  mixer1.gain(2,0);
  mixer1.gain(3,0);
  mixer2.gain(0,1.0);
  mixer2.gain(1,1.0);
  mixer2.gain(2,0);
  mixer2.gain(3,0);
  amp1.gain(GAIN_FACTOR);
  amp2.gain(GAIN_FACTOR);

  //let auto-poweroff speakers stay turned ON!
  sine1.frequency(IDLE_FREQ);
  sine1.amplitude(0);

  //led
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); // LOW: OFF

  //
  Serial.println("[setup] done.");
}

void loop() {
  runner.execute();
}
