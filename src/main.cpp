#include <stdint.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SparkFunBME280.h>
//Library allows either I2C or SPI, so include both.
#include <Wire.h>
#include <SPI.h>

//Global sensor object
BME280 bme280;
// const char* ssid = "NinaWiFi";
// const char* password = "15426378";
const char* ssid = "ProdwareRus";
const char* password = "18273645";

const char *mqtt_server = "m21.cloudmqtt.com";
const int mqtt_port = 19115;
const char *mqtt_user = "nodemcu";
const char *mqtt_pass = "qwerty123";
const char *mqtt_client = "NodeMCUv3"; // Client connections cant have the same connection name
const char *inTopic="In/NodeMCUv3/BME280";
const char *outTopicTemp = "Out/NodeMCUv3/BME280/Temperature";
const char *outTopicHum="Out/NodeMCUv3/BME280/Humidity";
const char *outTopicPress="Out/NodeMCUv3/BME280/Pressure";
const char *outTopicJson="Out/NodeMCUv3/BME280/Json";

char msg[50];

float h, t, p, pin, dp;
char temperatureCString[10];
char dpString[10];
char humidityString[10];
char pressureString[10];
char pressureMmString[10];

unsigned long previousMillis = 0;        // will store last temp was send
const long interval = 15000;

long lastReconnectAttempt = 0;

WiFiClient wificlient;
PubSubClient mqttclient(wificlient);

void getWeather() {
    h = bme280.readFloatHumidity();
    t = bme280.readTempC();
    //t = t*1.8+32.0;
    dp = t-0.36*(100.0-h);
    p = bme280.readFloatPressure()/100.0F;
    pin = 0.7500637554192*p;

		Serial.print("Temperature: ");
		Serial.print(t, 2);
		Serial.println(" C");

		Serial.print("Pressure: ");
		Serial.print(p, 2);
		Serial.println(" hPa");

		Serial.print("Humidity: ");
		Serial.print(h, 2);
		Serial.println(" %");

		Serial.println();

    // sprintf(temperatureCString, "%f", t);
    // sprintf(pressureString, "%f", p);
    // sprintf(pressureMmString, "%f", pin);
    // sprintf(dpString, "%f", dp);
    // sprintf(humidityString, "%f", h);
    dtostrf(t, 5, 1, temperatureCString);
    dtostrf(h, 5, 1, humidityString);
    dtostrf(p, 6, 1, pressureString);
    dtostrf(pin, 5, 2, pressureMmString);
    dtostrf(dp, 5, 1, dpString);
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

    root["name"] = "BME280";
    root["temperature"] = t;
    root["humidity"] = h;
    root["pressure"] = pin;

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
    mqttclient.publish(outTopicPress, pressureMmString);
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
    mqttclient.publish(outTopicPress, pressureMmString);
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
	bme280.settings.commInterface = I2C_MODE;
	bme280.settings.I2CAddress = 0x76;
	bme280.settings.runMode = 3; //Normal mode
	bme280.settings.tStandby = 0; //  0, 0.5ms
	bme280.settings.filter = 0; //  0, filter off
	bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
	bme280.settings.humidOverSample = 1;

	Serial.begin(9600);
	Serial.print("Program Started\n");
	Serial.print("Starting BME280... result of .begin(): 0x");

	//Calling .begin() causes the settings to be loaded
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	Serial.println(bme280.begin(), HEX);

	Serial.print("Displaying ID, reset and ctrl regs\n");

	Serial.print("ID(0xD0): 0x");
	Serial.println(bme280.readRegister(BME280_CHIP_ID_REG), HEX);
	Serial.print("Reset register(0xE0): 0x");
	Serial.println(bme280.readRegister(BME280_RST_REG), HEX);
	Serial.print("ctrl_meas(0xF4): 0x");
	Serial.println(bme280.readRegister(BME280_CTRL_MEAS_REG), HEX);
	Serial.print("ctrl_hum(0xF2): 0x");
	Serial.println(bme280.readRegister(BME280_CTRL_HUMIDITY_REG), HEX);

	Serial.print("\n\n");

	Serial.print("Displaying all regs\n");
	uint8_t memCounter = 0x80;
	uint8_t tempReadData;
	for(int rowi = 8; rowi < 16; rowi++ )
	{
		Serial.print("0x");
		Serial.print(rowi, HEX);
		Serial.print("0:");
		for(int coli = 0; coli < 16; coli++ )
		{
			tempReadData = bme280.readRegister(memCounter);
			Serial.print((tempReadData >> 4) & 0x0F, HEX);//Print first hex nibble
			Serial.print(tempReadData & 0x0F, HEX);//Print second hex nibble
			Serial.print(" ");
			memCounter++;
		}
		Serial.print("\n");
	}

	Serial.print("\n\n");

	if (!bme280.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

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

  mqttclient.setServer(mqtt_server, mqtt_port);
  mqttclient.setCallback(callback);
  lastReconnectAttempt = 0;

	Serial.println("Getting BME280 sensor data...");
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
      mqttclient.publish(outTopicPress, pressureMmString);

    }
    mqttclient.loop();
  }
}
