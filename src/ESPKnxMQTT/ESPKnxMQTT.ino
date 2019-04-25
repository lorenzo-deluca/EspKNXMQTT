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
	false,			// Configured
	false, 			// mqttConnected
	false, 			// wiFiConnected
	false, 			// KnxGateInit
	false,			// KnxDiscoveryEnabled
	false 	 		// mqttDiscoveryEnabled
};

// initial default config
_syscfgType SYSCONFIG = {
	CONFIG_VERSION, 	// version
	true, 				// mqttEnable
	true, 				// mqttUpdateEnable
	true,				// webServerEnable
	LOG_LEVEL_ERROR, 	// serialLogLevel
	LOG_LEVEL_ALL,		// mqttLogLevel
	
	"",					// mqtt_server[20]
	"",					// mqtt_port[6]
	"",					// mqtt_user[8]
	"", 				// mqtt_password[10]
   
	false //,				// static_cfg
	//"",					// static_ip[];
	//"",					// static_mask[];
	//""					// static_gtw[];
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
WiFiManagerParameter cfg_mqtt_server	("mqtt_server", "mqtt server", SYSCONFIG.mqtt_server, 20);
WiFiManagerParameter cfg_mqtt_port		("mqtt_port", "mqtt port", SYSCONFIG.mqtt_port, 6);
WiFiManagerParameter cfg_mqtt_user		("mqtt_user", "mqtt user", SYSCONFIG.mqtt_user, 8);
WiFiManagerParameter cfg_mqtt_password	("mqtt_password", "mqtt password", SYSCONFIG.mqtt_password, 10);

//WiFiManagerParameter cfg_static_ip		("static_ip", "static ip", SYSCONFIG.static_ip, IP_ADDRESS_LEN);
//WiFiManagerParameter cfg_static_mask	("static_mask", "static mask", SYSCONFIG.static_mask, IP_ADDRESS_LEN);
//WiFiManagerParameter cfg_static_gtw		("static_gtw", "static gtw", SYSCONFIG.static_gtw, IP_ADDRESS_LEN);

void setup()
{
	// startup
	pinMode(2, INPUT);
	pinMode(0, OUTPUT);

	// 
	//	10 seconds wait
	int pauses = 0;
	while (pauses < 100) 
	{
		pauses++;
		delay(100);
	}

	Serial.begin(115200);
	while (!Serial)
	{
		;
	}
	Serial.flush();
	digitalWrite(0, LOW); // A0 OUTPUT BASSO
	delay(10);

	strcpy(SYSCONFIG.mqtt_topic_prefix, TOPIC_PREFIX);

	// Load configuration from EEPROM
	RUNTIME.Configured = Configuration_Load();

	// config WiFi Manger
	WiFiManager_Setup();

	// 
 	pinMode(2, INPUT);
	if (digitalRead(2) == 0) 
	{
		// wifiManager.resetSettings();
		wifiManager.startConfigPortal(APSSID, APPSK);
		WriteLog(LOG_LEVEL_INFO, "Forced AP mode - reset WifiManager Settings");
	}
	else
	{
		// WiFi.disconnect();
		Serial.setTimeout(1000);
		if (!wifiManager.autoConnect(APSSID, APPSK))
		{
			delay(3000);
			ESP.restart();
		}
	}
	
	//if you get here you have connected to the WiFi
//	WriteLog(LOG_LEVEL_INFO, "WiFi connected - IPv4 =");
//	WriteLog(LOG_LEVEL_INFO, WiFi.localIP().toString().c_str());
	
	lastConnectTry = millis();

	// load configuration
	if (RUNTIME.Configured)
	{
		// startup Wifi Connection
		RUNTIME.wiFiConnected = true;

		// configure MQTT
		_MQTTClient.setServer(SYSCONFIG.mqtt_server, atoi(SYSCONFIG.mqtt_port));
		_MQTTClient.setCallback(MQTT_Callback);
	}
	else
	{
		WriteLog(LOG_LEVEL_ERROR, "LoadConfig NOK - Start AP Mode");

		wifiManager.startConfigPortal(APSSID, APPSK);
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

	if (s == 0 && millis() > (lastConnectTry + 30000)) 
	{
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
