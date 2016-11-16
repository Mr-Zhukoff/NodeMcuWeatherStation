#include <stdint.h>
#include <ESP8266WiFi.h>
#include <SparkFunBME280.h>
//Library allows either I2C or SPI, so include both.
#include <Wire.h>
#include <SPI.h>

//Global sensor object
BME280 bme280;
const char* ssid = "NinaWiFi";
const char* password = "15426378";
//const char* ssid = "Tomato24";
//const char* password = "medical122015";
float h, t, p, pin, dp;
char temperatureCString[6];
char dpString[6];
char humidityString[6];
char pressureString[7];
char pressureMmString[6];

// Web Server on port 80
WiFiServer server(80);

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

void setup()
{
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

	// Starting the web server
	server.begin();
	Serial.println("Web server running. Waiting for the ESP IP...");
	delay(10000);
	// Printing the ESP IP address
	Serial.println(WiFi.localIP());
	Serial.println("Getting BME280 sensor data...");
	getWeather();
}

void loop()
{
	// Listenning for new clients
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client");
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (c == '\n' && blank_line) {
            getWeather();
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            // your actual web page that displays temperature
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<head><META HTTP-EQUIV=\"refresh\" CONTENT=\"15\"></head>");
            client.println("<body><h1>ESP8266 Weather Web Server</h1>");
            client.println("<table border=\"2\" width=\"456\" cellpadding=\"10\"><tbody><tr><td>");
            client.println("<h3>Temperature = ");
            client.println(temperatureCString);
            client.println(" &#8451;</h3><h3>Humidity = ");
            client.println(humidityString);
            client.println(" %</h3><h3>Pressure = ");
            client.println(pressureString);
            client.println(" hPa (");
            client.println(pressureMmString);
            client.println(" mmHg.)</h3></td></tr></tbody></table></body></html>");
            break;
        }
        if (c == '\n') {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r') {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }
    // closing the client connection
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }
}
