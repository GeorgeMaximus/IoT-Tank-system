/*
  Credential.ino, AutoConnect for ESP8266.
  https://github.com/Hieromon/AutoConnect
  Copyright 2018, Hieromon Ikasamo.
  Licensed under The MIT License.
  https://opensource.org/licenses/MIT

  An example sketch for an Arduino library for ESP8266 WLAN configuration
  via the Web interface. This sketch provides a conservation measures
  utility for saved credentials in EEPROM.
  By accessing the root path, you can see the list of currently saved
  credentials via the browser. Enter an entry number of the credential,
  that entry will be deleted from EEPROM.
  This sketch uses PageBuilder to support handling of operation pages.
*/


/*
 * This version working on my simulate devices and have the following fuctions & fully tested:
 * Access point
 * Wifi connectivity
 * connecting multiple variables 
 * Ubidots integration
 * csv simulations
 * Multitasking
 */

/////////////////////// Mr. Duncacn Device Libraries /////////////////////////////////////////
///////////////////////////////
// Connecting to Ethernet 
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

#include <ETH.h>

// SD Card libraries
#include "FS.h"
#include "SD_MMC.h"
byte i;   // counter used in file reading
byte x;   // counter used in file reading
byte z;   // counter used in file reading

#include <DHT.h>
#include "Configuration.h"    // this file is intended to switch differnt options for the software. Not well used at the moment.

#include <Wire.h> // This library allows you to communicate with I2C devices.
/////////////////////// Dallas Temperature Sensor  ////////
#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 22;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Dallas temperature variables 
float temperatureC =0.0 ;
float temperatureF = 0.0 ;


/////////////////////////// Si7021 Temperature sensor //////////
#include "Si7021.h"

Si7021 si7021;

// Si7021 temperature variables 
float SiTemp =0.0 ;
float SiHum = 0.0 ;


/////////////////////////MPU-6050 Accelermoeter and gyro ////
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data
float sensor2;
float sensor3;
///////////////////////////////////////////////////////
///////////////////////////// OLED Variables and iniializing //////////////////////////////
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// Initialize the OLED display using Wire library for ESP-EVB SDA > 13 , SCL > 16

SSD1306  display(0x3c, 13, 16);


/////// DHT11 temp and humidity sensor setup ////////
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
uint8_t DHTPin = 19; // Set the GPIO19 as DHT11 sonic SENSOR

// defining the pin of the DHT sensor and it's variables
DHT dht(DHTPin, DHTTYPE);                
float Temperature;
float Humidity;


/////// Ultrasonic Setup //////////////////
const int trigPin = 2; // Set the GPIO2 as Ultra sonic SENSOR
const int echoPin = 4;

long duration;    // This are variables for the Ultrasonic readings
int distance;
float sensor1 ;     //


/////// Pressure Sensor value
float pressureV;    // value from ADC


/////// LED Setup //////////////////
const int ledPin = 32; // Set the GPIO 32 as Led 


/////// ADC for presssure sensor Setup //////////////////
const int pressPin = 34; // pressure sensor pin GPIO 34 >> adc0

// Ethernet and WIFI Integration functions :
void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case SYSTEM_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi access point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            Serial.println("IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
        default: break;
    }}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}
/////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// SD Card Functions ////////////////////


void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }
    Serial.print("Read from file: ");
    while(file.available()) WiFiSSID[i++] = file.read();
    WiFiSSID[i]= '\0';
}

void readFil(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File fil = fs.open(path);
    if(!fil){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(fil.available()) WiFiPASSWORD[x++] = fil.read();
    WiFiPASSWORD[x]= '\0';
    
     //   Serial.write(file.read());
      //  pass =String( Serial.write(file.read()));
    //}
}

void readID(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File ID = fs.open(path);
    if(!ID){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(ID.available()) DEVICE_LABEL[z++] = ID.read();
    DEVICE_LABEL[z]= '\0';
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
}


void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
        msgs_card ++ ;
        Serial.println("total Messages appended");
        Serial.println(msgs_card);
    } else {
        Serial.println("Append failed");
    }
}
/////////////////////////////////////////////////////////////////////////////////

/****************************************
 * Older Functions that have been working well.
 ****************************************/
