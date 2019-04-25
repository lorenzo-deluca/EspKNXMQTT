/*
  MQTT.ino - ESPKNXMQTT

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

#include "./define.h"

void MQTT_Loop()
{
	// in case of mqtt disconnect
	if (!_MQTTClient.connected())
		MQTT_Reconnect();

	// loop mqtt client
	_MQTTClient.loop();
}

String getValueSeparator(String data, char separator, int index)
{
	int found = 0;
	int strIndex[] = {0, -1};
	int maxIndex = data.length() - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++)
	{
		if (data.charAt(i) == separator || i == maxIndex)
		{
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}

	return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void MQTT_Publish(const char *topic, const char *payload)
{
	if (!RUNTIME.mqttConnected)
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

void MQTT_Reconnect()
{
	// Loop until we're reconnected
	while (!_MQTTClient.connected())
	{
		WriteLog(LOG_LEVEL_DEBUG, "Attempting MQTT connection...");

		// Provo a connettermi al server MQTT
		if (_MQTTClient.connect(MQTT_DEVICE_ID, MQTT_USER, MQTT_PASSWORD))
		{
			RUNTIME.mqttConnected = true;

			WriteLog(LOG_LEVEL_INFO, "%s ONLINE - IP %s", MQTT_DEVICE_ID, WiFi.localIP().toString().c_str());

			char sub_topic[100];
			
			// subscribe TOPIC_CMD => command from master
			snprintf_P(sub_topic, sizeof(sub_topic), PSTR("%s/#"), TOPIC_CMD);
			_MQTTClient.subscribe(sub_topic);

			// subscribe set command topic
			snprintf_P(sub_topic, sizeof(sub_topic), PSTR("%s/#"), TOPIC_SWITCH_SET);
			_MQTTClient.subscribe(sub_topic);
		}
		else
		{
			WriteLog(LOG_LEVEL_DEBUG, "failed, rc=%d try again in 5 seconds", (_MQTTClient.state()));

			RUNTIME.mqttConnected = false;

			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void MQTT_Callback(char *topic, byte *payload, unsigned int length)
{
	String received_payload = "";
	for (int i = 0; i < (int)length; i++)
		received_payload = received_payload + (char)payload[i];

	String received_topic = String(topic);

	WriteLog(LOG_LEVEL_DEBUG, "Topic %s - payload  %s", received_topic.c_str(), received_payload.c_str());

	if (received_topic.indexOf(TOPIC_CMD) == 0)
	{
		MQTT_CmdManager(received_topic, received_payload);
	}
	else if (received_topic.indexOf(TOPIC_SWITCH_SET) == 0)
	{
		_command receivedCommand;
		if (received_payload == String("ON"))
			receivedCommand.cmdType = CMD_TYPE_ON;
		else
			receivedCommand.cmdType = CMD_TYPE_OFF;

		// knx device address request
		String value1 = getValueSeparator(received_topic, '/', 3);

		// copy knx device address
		memcpy(receivedCommand.knxDeviceAddress, value1.c_str(), 4);
		receivedCommand.knxDeviceAddress[KNX_DEVICE_ADDRESS_SIZE - 1] = 0;

		// push in command queue
		commandList.push(&receivedCommand);
	}
}

void MQTT_DiscoveryUpdateStatus(char knxDeviceAddress[], int status)
{
	char mqtt_data[200];
	char mqtt_topic[120];

	snprintf_P(mqtt_topic, sizeof(mqtt_topic), PSTR("%s/%s"), TOPIC_SWITCH_STATE, knxDeviceAddress);

	switch(status)
	{
		case CMD_TYPE_ON:
		case CMD_TYPE_SCENE:
			snprintf_P(mqtt_data, sizeof(mqtt_data), "ON");
			break;
		
		case CMD_TYPE_OFF:
			snprintf_P(mqtt_data, sizeof(mqtt_data), "OFF");
			break;
	}

	MQTT_Publish(mqtt_topic, mqtt_data);
}

void MQTT_DiscoveryConfig(char knxDeviceAddress[], int status)
{
	char mqtt_data[200];
	char mqtt_topic[120];
	char type[20];

	switch(status)
	{
		case CMD_TYPE_SCENE:
		    snprintf_P(type, sizeof(type), "SCENE");
			break;

		case CMD_TYPE_ON:
		case CMD_TYPE_OFF:
			snprintf_P(type, sizeof(type), "GROUP");
			break;

		default:
			snprintf_P(type, sizeof(type), "UNKN");
	}

	snprintf_P(mqtt_topic, sizeof(mqtt_topic), TOPIC_DISCOVERY, knxDeviceAddress);

	snprintf_P(mqtt_data, sizeof(mqtt_data), "{\"name\": \"%s %s\", \"command_topic\": \"%s/%s\", \"state_topic\":\"%s/%s\"}",
				type, 
				knxDeviceAddress,
				TOPIC_SWITCH_SET,
				knxDeviceAddress,
				TOPIC_SWITCH_STATE,
				knxDeviceAddress);

	MQTT_Publish(mqtt_topic, mqtt_data);
}

void MQTT_CmdManager(String topic, String payload)
{
	String command = getValueSeparator(topic, '/', 2);

	if(command == String("MQTTDiscovery"))
	{
		if(payload == String("ON"))
		{
			WriteLog(LOG_LEVEL_INFO, "MQTTDiscoveryEnabled = true");
			RUNTIME.MQTTDiscoveryEnabled = true;
		}
		else
		{
			WriteLog(LOG_LEVEL_INFO, "MQTTDiscoveryEnabled = false");
			RUNTIME.MQTTDiscoveryEnabled = false;
		}
	}
	else if(command == String("KNXDiscovery"))
	{
		if(payload == String("ON"))
		{
			WriteLog(LOG_LEVEL_INFO, "KnxDiscoveryEnabled = true");
			RUNTIME.KnxDiscoveryEnabled = true;
		}
		else
		{
			WriteLog(LOG_LEVEL_INFO, "KnxDiscoveryEnabled = false");
			RUNTIME.KnxDiscoveryEnabled = false;
		}
	}
	else if(command == String("MQTTBusTrace"))
	{
		if(payload == String("ON"))
		{
			WriteLog(LOG_LEVEL_INFO, "mqttBusTrace = true");
			SYSCONFIG.mqttBusTrace = true;
		}
		else
		{
			WriteLog(LOG_LEVEL_INFO, "mqttBusTrace = false");
			SYSCONFIG.mqttBusTrace = false;
		}
	}
	else if(command == String("Save"))
	{
		saveConfiguration();
	}
	else if(command == String("KNXDeviceConfig"))
	{
		String deviceAddr = getValueSeparator(topic, '/', 3);

		// knx device address request
		char knxDeviceAddress[KNX_DEVICE_ADDRESS_SIZE];
		memset(&knxDeviceAddress, 0, KNX_DEVICE_ADDRESS_SIZE);
		memcpy(knxDeviceAddress, &deviceAddr[0], KNX_DEVICE_ADDRESS_SIZE-1);

		KNX_Device_Config(knxDeviceAddress, 
							getValueSeparator(payload, ',', 0), 
							StrToInt(getValueSeparator(payload, ',', 1)), 
							StrToInt(getValueSeparator(payload, ',', 2)));
	}
	else if(command == String("PrintKNXDevices"))
	{
		KNX_PrintDevices();
	}
	else if(command == String("MQTTUpdateDiscovery"))
	{
		KNX_UpdateDiscovery();
	}
}