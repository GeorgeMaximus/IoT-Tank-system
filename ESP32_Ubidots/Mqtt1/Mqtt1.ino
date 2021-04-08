
#include <WiFi.h>

#include <PubSubClient.h>

long  i;
IPAddress staticIP912_10(192,168,1,10);
IPAddress gateway912_10(192,168,1,1);
IPAddress subnet912_10(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);

void reconnectmqttserver() {
while (!client.connected()) {
Serial.print("Attempting MQTT connection...");
String clientId = "ESP32Client-";
 clientId += String(random(0xffff), HEX);
if (client.connect(clientId.c_str())) {
Serial.println("connected");
} else {
Serial.print("failed, rc=");
Serial.print(client.state());
Serial.println(" try again in 5 seconds");
delay(5000);
}
}
}

char msgmqtt[50];
void callback(char* topic, byte* payload, unsigned int length) {
  String MQTT_DATA = "";
  for (int i=0;i<length;i++) {
   MQTT_DATA += (char)payload[i];}

}

void setup()
{
i = 0;
Serial.begin(9600);

  WiFi.disconnect();
  delay(3000);
  Serial.println("START");
  WiFi.begin("GeorgeMaximus","maxi1234");
  while ((!(WiFi.status() == WL_CONNECTED))){
    delay(300);
    Serial.print("..");

  }
  Serial.println("Connected");
  WiFi.config(staticIP912_10, gateway912_10, subnet912_10);
  Serial.println("Your IP is");
  Serial.println((WiFi.localIP()));
  client.setServer("http://localhost/", 1883);
  client.setCallback(callback);

}


void loop()
{

    if (!client.connected()) {
    reconnectmqttserver();
    }
    client.loop();
    snprintf (msgmqtt, 50, "%ld ",i);
    client.publish("test", msgmqtt);
    i = i + 1;
    delay(4000);

}
