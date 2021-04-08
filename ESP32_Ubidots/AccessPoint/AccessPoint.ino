
#include <WiFi.h>

void setup()
{
Serial.begin(9600);

  delay(1000);
  Serial.println("Starting AP with ESP");
  WiFi.softAP("Esp32AccessPoint");
  Serial.println("Ip Address for the AP");
  Serial.println((WiFi.softAPIP()));

}


void loop()
{

    Serial.println("Number of station connected");
    Serial.println((WiFi.softAPgetStationNum()));
    delay(3000);

}
