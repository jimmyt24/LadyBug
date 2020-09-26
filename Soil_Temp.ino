#include <OneWire.h>

OneWire SoilTempDevice(10);

byte SPdata[12];
byte TempDeviceAddr[8];

void setup(void) {
  Serial.begin(9600);
}

void loop() {
  CheckSoilTempDevice();
  float celsius = GetSoilTemp();
  Serial.println(celsius);
  CheckSoilTempDevice();
  delay(3000);
}

void CheckSoilTempDevice()
{
  byte i;
  byte type_s;
  
  if ( !SoilTempDevice.search(TempDeviceAddr)) {
    SoilTempDevice.reset_search();
    delay(250);
    return;
  }

  if (OneWire::crc8(TempDeviceAddr, 7) != TempDeviceAddr[7]) {
      return;
  }

  switch (TempDeviceAddr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      return;
  } 
  
  SoilTempDevice.reset();
  SoilTempDevice.select(TempDeviceAddr);
  SoilTempDevice.write(0x44, 0); // start conversion
  delay(1000);
}

float GetSoilTemp()
{
  byte i;
  byte present;
  float temp;

  present = SoilTempDevice.reset();
  SoilTempDevice.select(TempDeviceAddr);
  SoilTempDevice.write(0xBE);   // Read Scratchpad where temp is stored

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    SPdata[i] = SoilTempDevice.read();
  }

  int16_t raw = (SPdata[1] << 8) | SPdata[0]; // Temp data is stored in first 2 bits
 
  byte cfg = (SPdata[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  

  temp = (float)raw / 16.0;
  return temp;
}