void ReadAllSensors(){    // Read all the sensors 
  mpu_read();     // calling the function of the MPU ( acceleromemeter ) 
  DallasTemp();   // calling the function of the Dallas Temperature sensor
  siTemp();     // calling the function of the Si7021 Temperature sensor
  
  //--------------Pressure Sensor------------------
  int z;              // for next loop
  int ADCreading;   // totals the ADC reading
  pressureV = 0;     // start with zero
  for (z=1; z<11; z++) {   // read pressure 10 times to get an average
/*    ADCreading =analogRead(pressPin);
    Serial.print("Pressire bit-- ");
    Serial.print(ADCreading);
    Serial.print("\t");
    Serial.println(i); */
    if (isnan(analogRead(pressPin))) 
    {
      ADCreading = 0;    // if ADC always reads zero then pressureV will be 0
    }
    else  {
      pressureV = pressureV + analogRead(pressPin);    // only do maths if pressure reading above 200 (guessed value)
    }
  }
  //Serial.print("Pressure total-- ");
  //Serial.println(pressureV);
  pressureV = pressureV/10;     // find average
  //Serial.print("Pressure avg-- ");
  //Serial.println(pressureV);
    // pressure sensor is 5V but ESP32 board only supplying 4V.
    // output should be 0.5V to 4.5V, now 0.4V to 3.6V. ADC can probably only read to 3.3V.
    // its a 12bar pressure sensor. 12bar range over 3.2V. 3.75bar/volt
    // 0 bar = 4096 * 0.4/3.3 = 496 bits
    // span = 4096/3.3 = 1241 bits/volt.
    // 1241 bits = 3.75 bar. 331 bits/bar
    // 0 bar = 496 bits
    // 2 bar = 496 + 662 = 1158 bits
    // 4 bar = 496 + 1324 = 1820 bits
    // 6 bar = 496 + 1986 = 2482 bits
    // 8 bar = 496 + 2648  = 3144 bits
    // 10 bar = 496 + 3310 = 3806 bits
    // bar = (ADC reading - 496)/331
  pressureV = pressureV - 320;
  if (pressureV<= 0) {    // prevent readings less than zero
    pressureV = 0;
  }
  pressureV = pressureV/331;  // convert to bar
  

  //------Gyroscope Sensor value--------
  //float sensor1 = (3.3*analogRead(SENSOR0))/4096;
  float sensor2 = gyro_x;  // defining the variable for gyroscope data in x-axis
  float sensor3 = gyro_y;  // defining the variable for gyroscope data in y-axis
  
  //--------------DHT Sensor------------------
  if (isnan(dht.readTemperature()) || isnan(dht.readHumidity())) 
  {
    Temperature = 0.0;
    Humidity = 0.0;
  }
  else 
  {
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity  
  }

  int lowerBoundd = 0 ;
  int upperBoundd = 100 ;

  if (Temperature > lowerBoundd && Temperature < upperBoundd) {
      Serial.print("DHT Temp are in range ");
  }
  else {
      Temperature = 999.9;
  }

  if (Humidity > lowerBoundd && Humidity < upperBoundd) {
      Serial.print("DHT Humidity are in range ");    
  }
  else {
      Humidity = 999.9;
  }
  
 //--------------Ultrasonic Sensor------------------
   // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
   // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  distance= duration*0.034/2;       // Calculating the distance
  sensor1 = distance;
  msgs_sensors ++ ;
}
 
void mpu_read(){ // this is the main function for reading and calibrating the data of the Acceleromemter sensor
  if (MPUSensor == true) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
    Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
    Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
    // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
    accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
    accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
    accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
    temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
    gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
    gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
    gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
 }
}

void DallasTemp(){
  if (DSTemp == true) {
    int lowerBound = 0 ;
    int upperBound = 100 ;
    sensors.requestTemperatures(); 
    temperatureC = sensors.getTempCByIndex(0);
    temperatureF = sensors.getTempFByIndex(0);
    if (temperatureC > lowerBound && temperatureC < upperBound) {
      Serial.print("Probe Temp are in range ");    
    }
    else {
      temperatureC = 999;
    }
  }
}

 void siTemp(){  /// For printing the sensor Si7021 data to serial
  int lowerBound = 0 ;
  int upperBound = 100 ;
  SiHum = si7021.measureHumidity();
  SiTemp = si7021.getTemperatureFromPreviousHumidityMeasurement();
  if (SiTemp > lowerBound && SiTemp < upperBound) {
    Serial.print("Si7021 Temp are in range ");
      
  }
  else {
    SiTemp = 999.9;
  }
  if (SiHum > lowerBound && SiHum < upperBound) {
    Serial.print("Si7021 Humidity are in range ");  
  }
  else {
    SiHum = 999.9;
  } 
}


