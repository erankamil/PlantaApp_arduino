#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define HOURS_IN_DAY 24
#define LIGHT_THRESHOLD 600

const char* serverName = "http://ec2-54-216-234-17.eu-west-1.compute.amazonaws.com/api/v1/planta/add-measurement-value";
const char* ssid = "wifi_name";
const char* password = "wifi_password";
String sensorID = "sensor_id";

/* Set timer to 1 hour */
unsigned long timerDelay = 3600000;
/* Set timer to 30 seconds */
unsigned long testTimerDelay = 30000;

unsigned long lastTime = 0;
unsigned int hourCount = 0;
int LightSensorPin = 24; 
int MoistureSensorPin = 23;
int lightValue = 0;  
int lightHoursCount = 0;
int moistureValue = 0;

/* Map values for the soil moisture sensor samplening */
int mapHigh = 750;
int mapLow = 330;

void ConnectToWifi();
void SendMoistureSensorData();
void SendLightSensorData();

void setup() {
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  ConnectToWifi();
}

void loop() {
  
  /*Send an HTTP POST request to the server every 60 minutes */
  if ((millis() - lastTime) > testTimerDelay) {
    
    /*Check WiFi connection status */
    if (WiFi.status()== WL_CONNECTED) {  
       
       /* Mesure the soil moisture sensor and send the sampling */
       SendMoistureSensorData();

       /* Mesure the light sensor and send the sampling */
       SendLightSensorData();      
    }
    else {
      Serial.println("WiFi Disconnected");
      ConnectToWifi();
    }
    lastTime = millis();
  }
}

void ConnectToWifi(){
  
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void SendMoistureSensorData(){

   WiFiClient client;
   HTTPClient http;
   http.begin(client, serverName);
       
   moistureValue = analogRead(MoistureSensorPin); 
   Serial.print("Moisture value: ");
   Serial.println(moistureValue);
   
   int percentageValue = map(moistureValue, mapLow, mapHigh, 0 , 100);
   
   Serial.print("Moisture percentage: ");
   Serial.println(percentageValue);
   
   String ValueToSend = String(percentageValue, DEC); 
   http.addHeader("Content-Type", "application/json");

   String httpRequestData = "{\"sensorId\":\"" + sensorID + "\",\"measurementType\":\"soilMoisturePerHour\",\"value\":\"" + ValueToSend + "\"}";
   Serial.println("Making HTTP PUT request to server with soil moisture percentage.");
   int httpResponseCode = http.PUT(httpRequestData);

   Serial.print("HTTP Response code: ");
   Serial.println(httpResponseCode);

   /* Free resources */
   http.end();      
}

void SendLightSensorData(){
   
   lightValue = analogRead(LightSensorPin);
    
   Serial.print("Light value: ");
   Serial.println(lightValue);
  
   hourCount+=1;
   Serial.print("Number of hours sampled today: ");
   Serial.println(hourCount);
   
   /*In case of value greater than threshold, increace light counter by one */
   if (lightValue >= LIGHT_THRESHOLD) {
        lightHoursCount+=1;
   }
   
   if (hourCount == HOURS_IN_DAY) {
    
      WiFiClient client;
      HTTPClient http;
      http.begin(client, serverName);

      Serial.print("Hours of light sampled today: ");
      Serial.println(lightHoursCount);
   
      String ValueToSend = String(lightHoursCount, DEC); 
      http.addHeader("Content-Type", "application/json");

      String httpRequestData = "{\"sensorId\":\"" + sensorID + "\",\"measurementType\":\"hoursOflightPerDay\",\"value\":\"" + ValueToSend + "\"}";
      Serial.println("Making HTTP PUT request to server with number of light hours for today.");
      int httpResponseCode = http.PUT(httpRequestData);

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      
      hourCount = 0;
      lightHoursCount = 0;

      /* Free resources */
      http.end(); 
   }
}
