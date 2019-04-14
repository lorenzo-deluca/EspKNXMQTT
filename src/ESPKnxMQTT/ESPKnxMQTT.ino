/*
  EspKnxMQTT.ino - ESPKNXMQTT

  Copyright (C) 2019 Lorenzo De Luca (me@lorenzodeluca.info)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Arduino C++ Library
#include <Arduino.h>

// external library
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <cppQueue.h>	 // https://github.com/SMFSW/Queue

extern "C"
{
#include "user_interface.h"
}

// program definitions
#include "./define.h"

// runtime data
_runtimeType RUNTIME = {
	false, // mqttConnected
	false, // wiFiConnected
	false, // KnxGateInit
	false  // mqttDiscoveryEnabled
};

// initial default config
_syscfgType SYSCONFIG = {
	CONFIG_VERSION,
	false,
	true,
	true,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_ALL};

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* Set these to your desired softAP credentials. They are not configurable at runtime */
#ifndef APSSID
#define APSSID "ESPKnxMQTT_ap"
#define APPSK "12345678"
#endif

const char *softAP_ssid = APSSID;
const char *softAP_password = APPSK;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "ESPKnxMQTT";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
unsigned long lastConnectTry = 0;

/** Current WLAN status */
unsigned int status = WL_IDLE_STATUS;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

WiFiClient _WiFiClient;
PubSubClient _MQTTClient(_WiFiClient);

// Timer loop from http://www.arduino.cc/en/Tutorial/BlinkWithoutDelayS
long previousMillis = 0;

// Instantiate command queue
Queue commandList(sizeof(_command), CMD_LIST_SIZE, CMD_LIST_IMPLEMENTATION);

void setup()
{
	// startup
	pinMode(2, INPUT);
	pinMode(0, OUTPUT);

	// aspetta 15 secondi perche' con l'assorbimento iniziale di corrente esp8266 fa disconnettere l'adattatore seriale
	//	int pauses = 0;
	//	while (pauses < 150) // 15 secondi wait
	//	{
	//		pauses++;
	//		delay(100); // wait 100ms
	//	}

	Serial.begin(115200);
	while (!Serial)
	{
		;
	}
	Serial.flush();
	digitalWrite(0, LOW); // A0 OUTPUT BASSO
	delay(10);

	// if AP jumper force AP mode
	RUNTIME.Configured = Configuration_Load(); 

	// load configuration
	if (RUNTIME.Configured)
	{
		WriteLog(LOG_LEVEL_DEBUG, "LoadConfig OK");

		// startup Wifi Connection
		RUNTIME.wiFiConnected = WiFi_ClientMode();

		// configure MQTT
		_MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
		_MQTTClient.setCallback(MQTT_Callback);
	}
	else
	{
		WriteLog(LOG_LEVEL_ERROR, "LoadConfig Error");

		WiFi_APMode();
	}
}

void loop()
{
	//
	// code life cycle
	//

	if (RUNTIME.Configured)
	{
		MQTT_Loop();

		if (RUNTIME.mqttConnected)
		{
			// initialize gate
			if (!RUNTIME.KnxGateInit)
				RUNTIME.KnxGateInit = KNX_Init();
			else
			{
				// current millisec
				unsigned long currentMillis = millis();

				if (currentMillis - previousMillis >= LOOP_INTERVAL_MILLISEC)
				{
					KNX_Loop();

					// save last exec
					previousMillis = currentMillis;
				}
			}
		}
	}

	// webserver stuff
	WebServer_Loop();
}
