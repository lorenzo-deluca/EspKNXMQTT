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
#include <PubSubClient.h> 	// https://github.com/knolleary/pubsubclient
#include <WiFiManager.h>	// https://github.com/tzapu/WiFiManager
#include <cppQueue.h>	 	// https://github.com/SMFSW/Queue

extern "C"
{
#include "user_interface.h"
}

// program definitions
#include "./define.h"

// runtime data
_runtimeType RUNTIME = {
	false, 			// mqttConnected
	false, 			// wiFiConnected
	false, 			// KnxGateInit
	false 	 		// mqttDiscoveryEnabled
};

// initial default config
_syscfgType SYSCONFIG = {
	CONFIG_VERSION, 	// version
	false, 				// networkConfig
	true, 				// mqttEnable
	true, 				// mqttUpdateEnable
	LOG_LEVEL_ERROR, 	// serialLogLevel
	LOG_LEVEL_ALL,		// mqttLogLevel
	"",					// mqtt_server
	"1883"				// mqtt_port
	};

WiFiClient _WiFiClient;
PubSubClient _MQTTClient(_WiFiClient);

// Timer loop from http://www.arduino.cc/en/Tutorial/BlinkWithoutDelayS
long previousMillis = 0;
unsigned long lastConnectTry = 0;

// Instantiate command queue
Queue commandList(sizeof(_command), CMD_LIST_SIZE, CMD_LIST_IMPLEMENTATION);

WiFiManager wifiManager;

// Add parameters to WiFiManager
WiFiManagerParameter custom_mqtt_server("mqtt_server", "mqtt server", SYSCONFIG.mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("mqtt_port", "mqtt port", SYSCONFIG.mqtt_port, 6);	

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

	Serial.println("Startup");
	Serial.println("Press A for start as AP");
	Serial.setTimeout(10000);
	char serin[4];
	Serial.readBytes(serin,1);
	if ((serin[0] == 'A') || (serin[0] == 'a')) 
	{
		wifiManager.resetSettings();
		Serial.println("Forced AP mode");
	}
	Serial.setTimeout(1000);

	wifiManager.addParameter(&custom_mqtt_server);
	wifiManager.addParameter(&custom_mqtt_port);
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	wifiManager.autoConnect(APSSID, APPSK);

	//if you get here you have connected to the WiFi
	WriteLog(LOG_LEVEL_DEBUG, "WiFi connected - IPv4 =");
    WriteLog(LOG_LEVEL_DEBUG, WiFi.localIP().toString().c_str());
	
	// 
	RUNTIME.Configured = Configuration_Load(); 
	lastConnectTry = millis();

	// load configuration
	if (RUNTIME.Configured)
	{
		WriteLog(LOG_LEVEL_DEBUG, "LoadConfig OK");

		// startup Wifi Connection
		RUNTIME.wiFiConnected = true;

		// configure MQTT
		_MQTTClient.setServer(SYSCONFIG.mqtt_server, MQTT_PORT);
		_MQTTClient.setCallback(MQTT_Callback);
	}
	else
	{
		WriteLog(LOG_LEVEL_ERROR, "LoadConfig NOK");
		wifiManager.resetSettings();
		
		delay(3000);
		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(5000);
	}
}

void loop()
{
	//
	// code life cycle
	//
	if(!RUNTIME.Configured)
		return;
	
    unsigned int s = WiFi.status();

	if (s == 0 && millis() > (lastConnectTry + 60000)) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
	  lastConnectTry = millis();
      wifiManager.autoConnect();
    }

	RUNTIME.wiFiConnected = (s == WL_CONNECTED);

	if (!RUNTIME.wiFiConnected)
		return;

	// WIFI Connected

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

	// webserver stuff
	// WebServer_Loop();
}
