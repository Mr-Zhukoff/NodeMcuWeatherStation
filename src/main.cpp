#include <stdint.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LedControl.h>

// const char* ssid = "NinaWiFi";
// const char* password = "15426378";
const char* ssid = "webclinic";
const char* password = "Hellokitty12";

const char *mqtt_server = "m21.cloudmqtt.com";
const int mqtt_port = 19115;
const char *mqtt_user = "nodemcu";
const char *mqtt_pass = "qwerty123";
const char *mqtt_client = "NodeMCUv3"; // Client connections cant have the same connection name
const char *inTopic="In/NodeMCUv3/DHT22";
const char *outTopicTemp = "Out/NodeMCUv3/DHT22/Temperature";
const char *outTopicHum="Out/NodeMCUv3/DHT22/Humidity";
const char *outTopicPress="Out/NodeMCUv3/DHT22/Pressure";
const char *outTopicJson="Out/NodeMCUv3/DHT22/Json";

char msg[50];

#define DHT1PIN 2
#define DHTTYPE DHT22

float h, t;
char temperatureCString[10];
char humidityString[10];
char tCharVal[5];
char hCharVal[5];

unsigned long previousMillis = 0;        // will store last temp was send
const long interval = 15000;

long lastReconnectAttempt = 0;

// CS (CS)(LOAD), CLK (CLK), LOAD(DIN), drivers
LedControl lc = LedControl(12, 14, 13, 1);

DHT dht1(DHT1PIN, DHTTYPE);
WiFiClient wificlient;
PubSubClient mqttclient(wificlient);

void showOnLcd(){
  Serial.println("Showing LCD value");
  lc.clearDisplay(0);
  dtostrf(t, 2, 2, tCharVal);
  dtostrf(h, 2, 2, hCharVal);
  Serial.println(tCharVal);
  Serial.println(hCharVal);
  lc.setChar(0,0,' ',false); // пустота
  lc.setChar(0,1,' ',false);
  lc.setChar(0,2,' ',false);
  lc.setChar(0,3,' ',false);
}

void getWeather() {
    h = dht1.readHumidity();
    t = dht1.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

		Serial.print("Temperature: ");
		Serial.print(t, 2);
		Serial.println(" C");


		Serial.print("Humidity: ");
		Serial.print(h, 2);
		Serial.println(" %");

		Serial.println();

    showOnLcd();

    dtostrf(t, 5, 1, temperatureCString);
    dtostrf(h, 5, 1, humidityString);

    delay(100);
}

void sendJsonData(){
  // Memory pool for JSON object tree.
  // Inside the brackets, 200 is the size of the pool in bytes.
  // If the JSON object is more complex, you need to increase that value.
    StaticJsonBuffer<200> jsonBuffer;

    // Create the root of the object tree.
    // It's a reference to the JsonObject, the actual bytes are inside the
    // JsonBuffer with all the other nodes of the object tree.
    // Memory is freed when jsonBuffer goes out of scope.
    JsonObject& root = jsonBuffer.createObject();

    root["name"] = "DHT22";
    root["temperature"] = t;
    root["humidity"] = h;
    //root["pressure"] = pin;

    int length = root.measureLength() + 1;
    char json[length];

    root.printTo(json, sizeof(json));

    Serial.println(json);

    mqttclient.publish(outTopicJson, json);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Conver the incoming byte array to a string
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
  Serial.print("Message arrived on topic: [");
  Serial.print(topic);
  Serial.print("], ");
  Serial.println(message);

  getWeather();
  if(message == "GetData"){
    Serial.print("Sending all data");
    mqttclient.publish(outTopicTemp, temperatureCString);
    mqttclient.publish(outTopicHum, humidityString);
    //mqttclient.publish(outTopicPress, pressureMmString);
    sendJsonData();
  }
  if(message == "temperature"){
    Serial.print("Sending temperature:");
    mqttclient.publish(outTopicTemp, temperatureCString);
  }
  else if (message == "humidity"){
    Serial.print("Sending humidity:");
    mqttclient.publish(outTopicHum, humidityString);
  }
  else if (message == "pressure"){
    Serial.print("Sending pressure:");
    //mqttclient.publish(outTopicPress, pressureMmString);
  }
}

boolean reconnect() {
  Serial.println("Attempting MQTT connection...");   // Attempt to connect
  if(!mqttclient.connect(mqtt_client, mqtt_user, mqtt_pass)){
    switch (mqttclient.state()) {
      case -4: Serial.println("Connection timeout"); break;
       case -3: Serial.println("Connection lost"); break;
       case -2: Serial.println("Connection failed"); break;
       case -1: Serial.println("Disconnected"); break;
       case 0: Serial.println("Connected"); break;
       case 1: Serial.println("Wrong protocol"); break;
       case 2: Serial.println("Wrong client id"); break;
       case 3: Serial.println("Server unavailable"); break;
       case 4: Serial.println("Wrong user/password"); break;
       case 5: Serial.println("Not authenticated"); break;
       default:
       Serial.print("Couldn't connect to server, state: ");
       Serial.println(mqttclient.state());
       break;
     }
     return mqttclient.connected();
  }
  else{
    mqttclient.subscribe(inTopic);
    return true;
  }
}


void setup() {
	Serial.begin(9600);
	Serial.print("\nProgram Started\n");

	// Connecting to WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected");
  // Printing the ESP IP address
  Serial.println(WiFi.localIP());

  // LCD Initialize MAX7219
  lc.shutdown(0, false);// turning LCD on
  lc.setIntensity(0, 8);// setting brghtness (0-min, 15-max)
	lc.clearDisplay(0);// clear display

  mqttclient.setServer(mqtt_server, mqtt_port);
  mqttclient.setCallback(callback);
  lastReconnectAttempt = 0;

	Serial.println("Getting DHT sensor data...");
  delay(500);
	getWeather();
}

void loop()
{
if (!mqttclient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
  else {
    // Client connected
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval) {
      // save the last time you read the sensor
      previousMillis = currentMillis;
      // Update sensor data
      getWeather();
      // Send readings
      mqttclient.publish(outTopicTemp, temperatureCString);
      mqttclient.publish(outTopicHum, humidityString);
      //mqttclient.publish(outTopicPress, pressureMmString);
    }
    mqttclient.loop();
  }
}