void   Display_Cloud_Sent(){
  //REQUIRES WiFiSSID  RETURNS: Nothing

  display.clear();                     // clear the display
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Data Written");
  display.drawString(30, 20, "   to Cloud ");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 45, "Connected to ");
  display.drawString(80, 45, "DBHome" );
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
//  display.drawString(60, 0, String(millis()));
  display.display();      // write the buffer to the display
}
  
void Dispay_Sensor_Values(){      // clear display and writes the sensor values
  // REQUIRES SiTemp, SiHum, Temperature, Humidity, pressureV.  RETURNS: Nothing

  display.clear();      // clear the display
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Int Air Temp: ");
  display.drawString(100, 0, String(SiTemp) );
  display.drawString(0, 10, "Int Air Hum: ");
  display.drawString(100, 10, String(SiHum) );
  display.drawString(0, 20, "Elec Temp: ");
  display.drawString(100, 20, String(Temperature ));
  display.drawString(0, 30, "Elec Hum: ");
  display.drawString(100, 30, String(Humidity) );
  display.drawString(0, 40, "Pressure");
  display.drawString(100, 40, String(pressureV) );
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.display(); 
}

void Dispay_Erros(){      // clear display and writes the sensor values
  // REQUIRES Reconnects, Count_Samples.  RETURNS: Nothing

  display.clear();      // clear the display
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Operation Log");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 20, "Reconnects: ");
  display.drawString(100, 20, String(Reconnects) );
  display.drawString(0, 30, "Sample Count: ");
  display.drawString(100, 30, String(Count_Samples) );
/*  display.drawString(0, 20, "Elec Temp: ");
  display.drawString(100, 20, String(Temperature ));
  display.drawString(0, 30, "Elec Hum: ");
  display.drawString(100, 30, String(Humidity) );
  display.drawString(0, 40, "Pressure");
  display.drawString(100, 40, String(pressureV) );
  display.setTextAlignment(TEXT_ALIGN_RIGHT);*/
  display.display(); 
}




void DisplaySensorsToSerial(){    // Display sensor values on serial port
  Serial.print(temperatureC);
  Serial.print("ºC . -- ");
  Serial.print(temperatureF);
  Serial.print("ºF . -- ");
  Serial.print(pressureV);
  Serial.println("bar. ");

  Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);      // accelerometer sensor
  Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
  Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
  Serial.println(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));

  Serial.print("Internal Hum: ");
  Serial.print(SiHum);
  Serial.print("% - Internal Temp: ");
  Serial.print(SiTemp);
  Serial.println("C");
}

/////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// Access Point SSID = espAp
// Access point Pass = 12345678

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <AutoConnect.h>
#include <AutoConnectCredential.h>
#include <PageBuilder.h>
#include <PubSubClient.h>

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer Server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer Server;
#endif

AutoConnect      Portal(Server);
String viewCredential(PageArgument&);
String delCredential(PageArgument&);

/////////////////////////////////////////////// MQTT Variable for Ubidots 
//#define WIFISSID "GeorgeMaximus" // Put your WifiSSID here
//#define PASSWORD "maxi1234" // Put your wifi password here
#define TOKEN "BBFF-l8gRwzQpEikMaXp3uAahxYodCzOX45" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "GRDuncanIoT32Maxi66" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string; 
                                           //it should be a random and unique ascii string and different from all other devices

/****************************************
 * Define Constants
 ****************************************/
#define VARIABLE_LABEL "sensor2" // Assing the variable label
#define DEVICE_LABEL "esp64" // Assig the device label

#define SENSOR 12 // Set the GPIO12 as SENSOR

char mqttBroker[]  = "industrial.api.ubidots.com";
char payload[300];
char topic[150];
// Space to store values to send
char str_sensor1[10];
char str_sensor2[10];
char str_sensor3[10];
char str_sensor4[10];
char str_accx[10];
char str_accy[10];
char str_accz[10];
char str_temp[10];
char str_humid[10];
char str_tempC[10];
char str_humidSi[10];
char str_tempSi[10];
// debug variables 
char str_dtc[10];
char str_sensors[10];
char str_card[10];
char str_connects[10];
char str_full[10];
// debug increments 
int msgs_cloud = 1; // number of msgs pushed to the cloud
int msgs_card = 0 ; // number of msgs pushed to the card
int msgs_sensors = 0; // counter of successful of data
int cardFull = 0; // Initialize the Card as if it is free or full


