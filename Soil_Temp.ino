#include <OneWire.h>

OneWire SoilTempDevice(10);

byte SPdata[12];
byte TempDeviceAddr[8];
unsigned long hour = 3600*1000UL;

void setup(void) {
  Serial.begin(9600);
}

void loop() {
  CheckSoilTempDevice();
  float celsius = GetSoilTemp();
  Serial.println(celsius);
  CheckSoilTempDevice();
  SoilTempDevice.depower();
  delay(100);
}

void CheckSoilTempDevice()
{
  byte i;

  // Checks for device by searching for address
  // Device address sent to TempDeviceAddr
  // If statement begins if search returns false or
  // no other devices present
  if ( !SoilTempDevice.search(TempDeviceAddr)) {
    SoilTempDevice.reset_search();                
    delay(250);
    return;
  }

  // Mandatory CRC check that the address is valid and the device is
  // functioning correctly
  if (OneWire::crc8(TempDeviceAddr, 7) != TempDeviceAddr[7]) {
      return;
  }

  // Starts a temperature conversion by sending the appropriate
  // command
  SoilTempDevice.reset();
  SoilTempDevice.select(TempDeviceAddr);
  SoilTempDevice.write(0x44, 0); // start conversion
  delay(800); // appropriate delay for conversion process
  // 750 ms conversion time
}

float GetSoilTemp()
{
  byte i;
  float temp;

  SoilTempDevice.reset();
  SoilTempDevice.select(TempDeviceAddr);
  SoilTempDevice.write(0xBE);   // Read Scratchpad where temp is stored

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    SPdata[i] = SoilTempDevice.read();
  }

  if (OneWire::crc8(SPdata, 8) != SPdata[8]) {
    return 1000;
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
