/*
  EspKnxMQTT.cpp - ESPKNXMQTT

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

// external library
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

#include <include.h>

bool mqttConnected = false;
bool wiFiConnected = false;
bool KnxGateInit = false;
bool mqttDiscoveryEnabled = false;

// initial config
syscfg_type SYSCONFIG = {
	CONFIG_VERSION,
	false,
	true,
	true,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_ALL};

WiFiClient _WiFiClient;
PubSubClient _MQTTClient(_WiFiClient);

long previousMillis = 0; // Timer loop from http://www.arduino.cc/en/Tutorial/BlinkWithoutDelay

void MQTT_Publish(const char *topic, const char *payload)
{
	if (!mqttConnected)
		return;

	_MQTTClient.publish(topic, payload);
	yield();

	if (SYSCONFIG.serialLogLevel >= LOG_LEVEL_DEBUG)
	{
		char Log[250];
		snprintf_P(Log, sizeof(Log), "MQTT_Publish topic <%s> payload <%s>", topic, payload);
		Serial.println(Log);
	}
}

void WriteLog(int msgLevel, const char *msgLog)
{
	if (SYSCONFIG.serialLogLevel >= msgLevel)
		Serial.println(msgLog);

	if (SYSCONFIG.mqttLogLevel >= msgLevel)
	{
		char Log[250];
		snprintf_P(Log, sizeof(Log), "%lu - %s", millis(), msgLog);
		MQTT_Publish(TOPIC_LOG, msgLog);
	}
}

void MQTT_Reconnect()
{
	// Loop until we're reconnected
	while (!_MQTTClient.connected())
	{
		WriteLog(LOG_LEVEL_DEBUG, "Attempting MQTT connection...");

		// Provo a connettermi al server MQTT
		if (_MQTTClient.connect(mqtt_device, MQTT_USER, MQTT_PASSWORD))
		{
			WriteLog(LOG_LEVEL_DEBUG, "connected");
			mqttConnected = true;

			WriteLog(LOG_LEVEL_INFO, "EspKnxMQTT ONLINE!");

			// subscribe TOPIC_CMD => command from master
			_MQTTClient.subscribe(TOPIC_CMD);

			// subscribe set command topic
			char set_topic[100];
			snprintf_P(set_topic, sizeof(set_topic), PSTR("%s/#"), TOPIC_DISCOVERY);
			_MQTTClient.subscribe(set_topic);
		}
		else
		{
			char log[250];
			sprintf(log, "failed, rc=%d try again in 5 seconds", (_MQTTClient.state()));
			WriteLog(LOG_LEVEL_DEBUG, log);

			mqttConnected = false;

			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void MQTT_Callback(char *topic, byte *payload, unsigned int length)
{
	//
	char log[200];
	snprintf_P(log, sizeof(log), "Arrived %d byte on topic %s", length, topic);
	WriteLog(LOG_LEVEL_DEBUG, log);

	String ComandoRicevuto = "";
	for (int i = 0; i < (int)length; i++)
		ComandoRicevuto = ComandoRicevuto + (char)payload[i];

	WriteLog(LOG_LEVEL_DEBUG, ComandoRicevuto.c_str());

	if (strcmp(topic, TOPIC_CMD) == 0)
	{
		if (ComandoRicevuto == String("MQTTDiscoveryOn"))
			mqttDiscoveryEnabled = true;
		else if (ComandoRicevuto == String("MQTTDiscoveryOff"))
			mqttDiscoveryEnabled = false;
	}
	else if (strcmp(topic, "pir1Status") == 0)
	{
	}

	//sprintf(Log, "Ricevuto Comando <%s>", string2char(ComandoRicevuto));
	//client.publish(trace_topic, Log);
	//	GestioneComando(ComandoRicevuto);
}

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
	WiFi_Startup();

	_MQTTClient.setServer(MQTT_SERVER, MQTT_PORT);
	_MQTTClient.setCallback(MQTT_Callback);
}

void loop()
{
	// code life cycle

	// in case of disconnect
	if (!_MQTTClient.connected())
		MQTT_Reconnect();

	// loop mqtt client
	_MQTTClient.loop();

	if (!KnxGateInit)
	{
		KNX_Init();
		KnxGateInit = true;
	}
	else
	{

		// millisecondi attuali
		unsigned long currentMillis = millis();

		if (currentMillis - previousMillis >= LOOP_INTERVAL_MILLISEC)
		{
			KNX_ReadBus();

			/*
			if (CmdExec != CMD_NO_CDM)
			{
				KyoUnit_GesCmdExec(CmdExec);
				CmdExec = CMD_NO_CDM;
			}
			*/

			// salvo ultima esecuzione
			previousMillis = currentMillis;
		}
	}
}
