
//Design 3 Lady_Bug_Test_V1 "LadyBug"
// Author: David Tucker, Isaac Munoz

//include libraries

#include <SoftwareSerial.h>
#include <OneWire.h> // for soil temp
#include <dht11.h> //for temp and humidity sensor

//set pins
dht11 DHT;
#define DHT11_PIN 8 // pin the dht11 is connected to
OneWire SoilTempDevice(10);
SoftwareSerial wifiSerial(2, 3); // RX, TX pins

// define global variables
int HourTimer = 3599; // set counter so measerment will be taken as soon as device is turned on;
int AlarmTimer = 0; // a timer to count a 10 second delay between each low water call
//veriable to store connection status
bool C_0 = false; // client 0 
bool C_1 = false; // client 1 
bool C_2 = false; // client 2 
bool C_3 = false; // client 3 
bool C_4 = false; // client 4 

//sensors
bool WaterLow = false; // flag to be set when water is low

int LDR_Raw; // to store raw light value
int light; // for actual light

float S_celsius = 0; // to store soil temp

int moistvalue = 0;// to store the water level

float humidity = 0; // air humidty

float temp = 0; // air temp

byte SPdata[12]; // used for soilt temp
byte TempDeviceAddr[8]; // used for soil temp

// WiFi
// string to hold variable messages to send to wifi
String Light_S = "";
String SoilTemp_S = "";
String MoistVal_S = "";
String AirHumidity_S = "";
String AirTemp_S = "";
int message_length = 0; //variable to store how many bytes long the message to the esp is

String wifiServerMode = "AT+CIPMUX=1"; //WiFi connection mode to set up multiple connections
String wifiServerStart = "AT+CIPSERVER=1,80"; // starts TCP server on port 80
String wifiMessage = ""; // String to store message from ESP
bool wifiMessageComplete = false; //flag to show the end of a message

void setup() {
  //start serials
  // open serial though USB to communicate with PC for debugging
  Serial.begin(9600); 
  // open serial though pins 10 and 11 to communicate with EPS8266 for Wi-Fi connection
  wifiSerial.begin(9600); // set to same buad rate as ESP8266!
  wifiSerial.println("ATE0"); //stop ESP from echoeing commands so not to waste time reading them
  delay(1000); // delay for ESP to process
  wifiSerial.println(wifiServerMode); //set connection mode to multiple connections
  delay(1000); // delay for ESP to process
  wifiSerial.println(wifiServerStart); // start the server on port 80
  delay(1000); // delay for ESP to process
}

