/****************************************
* Filename : GWS_Monitor_03
* This is software created after shipping the first prototype.
* Created to communicate with Ubidots after paying the subscription.
* Changes: -
* 1. Comunicates with GWSMonitor002 on Ubidots
* 
* 
 * Author: George Raouf
 *
 * Created on 1/8/2021
 * modified D.Bennett on 10th Jan 21 and 14th Feb
 * 
 *  TO DO
 *  1. Get Credentials from SD card - DONE
 *  2. Get Device name from SD card
 *  2. Need to be able to set different sampling intervals for each sensor - NEXT VERSION
 *  3. Need to have variable names that match those displayed on Ubidots
 *  4. Humidity needs to be dispayed with %
 *  5. Temperature needs to be displayed with C
 *  6. Need to scale pressure sensor - DONE
 *  7. Fix cause of exception. FIXED - string used in MQTT cloud write was too long
 *  8. Need to check sensor readings are valid - NEXT VERSION
 * 
 * System Sensors: -
 *  Internal Temperature - derived from Si7021
 *  Internal Humidity - derived from same Si7021
 *  Electronics Temerature - derived from DHT11
 *  Electronics Humidity - derived from DHT11
 *  External Probe Temperaature - derived from DS18B20
 *  Air Pressure - analog voltage  *** need to agree calibration
 *  Accelermoeter - derived from MPU-6050
 * 
 *Example code for :
       > Connecting Temperature and Humidity sensor to Microcontroller ( ESP )
       > Connecting and Installing the Ultrasonic sensor
       > Creating a Wifi connection
       > Pushing the sensors data to the cloud Ubidots
       > Connecting the accelemometer 
       > Adding the Pressure sensor to the Pin GIOP 34 - ADC0 
       > This version having data for the DHT ( Temperature and Humidity ) 
       > Before Uploading please change the Username and Password of the wifi within this file
       > One wire and DallasTemperature libraries need to be installed
       > Conncect the Dallas Temperature sensor to it's pin
       > This version is having a configuration file that change in the sampling rate as well as controlling the sensors
       > Connect a Led to Gpio 32
       > Si7021 is connected ( need to push data ) 
      
 *
 * For more information refer to www.BostinTechnology.com
 *
 ****************************************/
/****************************************
 * Include Libraries
 ****************************************/
#include <WiFi.h>         //this is the WIFI library for setting up the wifi connection on esp32
///////////////////////////////
// Connecting to Ethernet 
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

#include <ETH.h>
///////////////////////////// Multible Threading /////////////////
#include "Thread.h"
//int ledPin = 2;

//My simple Thread
Thread myThread1 = Thread();
Thread myThread2 = Thread();
/////////////////////////////////////////////
#include <PubSubClient.h>
#include <DHT.h>
#include "Configuration.h"
//#include "credentials.h"

//#define DEVICE_LABEL "GWSMonitoring002" // Assign the device label
// Device ID will be assigned from the SD Card
//#define DEVICE_LABEL "" // Assign the device label

#include <Wire.h> // This library allows you to communicate with I2C devices.

//////////////// Multiple Wifi /////////////////
//#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

////////////////////////////////////////////

// SD Card libraries
#include "FS.h"
#include "SD_MMC.h"
//#include <SPI.h>
//#include <SD.h>
byte i;   // counter used in file reading
byte x;   // counter used in file reading
byte z;   // counter used in file reading


/****************************************
 * Hardware Initialisation
 ****************************************/
  // If we aren't using the SD card for passing the wifi credentials we use the credentials below
    

////////////////////////// Wifi and SD card setup ////////////////
char WiFiSSID = "DBHome";      // hold wifi ssid read from SD card
char WiFiPASSWORD ="DB16091963";  // holds wifi password read from SSID card
char DEVICE_LABEL = ""; // holder for the device Id


////////////////////////// MQTT setup ////////////////
#define TOKEN "BBFF-l8gRwzQpEikMaXp3uAahxYodCzOX45" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "GRDuncanIoT32DhTTankMaximus" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string;

// This to create a connection with the MQTT broker and dashboard of the Ubidots 
char mqttBroker[]  = "things.ubidots.com";
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

