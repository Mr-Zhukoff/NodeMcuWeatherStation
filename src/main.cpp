#include <stdint.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SparkFunBME280.h>
//Library allows either I2C or SPI, so include both.
#include <Wire.h>
#include <SPI.h>

//Global sensor object
BME280 bme280;
// const char* ssid = "NinaWiFi";
// const char* password = "15426378";
const char* ssid = "ZhukoffAP";
const char* password = "18273645";

const char *mqtt_server = "m21.cloudmqtt.com";
const int mqtt_port = 19115;
const char *mqtt_user = "nodemcu";
const char *mqtt_pass = "qwerty123";
const char *mqtt_client = "NodeMCUv3"; // Client connections cant have the same connection name
const char* outTopic = "NodeMCUv3/BME280/";
const char* inTopic = "NodeMCUv3/BME280/";
char msg[50];

float h, t, p, pin, dp;
char temperatureCString[6];
char dpString[6];
char humidityString[6];
char pressureString[7];
char pressureMmString[6];

unsigned long previousMillis = 0;        // will store last temp was send
const long interval = 300000;

long lastReconnectAttempt = 0;
// Web Server on port 80
WiFiServer wwwserver(80);
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

    dtostrf(t, 5, 1, temperatureCString);
    dtostrf(h, 5, 1, humidityString);
    dtostrf(p, 6, 1, pressureString);
    dtostrf(pin, 5, 2, pressureMmString);
    dtostrf(dp, 5, 1, dpString);
    delay(100);
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
    Serial.println(temperatureCString);
    Serial.println(humidityString);
    Serial.println(pressureMmString);
    // Send readings
    mqttclient.publish("NodeMCUv3/BME280/temperature", temperatureCString);
    mqttclient.publish("NodeMCUv3/BME280/humidity", humidityString);
    mqttclient.publish("NodeMCUv3/BME280/pressure", pressureMmString);
  }
  if(message == "temperature"){
    Serial.print("Sending temperature:");
    Serial.println(temperatureCString);
    mqttclient.publish("NodeMCUv3/BME280/temperature", temperatureCString);
  }
  else if (message == "humidity"){
    Serial.print("Sending humidity:");
    Serial.println(humidityString);
    mqttclient.publish("NodeMCUv3/BME280/humidity", humidityString);
  }
  else if (message == "pressure"){
    Serial.print("Sending pressure:");
    Serial.println(pressureMmString);
    mqttclient.publish("NodeMCUv3/BME280/pressure", pressureMmString);
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

	Serial.print("Displaying concatenated calibration words\n");
	Serial.print("dig_T1, uint16: ");
	Serial.println(bme280.calibration.dig_T1);
	Serial.print("dig_T2, int16: ");
	Serial.println(bme280.calibration.dig_T2);
	Serial.print("dig_T3, int16: ");
	Serial.println(bme280.calibration.dig_T3);

	Serial.print("dig_P1, uint16: ");
	Serial.println(bme280.calibration.dig_P1);
	Serial.print("dig_P2, int16: ");
	Serial.println(bme280.calibration.dig_P2);
	Serial.print("dig_P3, int16: ");
	Serial.println(bme280.calibration.dig_P3);
	Serial.print("dig_P4, int16: ");
	Serial.println(bme280.calibration.dig_P4);
	Serial.print("dig_P5, int16: ");
	Serial.println(bme280.calibration.dig_P5);
	Serial.print("dig_P6, int16: ");
	Serial.println(bme280.calibration.dig_P6);
	Serial.print("dig_P7, int16: ");
	Serial.println(bme280.calibration.dig_P7);
	Serial.print("dig_P8, int16: ");
	Serial.println(bme280.calibration.dig_P8);
	Serial.print("dig_P9, int16: ");
	Serial.println(bme280.calibration.dig_P9);

	Serial.print("dig_H1, uint8: ");
	Serial.println(bme280.calibration.dig_H1);
	Serial.print("dig_H2, int16: ");
	Serial.println(bme280.calibration.dig_H2);
	Serial.print("dig_H3, uint8: ");
	Serial.println(bme280.calibration.dig_H3);
	Serial.print("dig_H4, int16: ");
	Serial.println(bme280.calibration.dig_H4);
	Serial.print("dig_H5, int16: ");
	Serial.println(bme280.calibration.dig_H5);
	Serial.print("dig_H6, uint8: ");
	Serial.println(bme280.calibration.dig_H6);

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
	// Starting the web server
	wwwserver.begin();
	Serial.println("Web server running. Waiting for the ESP IP...");
  for (size_t i = 0; i < 10; i++) {
    Serial.print(".");
    delay(1000);
  }
	Serial.println("Getting BME280 sensor data...");
	getWeather();
}

void loop()
{
	// Listenning for new clients
  WiFiClient client = wwwserver.available();

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
      mqttclient.publish("NodeMCUv3/BME280/temperature", temperatureCString);
      mqttclient.publish("NodeMCUv3/BME280/humidity", humidityString);
      mqttclient.publish("NodeMCUv3/BME280/pressure", pressureMmString);

    }
    mqttclient.loop();
  }
}