// define functions
//soil moisture
void moisture(){
  moistvalue = analogRead(A0);
  if (moistvalue <= 300){
   // Serial.print("Moisture Sensor Value:");
   // Serial.println(analogRead(A0),":");  
    Serial.println("Dry Soil");
    WaterLow = true;
   }
   if (moistvalue > 300 & moistvalue <= 700){
    //Serial.print("Moisture Sensor Value:");
   // Serial.println(analogRead(A0),":");  
    Serial.println("Humid Soil");
    WaterLow = false;
   }
   if (moistvalue > 700 & moistvalue <= 950){
    //Serial.print("Moisture Sensor Value:");
   // Serial.println(analogRead(A0),":");  
    Serial.println("In Water");
    WaterLow = false;
   }
  
}
// soil temp
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

  
  // Mandatory CRC check that the Scratchpad data is valid
  // Returns 1000 temperature if something failed
  if (OneWire::crc8(SPdata, 8) != SPdata[8]) {
    return 1000;
  }

  int16_t raw = (SPdata[1] << 8) | SPdata[0]; // Temp data is stored in first 2 bits of data[1]
 
  byte cfg = (SPdata[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time

  temp = (float)raw / 16.0;
  return temp;
}
// function to read data from ESP
void ReadFromWiFi()
{
  
    while (wifiMessageComplete != true)
    {
      // recieved next byte
      char inByte = (char)wifiSerial.read();
      //check if the inByate equals the end line message
      if (inByte == '\n')  
      {
          wifiMessageComplete = true; // message complete
      }
      else
      {
          wifiMessage += inByte; // add the next byte to the recived message string
      }
      delay (20); //delay to allow the next byte to arrive from ESP
    }
    wifiMessage.trim(); // remoce excess space before or after the string
    Serial.println(wifiMessage); //debugging
    
    // Now the message has been recieved it can be decoded and acted on
    //first check if there are new connections
  
    if (wifiMessage == "0,CONNECT")
    {
        C_0 = true; // clinet 0 has connected  
        Serial.println("client 0 connected"); //debugging
    }
    else if (wifiMessage == "1,CONNECT")
    {
        C_1 = true; // clinet 1 has connected  
    }
    else if (wifiMessage == "2,CONNECT")
    {
        C_2 = true; // clinet 2 has connected  
    }
    else if (wifiMessage == "3,CONNECT")
    {
        C_3 = true; // clinet 3 has connected  
    }
    else if (wifiMessage == "4,CONNECT")
    {
        C_4 = true; // clinet 4 has connected  
    }

    // second check if any clienst have disconnected 

    else if (wifiMessage == "0,CLOSED")
    {
        C_0 = false; // clinet 0 has disconnected  
        Serial.println("client 0 disconnected"); // degugging
    }
    else if (wifiMessage == "1,CLOSED")
    {
        C_1 = false; // clinet 1 has disconnected  
    }
    else if (wifiMessage == "2,CLOSED")
    {
        C_2 = false; // clinet 2 has disconnected  
    }
    else if (wifiMessage == "3,CLOSED")
    {
        C_3 = false; // clinet 3 has disconnected  
    }
    else if (wifiMessage == "4,CLOSED")
    {
        C_4 = false; // clinet 4 has disconnected  
    }

    //finally check if any clienst are requesting data
    else if (wifiMessage == "+IPD,0,4:Data")
    {
        Serial.println("Sending message to client 0"); //debugging
 
        message_length = Light_S.length()+ 2;
        // call wifi send function with sensor data to client 0 
        SendToWiFi(Light_S,message_length,0); //send light data
        
        message_length = SoilTemp_S.length()+ 2;
        SendToWiFi(SoilTemp_S,message_length,0); //send soil temp data

        message_length = MoistVal_S.length()+ 2;
        SendToWiFi(MoistVal_S,message_length,0); //send water moisture data

        message_length = AirHumidity_S.length()+ 2;
        SendToWiFi(AirHumidity_S,message_length,0); //send air humidity data

        message_length = AirTemp_S.length()+ 2;
        SendToWiFi(AirTemp_S,message_length,0); //send air temp data 
    }
    else if (wifiMessage == "+IPD,1,4:Data")
    {
        // call wifi send function with sensor data to client 1 
        Serial.println("Sending message to client 1"); //debugging
 
        message_length = Light_S.length()+ 2;
        SendToWiFi(Light_S,message_length,1); //send light data
        
        message_length = SoilTemp_S.length()+ 2;
        SendToWiFi(SoilTemp_S,message_length,1); //send soil temp data

        message_length = MoistVal_S.length()+ 2;
        SendToWiFi(MoistVal_S,message_length,1); //send water moisture data

        message_length = AirHumidity_S.length()+ 2;
        SendToWiFi(AirHumidity_S,message_length,1); //send air humidity data

        message_length = AirTemp_S.length()+ 2;
        SendToWiFi(AirTemp_S,message_length,1); //send air temp data 
    }
    else if (wifiMessage == "+IPD,2,4:Data")
    {
        // call wifi send function with sensor data to client 2 
        Serial.println("Sending message to client 2"); //debugging
 
        message_length = Light_S.length()+ 2;
        // call wifi send function with sensor data to client 0 
        SendToWiFi(Light_S,message_length,2); //send light data
        
        message_length = SoilTemp_S.length()+ 2;
        SendToWiFi(SoilTemp_S,message_length,2); //send soil temp data

        message_length = MoistVal_S.length()+ 2;
        SendToWiFi(MoistVal_S,message_length,2); //send water moisture data

        message_length = AirHumidity_S.length()+ 2;
        SendToWiFi(AirHumidity_S,message_length,2); //send air humidity data

        message_length = AirTemp_S.length()+ 2;
        SendToWiFi(AirTemp_S,message_length,2); //send air temp data 
    }
    else if (wifiMessage == "+IPD,3,4:Data")
    {
        Serial.println("Sending message to client 3"); //debugging
 
        message_length = Light_S.length()+ 2;
        // call wifi send function with sensor data to client 0 
        SendToWiFi(Light_S,message_length,3); //send light data
        
        message_length = SoilTemp_S.length()+ 2;
        SendToWiFi(SoilTemp_S,message_length,3); //send soil temp data

        message_length = MoistVal_S.length()+ 2;
        SendToWiFi(MoistVal_S,message_length,3); //send water moisture data

        message_length = AirHumidity_S.length()+ 2;
        SendToWiFi(AirHumidity_S,message_length,3); //send air humidity data

        message_length = AirTemp_S.length()+ 2;
        SendToWiFi(AirTemp_S,message_length,3); //send air temp data 
    }
    else if (wifiMessage == "+IPD,4,4:Data")
    {
        Serial.println("Sending message to client 4"); //debugging
 
        message_length = Light_S.length()+ 2;
        // call wifi send function with sensor data to client 0 
        SendToWiFi(Light_S,message_length,4); //send light data
        
        message_length = SoilTemp_S.length()+ 2;
        SendToWiFi(SoilTemp_S,message_length,4); //send soil temp data

        message_length = MoistVal_S.length()+ 2;
        SendToWiFi(MoistVal_S,message_length,4); //send water moisture data

        message_length = AirHumidity_S.length()+ 2;
        SendToWiFi(AirHumidity_S,message_length,4); //send air humidity data

        message_length = AirTemp_S.length()+ 2;
        SendToWiFi(AirTemp_S,message_length,4); //send air temp data 
    }
    wifiMessage = ""; //reset the message string
    wifiMessageComplete = false; // reset flag
    
}

void SendToWiFi(String Message, int StringLength, int ClientID) // note string length must be the length of the actual message plus 2 to account for the new line characters
{
    int timeout = 0; // counter for 5 seconds, if no respoce from ESP exit send function
    wifiSerial.println("AT+CIPSEND=" + String(ClientID) + "," + String(StringLength));  //send command to ESP telling it to expect a message to "ClintID" of length "StringLength"
    delay(50); // delay for ESP
    wifiSerial.println(Message);  // send the message string
    
    // wait for SEND OK messag. this is done to prevent other messages from overlapping eachother when sending and recieving 
    while(wifiMessage != "SEND OK")
    {
        wifiMessage = ""; //reset the message string
        wifiMessageComplete = false; // reset flag
        while (wifiMessageComplete != true)
        {
          // recieved next byte
          char inByte = (char)wifiSerial.read();
          //check if the inByate equals the end line message
          if (inByte == '\n')  
          {
              wifiMessageComplete = true; // message complete
          }
          else
          {
              wifiMessage += inByte; // add the next byte to the recived message string
          }
          delay (20); //delay to allow the next byte to arrive from ESP
          
          timeout++;
          if (timeout > 249) // if timeout last for 5 seconds exit send function
          {wifiMessage = "SEND OK";}
        }
       
        wifiMessage.trim(); // remoce excess space before or after the string
    }
    wifiMessage = ""; //reset the message string
    wifiMessageComplete = false; // reset flag
    
}

void loop() {
  // first, check if water is low
  /*if (WaterLow)// check if the flag equals true
  {
    if (AlarmTimer > 9) // check if it has been 10 seconds since last low water alarm
    {
        // cheks if clients are connected, if so send warning message
        if (C_0)
        {
            SendToWiFi("WATER ME!",11,0);
        }
        if (C_1)
        {
            SendToWiFi("WATER ME!",11,1);
        }
        if (C_2)
        {
            SendToWiFi("WATER ME!",11,2);
        }
        if (C_3)
        {
            SendToWiFi("WATER ME!",11,3);
        }
        if (C_4)
        {
            SendToWiFi("WATER ME!",11,4);
        }
        AlarmTimer = 0;// reset counter
    }
  }*/

  //second, check if serial message from ESP is available
  if (wifiSerial.available())
  {
    // call function to record string untill end character
    ReadFromWiFi();
  }

  //third, increment counter and delay one second 
  HourTimer++; // increment the counters
  AlarmTimer++;
  delay(1000); //delay 1000 ms or 1 s

  //forth, check if counter equals 3600 and one hour has passed since last recording
  if (HourTimer > 5)// chnage this to adjust how many seconds occure between sensor reads
  {
    // record sensor data
    CheckSoilTempDevice();//initialize soil temp sensor
    S_celsius = GetSoilTemp();//record the soil temp
    LDR_Raw = analogRead(A1);//record the raw LDR data
    light=sensorRawToPhys(LDR_Raw); //conversion to get real light value
    moisture();//get soil moisture, sets flag and saves value to moistvalue
    
    // read tamp and humidity sensor to ensure it is working correctl and print debugging errors
    int check;
    check = DHT.read(DHT11_PIN);    // READ DATA
    switch (check) {
      
    case DHTLIB_OK:
        Serial.print("DHT11 ON");
        break;
    case DHTLIB_ERROR_CHECKSUM:
        Serial.print("DHT11 ERROR: CHECKSUM ERROR,\t");
        break;
    case DHTLIB_ERROR_TIMEOUT:
        Serial.print("DHT11 ERROR: DISCONNECTED/TIMEDOUT,\t");
        break;
    default:
        Serial.print("DHT11 ERROR: UNKNOWN,\t");
        break;
    }
    // record temp and humidity from dht11
    humidity = DHT.humidity;
    temp = DHT.temperature;

    //check vlaues to set flags other than low water which is set in the moisture function

    // convet numbers to string format for easy wifi transmitting 
    Light_S = "Light Level: " + String(light) + " lumens";
    SoilTemp_S = "Soil Temp: " + String(S_celsius) + " degrees";
    MoistVal_S = "Soil moisture: " + String(moistvalue);
    AirHumidity_S = "Humidity: " + String(humidity) + " %";
    AirTemp_S = "Air Temp: " + String(temp)+ " degrees";

    // print recorded values to the com port for debugging

    Serial.println(Light_S);
    Serial.println(SoilTemp_S);
    Serial.println(MoistVal_S);
    Serial.println(AirHumidity_S);
    Serial.println(AirTemp_S);
    Serial.println("");


    // reset counter
    HourTimer = 0 ;
    CheckSoilTempDevice();//rechect the soiltemp sensor
    SoilTempDevice.depower(); //power down the device to save power
  }
  //Serial.println("Looped"); // debugging

}
int sensorRawToPhys(int raw){
  // Conversion rule
  float Vout = float(raw) * (5 / float(1023));// Conversion analog to voltage assuming 5V
  float RLDR = (4700 * (5 - Vout))/Vout; // Conversion voltage to resistance assuming 4.7K Ohm Resistor
  int phys=500/(RLDR/1000); // Conversion resitance to lumen
  return phys;
}