/****************************************
 * Error Counters
 ****************************************/
int Reconnects = 0;     // counts number of WiFi reconnects
int Count_Samples = 0;  // counts number of samples read from sensors


////////////////////////// Variables used to write CSV file to Sd Card ////////////////
String dataStr = "";
//char HeadStr[100] = "Time ,DSTemp , SiTemp , SiHum "; // for the CSV for data logger
char HeadStr[100] = "Time ,DSTemp , SiTemp , SiHum , DHT Temp , DHT Humidity , Pressure"; // for the CSV for data logger

String statusRec = "";
char statusStr[100] = "Sensors , SD card  , DataTocloud , Reconnects";   // for the CSV for Debug messages


////////////////////////////////////////////////////////////////////////


int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data
float sensor2;
float sensor3;
float Temperature;
float Humidity;
//long duration;    // This are variables for the Ultrasonic readings
int distance;
float sensor1 ;     //
float temperatureC;
float SiTemp;
float SiHum;
/////// Pressure Sensor value
float pressureV;    // value from ADC

/////////////////////////////////////////////////////////
/****************************************
 * Auxiliar Functions
 ****************************************/
WiFiClient ubidots;
PubSubClient client(ubidots);

void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      Reconnects ++;
      Serial.println("Reconnects = ");
      Serial.println(Reconnects);
      // Wait 2 seconds before retrying
      if (Reconnects > 3){
        Reconnects = 0;
        ESP.restart();
      }
      delay(2500);
    }
  }
}

/////////////////////////////////////////////////////

// Specified the offset if the user data exists.
// The following two lines define the boundalyOffset value to be supplied to
// AutoConnectConfig respectively. It may be necessary to adjust the value
// accordingly to the actual situation.

#define CREDENTIAL_OFFSET 0
//#define CREDENTIAL_OFFSET 64

/**
 *  An HTML for the operation page.
 *  In PageBuilder, the token {{SSID}} contained in an HTML template below is
 *  replaced by the actual SSID due to the action of the token handler's
 * 'viewCredential' function.
 *  The number of the entry to be deleted is passed to the function in the
 *  POST method.
 */
static const char PROGMEM html[] = R"*lit(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
  font-family:Helvetica,Arial,sans-serif;
  -ms-text-size-adjust:100%;
  -webkit-text-size-adjust:100%;
  }
  .menu > a:link {
    position: absolute;
    display: inline-block;
    right: 12px;
    padding: 0 6px;
    text-decoration: none;
  }
  </style>
</head>
<body>
<div class="menu">{{AUTOCONNECT_MENU}}</div>
<form action="/del" method="POST">
  <ol>
  {{SSID}}
  </ol>
  <p>Enter deleting entry:</p>
  <input type="number" min="1" name="num">
  <input type="submit">
</form>
</body>
</html>
)*lit";

static const char PROGMEM autoconnectMenu[] = { AUTOCONNECT_LINK(BAR_24) };

// URL path as '/'
PageElement elmList(html,
  {{ "SSID", viewCredential },
   { "AUTOCONNECT_MENU", [](PageArgument& args) {
                            return String(FPSTR(autoconnectMenu));} }
  });
PageBuilder rootPage("/", { elmList });

// URL path as '/del'
PageElement elmDel("{{DEL}}", {{ "DEL", delCredential }});
PageBuilder delPage("/del", { elmDel });

// Retrieve the credential entries from EEPROM, Build the SSID line
// with the <li> tag.
String viewCredential(PageArgument& args) {
  AutoConnectCredential  ac(CREDENTIAL_OFFSET);
  station_config_t  entry;
  String content = "";
  uint8_t  count = ac.entries();          // Get number of entries.

  for (int8_t i = 0; i < count; i++) {    // Loads all entries.
    ac.load(i, &entry);
    // Build a SSID line of an HTML.
    content += String("<li>") + String((char *)entry.ssid) + String("</li>");
  }

  // Returns the '<li>SSID</li>' container.
  return content;
}