// CSV Structure and variables to be stored in the SD Card
//char charRead;
char dataStr[100] = "";
char HeadStr[100] = "Time ,DSTemp , SiTemp , SiHum , DHT Temp , DHT Humidity , Pressure"; // for the CSV for data logger

char statusRec[100] = "";
char statusStr[100] = "Time , Sensors , SD card  , DataTocloud , increment, Reconnects";   // for the CSV for Debug messages

char buffer[7];
///////////////////////////

const uint8_t NUMBER_OF_VARIABLES = 20; // Number of variable to subscribe to
char * variable_labels[NUMBER_OF_VARIABLES] = {"relay_big", "relay_small", "relay_fan"}; // labels of the variable to subscribe to


//float value; // To store incoming value.
uint8_t variable; // To keep track of the state machine and be able to use the switch case.
const int ERROR_VALUE = 65535; // Set here an error value

 // create a wifi client for ubidots
WiFiClient ubidots;
PubSubClient client(ubidots);

/*
/////////////////////// Wifi Manager CallBack return credentials /////////
//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  //ticker.attach(0.2, tick);
}
////////////////////////////////////////////////////////////////////
*/
// This function to define the variables for the cloud itself
void get_variable_label_topic(char * topic, char * variable_label) {
  Serial.print("topic:"); // print the topic we push the data to
  Serial.println(topic);
  sprintf(variable_label, "");
  for (int i = 0; i < NUMBER_OF_VARIABLES; i++) {  // for loop is created to define and initialize the variabes label for ubidots
    char * result_lv = strstr(topic, variable_labels[i]);
    if (result_lv != NULL) {
      uint8_t len = strlen(result_lv);
      char result[100];
      uint8_t i = 0;
      for (i = 0; i < len - 3; i++) {
        result[i] = result_lv[i];
      }
      result[i] = '\0';
      Serial.print("Label is: ");
      Serial.println(result); 
      sprintf(variable_label, "%s", result); // send the results as data to ubidots
      break;
    }
  }
}
// create a float variable for the payload using a for loop
float btof(byte * payload, unsigned int length) {
  char * demo_ = (char *) malloc(sizeof(char) * 10);
  for (int i = 0; i < length; i++) {
    demo_[i] = payload[i];
  }
  return atof(demo_);
}

// Error handling and defining of the variables to the system of ubidots
void set_state(char* variable_label) {
  variable = 0;
  for (uint8_t i = 0; i < NUMBER_OF_VARIABLES; i++) {
    if (strcmp(variable_label, variable_labels[i]) == 0) {
      break;
    }
    variable++;
  }
  if (variable >= NUMBER_OF_VARIABLES) variable = ERROR_VALUE; // Not valid
}

// this function  for subscribe for specific topics from Ubidots
//We now need to create the callback function to process the received message.
void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
  Serial.println("....... MQTT MSG Topic...");
}

// the function for setting up the conniction with UBIDTOS 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) { // trying to connect to the MQTT Broker
      Serial.println("Connected");
    } else { // else it will try again to connect every two seconds
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

/////////////////////// Dallas Temperature Sensor Initialisation  ////////
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


/////////////////////////MPU-6050 Accelermoeter and gyro //////////////////////
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data
float sensor2;
float sensor3;

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}


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



/****************************************
 * Interrupts
 ****************************************/

/****************************************
 * Functions
 ****************************************/

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

