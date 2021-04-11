

/****************************************
 * Include Libraries
 ****************************************/
//#include <WiFi.h>         //this is the WIFI library for setting up the wifi connection on esp32
//#include <PubSubClient.h>
///////////////////////////////

/*
///////////////////////////// Multible Threading /////////////////
#include "Thread.h"
//int ledPin = 2;

//My simple Thread
Thread myThread1 = Thread();
Thread myThread2 = Thread();
/////////////////////////////////////////////
*/


//// Access point creation libraries 


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
////////////////////////////////////////////////////



/****************************************
 * Initialisation
 ****************************************/
  // If we aren't using the SD card for passing the wifi credentials we use the credentials below
////////////////////////// Wifi and SD card setup ////////////////
//char* WiFiSSID = "GeorgeMaximus";      // hold wifi ssid read from SD card
//char* WiFiPASSWORD ="maxi1234";  // holds wifi password read from SSID card
char* DEVICE_LABEL = "GWSMonitoring05"; // holder for the device Id


////////////////////////// MQTT setup ////////////////
#define TOKEN "BBFF-l8gRwzQpEikMaXp3uAahxYodCzOX45" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "GRDuncanIoT32DhTTankMaximus53" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string;

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


////////////////////////// Variables used to write CSV file to Sd Card ////////////////
String dataStr = "";
char HeadStr[100] = "Time ,DSTemp , SiTemp , SiHum "; // for the CSV for data logger
//char HeadStr[100] = "Time ,DSTemp , SiTemp , SiHum , DHT Temp , DHT Humidity , Pressure"; // for the CSV for data logger

String statusRec = "";
char statusStr[100] = "Time , Sensors , SD card  , DataTocloud , increment, Reconnects";   // for the CSV for Debug messages

char buffer[7];


const uint8_t NUMBER_OF_VARIABLES = 20; // Number of variable to subscribe to
char * variable_labels[NUMBER_OF_VARIABLES] = {"relay_big", "relay_small", "relay_fan"}; // labels of the variable to subscribe to


//float value; // To store incoming value.
uint8_t variable; // To keep track of the state machine and be able to use the switch case.
const int ERROR_VALUE = 65535; // Set here an error value

 // create a wifi client for ubidots
WiFiClient ubidots;
PubSubClient client(ubidots);

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
            Reconnects ++;
      Serial.println("Reconnects = ");
      Serial.println(Reconnects);
      // Wait 2 seconds before retrying
      if (Reconnects > 3){
        Reconnects = 0;
        ESP.restart();
      }
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

// defining the variables

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

/****************************************
 * Functions
 ****************************************/

 //////////////////////////////// Multi Tasking /////////////////////
TaskHandle_t DataToCloudTask;
TaskHandle_t SensorsDataTask;

void Task1code( void * pvParameters ){
  while (true){
    
      
      DataToCloud();
      delay(3000); // task repeat every number of milliseconds
  }
}

void Task2code( void * pvParameters ){
  while (true){

    ReadAllSensors();
    delay(500);
    DataToCSV();
     delay(500);
    DebugToCSV();
    
       delay(500);  // task repeat every number of milliseconds
  }
}

void DataToCSV() {
  //dataStr[0] = 0; //clean out string
  Serial.println("From Data to CSV Function");
   String SSupd = String(millis()) + "," + String(temperatureC) + ","+ String(SiTemp) + ","+ String(SiHum);

 dataStr = SSupd ;
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
  /*
  boolean rc = client.publish(topic, payload); // publish the payload to the MQTT broker
  if (rc) {
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

  */
   client.publish(topic, payload);
  client.loop();
      Serial.println("");
    Serial.println("Msg Sent to cloud");
    msgs_cloud ++ ;
    Serial.println("total Msgs Sent to cloud");
    Serial.println(msgs_cloud);
  
    //delay(1000); // task repeat every number of milliseconds

}

 
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
  //
     Portal.handleClient();
    

}
