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
#define MQTT_CLIENT_NAME "GRDuncanIoT32DhTTankMaximusesp32" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string; 
                                           //it should be a random and unique ascii string and different from all other devices

/****************************************
 * Define Constants
 ****************************************/
#define VARIABLE_LABEL "sensor" // Assing the variable label
#define DEVICE_LABEL "esp32" // Assig the device label

#define SENSOR 12 // Set the GPIO12 as SENSOR

char mqttBroker[]  = "industrial.api.ubidots.com";
char payload[100];
char topic[150];
// Space to store values to send
char str_sensor[10];
char str_sensor2[10];
char str_sensor3[10];

float sensor;
float sensor2;
float sensor3;
int Reconnects = 0 ;

////////////////////////////////////////////////////////////////////////
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

//////////////////////////////// Multi Tasking /////////////////////
TaskHandle_t DataToCloudTask;
TaskHandle_t SensorsDataTask;

void Task1code( void * pvParameters ){
  while (true){
    Serial.println("Hello from first task");
      if (!client.connected()) {
    reconnect();
  }

  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  //sprintf(payload, "{\"%s\":", VARIABLE_LABEL); // Adds the variable label
  
  //float sensor = random(500); 
  
  /* 4 is mininum width, 2 is precision; float value is copied onto str_sensor*/
  dtostrf(sensor, 4, 2, str_sensor);
  dtostrf(sensor2, 4, 2, str_sensor2);
  dtostrf(sensor3, 4, 2, str_sensor3);
    sprintf(payload, "{\"");
  sprintf(payload, "%s%s\":%s", payload, "Ultrasonic", str_sensor);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_Y", str_sensor2);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_X", str_sensor3);
    sprintf(payload, "%s}", payload);
  Serial.println(payload);
  Serial.println("Publishing data to Ubidots Cloud");
  client.publish(topic, payload);
  client.loop();
  
    delay(3000); // task repeat every number of milliseconds

  }
}

void Task2code( void * pvParameters ){
  while (true){
    Serial.println("Hello from Second task");
    
      sensor = random(500); 
  sensor2 = random(500); 
  sensor3 = random(500); 
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

void DataCollection(){
  sensor = random(500); 
  sensor2 = random(500); 
  sensor3 = random(500); 
  
}

void DataToCloud() {
  if (!client.connected()) {
    reconnect();
  }

  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  //sprintf(payload, "{\"%s\":", VARIABLE_LABEL); // Adds the variable label
  
  //float sensor = random(500); 
  
  /* 4 is mininum width, 2 is precision; float value is copied onto str_sensor*/
  dtostrf(sensor, 4, 2, str_sensor);
  dtostrf(sensor2, 4, 2, str_sensor2);
  dtostrf(sensor3, 4, 2, str_sensor3);
    sprintf(payload, "{\"");
  sprintf(payload, "%s%s\":%s", payload, "Ultrasonic", str_sensor);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_Y", str_sensor2);
  sprintf(payload, "%s,\"%s\":%s", payload, "Gyro_X", str_sensor3);
    sprintf(payload, "%s}", payload);
  Serial.println(payload);
  Serial.println("Publishing data to Ubidots Cloud");
  client.publish(topic, payload);
  client.loop();
  delay(1000);
}
