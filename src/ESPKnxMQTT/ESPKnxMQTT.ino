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
#include <cppQueue.h> // https://github.com/SMFSW/Queue

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
	LOG_LEVEL_ALL
};

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

	// load configuration
	if (Configuration_Load())
		WriteLog(LOG_LEVEL_DEBUG, "LoadConfig OK");
	else
		WriteLog(LOG_LEVEL_ERROR, "LoadConfig Error");

	// startup Wifi Connection
	RUNTIME.wiFiConnected = WiFi_Startup();

	_MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
	_MQTTClient.setCallback(MQTT_Callback);
}

void loop()
{
	//
	// code life cycle
	//

	// in case of mqtt disconnect
	if (!_MQTTClient.connected())
		MQTT_Reconnect();

	// loop mqtt client
	_MQTTClient.loop();

	// initialize gate
	if (!RUNTIME.KnxGateInit)
	{
		RUNTIME.KnxGateInit = KNX_Init();
	}
	else
	{
		// current millisec
		unsigned long currentMillis = millis();

		if (currentMillis - previousMillis >= LOOP_INTERVAL_MILLISEC)
		{
			// check
			for (int i = 0; i < commandList.getCount(); i++)
			{
				_command cmd;
				commandList.pop(&cmd);

				char log[250];
				snprintf_P(log, sizeof(log), "exec command type %d to device address [%s]", cmd.cmdType, cmd.knxDeviceAddress);
				WriteLog(LOG_LEVEL_DEBUG, log);

				KNX_ExeCommand(cmd.knxDeviceAddress, cmd.cmdType);
			}

			// read bus
			KNX_ReadBus();

			// salvo ultima esecuzione
			previousMillis = currentMillis;
		}
	}
}