// Delete a credential entry, the entry to be deleted is passed in the
// request parameter 'num'.
/*
 String delCredential(PageArgument& args) {
  AutoConnectCredential  ac(CREDENTIAL_OFFSET);
  if (args.hasArg("num")) {
    int8_t  e = args.arg("num").toInt();
    Serial.printf("Request deletion #%d\n", e);
    if (e > 0) {
      station_config_t  entry;

      // If the input number is valid, delete that entry.
      int8_t  de = ac.load(e - 1, &entry);  // A base of entry num is 0.
      if (de > 0) {
        Serial.printf("Delete for %s ", (char *)entry.ssid);
        Serial.printf("%s\n", ac.del((char *)entry.ssid) ? "completed" : "failed");

        // Returns the redirect response. The page is reloaded and its contents
        // are updated to the state after deletion. It returns 302 response
        // from inside this token handler.
        Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
        Server.send(302, "text/plain", "");
        Server.client().flush();
        Server.client().stop();

        // Cancel automatic submission by PageBuilder.
        delPage.cancel();
      }
    }
  }
  return "";
}
*/
////////////////////////////// Functions //////////////////////

void DataToCloud() {
  if (!client.connected()) {
    reconnect();
  }

 sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);  // we changed this line with the device ID 
  sprintf(payload, "%s", ""); // Cleans the payload
  /* 4 is mininum width, 2 is precision; float value is copied onto str_sensor*/
  dtostrf(sensor1, 4, 2, str_sensor1);
  dtostrf(sensor2, 4, 2, str_sensor2);
  dtostrf(sensor3, 4, 2, str_sensor3);
  dtostrf(pressureV, 4, 2, str_sensor4);
  dtostrf(accelerometer_x, 4, 2, str_accx);
  dtostrf(accelerometer_y, 4, 2, str_accy);
  dtostrf(accelerometer_z, 4, 2, str_accz);
  dtostrf(Temperature, 4, 2, str_temp);
  dtostrf(Humidity, 4, 2, str_humid);
  dtostrf(temperatureC, 4, 2, str_tempC);
  dtostrf(SiHum, 4, 2, str_humidSi);
  dtostrf(SiTemp, 4, 2, str_tempSi);
// debug variables to be assigned
   dtostrf(msgs_sensors, 4, 2, str_sensors);
 dtostrf(msgs_cloud, 4, 2, str_dtc);
  dtostrf(msgs_card, 4, 2, str_card);
  dtostrf(Reconnects, 4, 2, str_connects);
  dtostrf(cardFull, 4, 2, str_full);
  
  // This section is to send the data measured from the esp32 to the ubidots
  sprintf(payload, "{");
  sprintf(payload, "%s%s\":%s", payload, "Ultrasonic", str_sensor1);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_Y", str_sensor2);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_X", str_sensor3);
  sprintf(payload, "%s,\"%s\":%s", payload, "Pressure", str_sensor4);
    sprintf(payload, "%s}", payload);
  Serial.println(payload);
  Serial.println("Pushing data to the cloud");

  Serial.println("Publishing data to Ubidots Cloud Packet 1");
  client.publish(topic, payload);
  delay(200);


sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"");
  
  //sprintf(payload, "%s,\"%s\":%s", payload, "acc_X", str_accx);
  //sprintf(payload, "%s,\"%s\":%s", payload, "acc_Y", str_accy);
  //sprintf(payload, "%s,\"%s\":%s", payload, "acc_Z", str_accz);
  
  sprintf(payload, "%s%s\":%s", payload, "Temperature", str_temp);
  sprintf(payload, "%s,\"%s\":%s", payload, "Humidity", str_humid);
  sprintf(payload, "%s,\"%s\":%s", payload, "temperatureC", str_tempC);
  sprintf(payload, "%s,\"%s\":%s", payload, "SiTemp", str_humidSi);
  sprintf(payload, "%s,\"%s\":%s", payload, "SiHum", str_tempSi);
      sprintf(payload, "%s}", payload);
  Serial.println(payload);
  Serial.println("Pushing data to the cloud");

  Serial.println("Publishing data to Ubidots Cloud Packet 2");
  client.publish(topic, payload);
  delay(200);


sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"");

  
// debug variables to be assigned

  sprintf(payload, "%s%s\":%s", payload, "Sensors_Status", str_sensors);
  sprintf(payload, "%s,\"%s\":%s", payload, "Msgs_to_cloud", str_dtc);
  sprintf(payload, "%s,\"%s\":%s", payload, "Msgs_to_card", str_card);
  sprintf(payload, "%s,\"%s\":%s", payload, "Reconnects", str_connects);
  sprintf(payload, "%s,\"%s\":%s", payload, "cardFull", str_full);
  sprintf(payload, "%s}", payload);
  Serial.println(payload);
  Serial.println("Pushing data to the cloud");

  Serial.println("Publishing data to Ubidots Cloud");
  client.publish(topic, payload);
  client.loop();
  //delay(1000);
}

/*
void ReadAllSensors(){    // Read all the sensors 
    float sensor2 = random(100);  // defining the variable for gyroscope data in x-axis
  float sensor3 = random(100);  // defining the variable for gyroscope data in y-axis
  pressureV =  random(100);
  Temperature = random(100);
  Humidity = random(100);
  sensor1 = random(100);
  SiHum = random(100);
  SiTemp = random(100);
  temperatureC = random(100);
  accelerometer_y = random(100);
  accelerometer_z = random(100);
  accelerometer_x= random(100);
  
  cardFull = 0;
   msgs_sensors ++ ;
       Serial.println("Sensors Read Done & No. of function loops = ");
    Serial.println(msgs_sensors);
}
*/
void DataToCSV() {
  //dataStr[0] = 0; //clean out string
  Serial.println("From Data to CSV Function");
   String SSupd = String(millis()) + "," + String(temperatureC) + ","+ String(SiTemp) + ","+ String(SiHum);
   String SSupb = String(Temperature) + "," + String(Humidity) + ","+ String(pressureV);
   
 dataStr = SSupd + "," + SSupb ;
  Serial.println(HeadStr);
  Serial.println(dataStr);
  msgs_card ++ ;
  
}

void DebugToCSV() {
  Serial.println("From Debug to CSV Function");
   
   String SSup = String(msgs_sensors) + "," + String(msgs_card) + ","+ String(msgs_cloud) + ","+ String(Reconnects);

  statusRec = SSup;
  Serial.println(statusStr);
  Serial.println(statusRec);
}

//////////////////////////////// Multi Tasking /////////////////////
TaskHandle_t DataToCloudTask;
TaskHandle_t SensorsDataTask;

void Task1code( void * pvParameters ){
  while (true){
    Serial.println("Data to cloud task started");
      DataToCloud ();

    delay(3000); // task repeat every number of milliseconds

    Serial.println("Data to cloud task ended");
  }
}

void Task2code( void * pvParameters ){
  while (true){
    Serial.println("Hello from Second task");
      ReadAllSensors();
    delay(500);
    DataToCSV();
     delay(500);
    DebugToCSV();
  Serial.println("Sensors data collected");
  
    delay(1000);  // task repeat every number of milliseconds

  }
}

/////////////////////////////////////////////////////////////////////////////////////

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Serial is started " );

// create the thread of task os getting sensor data 
  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
     Task2code,    //Task function.
     "SensorsDataTask", //name of task
     10000, //Stack size of task
     NULL, //parameter of the task
     1, //priority of the task
     &SensorsDataTask, //Task handle to keep track of created task
     0); //pin task to core 0

  rootPage.insert(Server);    // Instead of Server.on("/", ...);
  //delPage.insert(Server);     // Instead of Server.on("/del", ...); ( have deleted this ) 

  // Set an address of the credential area.
  Serial.println("COnfiguration is starting" );
  AutoConnectConfig Config;
  Config.boundaryOffset = CREDENTIAL_OFFSET;
   Serial.println("COnfiguration Portal is on the way" );
  Portal.config(Config);

  // Start
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.SSID());
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

//////////////// MQTT Ubidots Initiaization //////////
    client.setServer(mqttBroker, 1883);
    Serial.println("MQTT Broker connected" );
  client.setCallback(callback);  

    //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
     Task1code,    //Task function.
     "DataToCloudTask", //name of task
     10000, //Stack size of task
     NULL, //parameter of the task
     1, //priority of the task
     &DataToCloudTask, //Task handle to keep track of created task
     0); //pin task to core 0

}

void loop() {
 // DataCollection();
//  DataToCloud();
    Portal.handleClient();
}
