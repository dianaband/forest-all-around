//
// a controller w/ 6 knobs (or pots)
//

//
// Forest all/around @ MMCA, Seoul
//

//
// 2020 10 14
//

#include <Arduino.h>
#include <HystFilter.h>
#include <Firmata.h>

// 10 bit ADC = 1024, 64 discrete output values required, margin = 3 units (of 1024)
HystFilter potA( 1024, 64, 3 );
HystFilter potB( 1024, 64, 3 );
HystFilter potC( 1024, 64, 3 );
HystFilter potD( 1024, 64, 3 );
HystFilter potE( 1024, 64, 3 );
HystFilter potF( 1024, 64, 3 );

void analogWriteCallback(byte pin, int value) {
}

void setup() {
  Firmata.setFirmwareVersion(FIRMATA_FIRMWARE_MAJOR_VERSION, FIRMATA_FIRMWARE_MINOR_VERSION);
  Firmata.attach(ANALOG_MESSAGE, analogWriteCallback);
  Firmata.begin(57600);
}

void loop() {
  Firmata.sendAnalog(0, potA.getOutputLevel(analogRead(0)));
  Firmata.sendAnalog(1, potB.getOutputLevel(analogRead(1)));
  Firmata.sendAnalog(2, potC.getOutputLevel(analogRead(2)));
  Firmata.sendAnalog(3, potD.getOutputLevel(analogRead(3)));
  Firmata.sendAnalog(4, potE.getOutputLevel(analogRead(4)));
  Firmata.sendAnalog(5, potF.getOutputLevel(analogRead(5)));

  delay(50);
}