void DataToCSV() {
   dataStr[0] = 0; //clean out string
 //statusRec[0] =0; 
  //----------------------- using c-type ---------------------------
 //convert floats to string and assemble c-type char string for writing:
 ltoa( millis(),buffer,10); //convert long to charStr
 strcat(dataStr, buffer); //add it to the end
 strcat( dataStr, ", "); //append the delimiter
 
 //dtostrf(floatVal, minimum width, precision, character array);
 dtostrf(temperatureC, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( dataStr, buffer); //append the converted float
 strcat( dataStr, ", "); //append the delimiter

 dtostrf(SiTemp, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( dataStr, buffer); //append the converted float
 strcat( dataStr, ", "); //append the delimiter

 dtostrf(SiHum, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( dataStr, buffer); //append the converted float
 strcat( dataStr, ", "); //terminate correctly 

 //dtostrf(floatVal, minimum width, precision, character array);
 dtostrf(Temperature, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( dataStr, buffer); //append the converted float
 strcat( dataStr, ", "); //append the delimiter

 dtostrf(Humidity, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( dataStr, buffer); //append the converted float
 strcat( dataStr, ", "); //append the delimiter

 dtostrf(pressureV, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( dataStr, buffer); //append the converted float
 strcat( dataStr, 0); //terminate correctly 

  Serial.println(HeadStr);
 Serial.println(dataStr);

    // SD card send data 

   appendFile(SD_MMC, "/datalogger.csv", dataStr);
  Serial.print("SD_MMC Card DataLogger.csv : ");
  Serial.print(dataStr);
  Serial.print("  Done  ");
 
 msgs_card ++ ;

}

void DebugToCSV() {
   //dataStr[0] = 0; //clean out string
 statusRec[0] =0; 
  //convert floats to string and assemble c-type char string for writing:
 ltoa( millis(),buffer,10); //convert long to charStr
 strcat(statusRec, buffer); //add it to the end
 strcat( statusRec, ", "); //append the delimiter
 
 //dtostrf(floatVal, minimum width, precision, character array);
 dtostrf(msgs_sensors, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( statusRec, buffer); //append the converted float
 strcat( statusRec, ", "); //append the delimiter


 dtostrf(msgs_card, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( statusRec, buffer); //append the converted float
 strcat( statusRec, ", "); //append the delimiter

 dtostrf(msgs_cloud, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( statusRec, buffer); //append the converted float
 strcat( statusRec, ", "); //terminate correctly 



 //dtostrf(floatVal, minimum width, precision, character array);
 dtostrf(Reconnects, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
 strcat( statusRec, buffer); //append the converted float
 strcat( statusRec, 0); //append the delimiter

Serial.println(statusStr);
Serial.println(statusRec);

    // SD card send data 

   appendFile(SD_MMC, "/debuglogger.csv", statusRec);
  Serial.print("SD_MMC Card debugLogger.csv : ");
  Serial.print(statusRec);
  Serial.print("  Done  ");
 
 

}

void DataToCloud() {
  // REQUIRES sensor1, sensor2, sensor3, pressureV, accelermoeter_x, accelermoeter_y, accelerometer_z, Temperature, Humidity, temperatureC, SiHum, SiTemp.  RETURNS: Nothing
  
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
  sprintf(payload, "{\"");
  sprintf(payload, "%s%s\":%s", payload, "Ultrasonic", str_sensor1);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_Y", str_sensor2);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_X", str_sensor3);
  sprintf(payload, "%s,\"%s\":%s", payload, "Pressure", str_sensor4);
  sprintf(payload, "%s,\"%s\":%s", payload, "acc_X", str_accx);
  sprintf(payload, "%s,\"%s\":%s", payload, "acc_Y", str_accy);
  sprintf(payload, "%s,\"%s\":%s", payload, "acc_Z", str_accz);
  sprintf(payload, "%s,\"%s\":%s", payload, "Temperature", str_temp);
  sprintf(payload, "%s,\"%s\":%s", payload, "Humidity", str_humid);
  sprintf(payload, "%s,\"%s\":%s", payload, "temperatureC", str_tempC);
  sprintf(payload, "%s,\"%s\":%s", payload, "SiTemp", str_humidSi);
  sprintf(payload, "%s,\"%s\":%s", payload, "SiHum", str_tempSi);
// debug variables to be assigned

  sprintf(payload, "%s,\"%s\":%s", payload, "Sensors_Status", str_sensors);
  sprintf(payload, "%s,\"%s\":%s", payload, "Msgs_to_cloud", str_dtc);
  sprintf(payload, "%s,\"%s\":%s", payload, "Msgs_to_card", str_card);
  sprintf(payload, "%s,\"%s\":%s", payload, "Reconnects", str_connects);
  sprintf(payload, "%s,\"%s\":%s", payload, "cardFull", str_full);
  sprintf(payload, "%s}", payload);
  Serial.println(payload);
  Serial.println("Pushing data to the cloud");

  ///////////////////////////////// Respond back from the server ///////////
  boolean rc = client.publish(topic, payload); // publish the payload to the MQTT broker
      if (rc) 
    {
      Serial.println("");
     Serial.println("Msg Sent to cloud");
      msgs_cloud ++ ;
     Serial.println("total Msgs Sent to cloud");
     Serial.println(msgs_cloud);


    }
    else  {
       Serial.println("");
     Serial.println("Msg Failed to Sent to cloud");
    }
  //boolean rc = mqttClient.publish("myTopic", "myMessage");
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
    
     //   Serial.write(file.read());
      //  pass =String( Serial.write(file.read()));
    //}
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
    
     //   Serial.write(file.read());
      //  pass =String( Serial.write(file.read()));
    //}
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
  
    // delay
    delay(MPUSensorSamp);   // This delay for sampling intervals from configuration.h
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
    delay(DSTempSamp);       // This delay for sampling intervals from configuration.h
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
  delay(1000);   
 }

/****************************************
 * Setup
 ****************************************/
void setup() {
  ////////////////////////  Using the Ethernet & Wifi Integration //////////
      // delete old config
    WiFi.disconnect(true);

  // If we aren't using the SD card for passing the wifi credentials we use the credentials below
    

////////////////////////// Wifi and SD card setup ////////////////
     //WiFiSSID[] = "DBHome";      // hold wifi ssid read from SD card
      //WiFiPASSWORD[] = "DB16091963";  // holds wifi password read from SSID card
      delay(1000);

    // Examples of different ways to register wifi events
    WiFi.onEvent(WiFiEvent);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
    WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
        Serial.print("WiFi lost connection. Reason: ");
        Serial.println(info.disconnected.reason);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    // Remove WiFi event
    Serial.print("WiFi Event ID: ");
    Serial.println(eventID);
    // WiFi.removeEvent(eventID);

     WiFi.begin(WiFiSSID, WiFiPASSWORD); // using your wifi username and password to connect to wifi


    Serial.println();
    Serial.println();
    Serial.println("Wait for WiFi... ");
    
///////////////////////////////////////////// Multiple Threading ////////

  myThread1.onRun(DataToCSV);
  myThread1.setInterval(500);

    myThread2.onRun(DataToCloud);
  myThread2.setInterval(1000);

  /////////////////////////////////////////////////////////////


  /*
  noInterrupts();   // disable all interrupts
  ////////////////////////////////////////////// Wifi Manager for the AP/////
   WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  Serial.begin(115200); // initializing a serial monitor 

     //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }
  
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  
*/

  
/////////////////////////////////////// SD card ////////////////
  if(!SD_MMC.begin()){
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD_MMC card attached");
      return;
  }

  Serial.print("SD_MMC Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  // If we want to read the credentials from the Sd card uncomment the below section
  /*
  readFile(SD_MMC, "/ssid.txt");
  Serial.print("SD_MMC Card ssid.txt : ");
  Serial.print(WiFiSSID);
  Serial.print("  Done  ");

  readFil(SD_MMC, "/pass.txt");
  Serial.print("SD_MMC Card pass.txt : ");
  Serial.print(WiFiPASSWORD);
  Serial.print("  Done  ");

  readID(SD_MMC, "/id.txt");
  Serial.print("SD_MMC Card id.txt : ");
  Serial.print(DEVICE_LABEL);
  Serial.print("  Done  ");
*/
  // Check the capacity of the Card and if there is space we can write to it
      Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
    float totalStorage = SD_MMC.totalBytes() / (1024 * 1024) ;
    float usedStorage = SD_MMC.usedBytes() / (1024 * 1024) ;

  if (totalStorage > 1.1*usedStorage) {
     writeFile(SD_MMC, "/datalogger.csv", HeadStr);
  Serial.print("SD_MMC Card DataLogger.csv : ");
  Serial.print(HeadStr);
  Serial.print("  Done  ");

  writeFile(SD_MMC, "/debuglogger.csv", statusStr);
  Serial.print("SD_MMC Card debugLogger.csv : ");
  Serial.print(statusStr);
  Serial.print("  Done  ");

  }
  else {
    Serial.print("Warning :You should Change the SD Card as it is FULL ");
    cardFull = 1 ;
  }
 


  // initializing the I/O
  pinMode(trigPin, OUTPUT);       // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);        // Sets the echoPin as an Input
  pinMode(pressPin, INPUT);
  pinMode(DHTPin, INPUT);         // defining the DHT temperature sensor as input
  pinMode (ledPin, OUTPUT);       // setup Led pin as a digital output pin

  /////////////////    DHT11 temp and humidity initilisation  ////////////////////////////////////// 
  dht.begin();  //Starting DHTT
  //////////////////    DS18B20 temp probe initialisation  //////////////////////////////////////
  sensors.begin(); // initializing the DS18B20  sensor

  //////////////////    Temp Humidity Si7021 sensor Initilisation  ////////////////////////
  uint64_t serialNumber = 0ULL;
  
  si7021.begin();
  serialNumber = si7021.getSerialNumber();

  //arduino lib doesn't natively support printing 64bit numbers on the serial port
  //so it's done in 2 times
  Serial.print("Si7021 serial number: ");
  Serial.print((uint32_t)(serialNumber >> 32), HEX);
  Serial.println((uint32_t)(serialNumber), HEX);

  //Firware version
  Serial.print("Si7021 firmware version: ");
  Serial.println(si7021.getFirmwareVersion(), HEX);

  //////////////////    Accelermoter Initilisation  //////////////////////////////////////
  Wire.begin(); // MPU to begin ( Acceleromemter ) 
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B);                 // PWR_MGMT_1 register
  Wire.write(0);                    // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  ///////////////////////////////////OLED_Starting ///////////////////////
  // Write Splash Screen
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "GWS Monitoring");
  display.setTextAlignment(TEXT_ALIGN_LEFT); 
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 20, "V2.0 ");
  display.setTextAlignment(TEXT_ALIGN_LEFT); 
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 30, "Connecting to wifi ");
  display.drawString(0, 40,String(WiFi.status());
  display.display();
  delay(1000);

  ////////////////////  Initialise WiFi////////////////////////////////////////////
/*
    wifiMulti.addAP("DBHome", "DB16091963");  // the default credentials of your Wifi
    wifiMulti.addAP(WiFiSSID, WiFiPASSWORD);  // Client wifi that assigned from the sd card
   

    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }

*/
//////////////////
    /*
  WiFi.begin(WiFiSSID, WiFiPASSWORD); // using your wifi username and password to connect to wifi

  // Connect to WifI
  Serial.println();
  Serial.println("Wait for WiFi... ");  
  Serial.println("With SSID ");
  Serial.println("DBHome");
  while (WiFi.status() != WL_CONNECTED) { // to make sure we are connected to wifi using a while loop
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("");
  Serial.print("WiFi Connected ");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 */
  
  client.setServer(mqttBroker, 1883); // create a mqtt broker server
  client.setCallback(callback);  // create a callback from the ubidots
}

/****************************************
 * Main loop
 ****************************************/
void loop() {
  ReadAllSensors();           // read all sensors
  
    // checks if thread should run ( Sending sensor data to the SD card ) 
  if(myThread1.shouldRun())
    myThread1.run();
    
  //DataToCSV()                 // Send sensors data to CSV 
  Count_Samples++;            // increment sample counter
  DisplaySensorsToSerial();   // Display sensor values
  Dispay_Sensor_Values();     // clear display and writes the sensor values. REQUIRES SiTemp, SiHum, Temperature, Humidity, pressureV.  RETURNS: Nothing
  delay(2000);                // wait for display
  Dispay_Erros();             // clear display and writes the sensor values. REQUIRES Reconnects, Count_Samples  RETURNS: Nothing
  delay(2000);                // wait for display
/*
    if(wifiMulti.run() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        delay(1000);
    }

    */

  if (!client.connected()) {    //  check for WiFi connection
    reconnect();
    Reconnects++;       // increment reconnect counter
    }
  
  //--------------Uploading Values to cloud-------------

// checks if thread should run ( Uploading sensor data to the cloud ) 
    if(myThread2.shouldRun())
    myThread2.run();
    
  //DataToCloud();       // REQUIRES sensor1, sensor2, sensor3, pressureV, accelermoeter_x, accelermoeter_y, accelerometer_z, Temperature, Humidity, temperatureC, SiHum, SiTemp.  RETURNS: Nothing
  Display_Cloud_Sent();     // REQUIRES WiFiSSID  RETURNS: Nothing  
  
  DebugToCSV();


  delay(1000);        // wait for display
  
  client.loop();      // WHAT DOES THIS STATEMENT DO?
}
