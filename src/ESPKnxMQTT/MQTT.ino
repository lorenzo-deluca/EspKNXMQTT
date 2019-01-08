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

String getValue(String data, char separator, int index)
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
        if (_MQTTClient.connect(mqtt_device, MQTT_USER, MQTT_PASSWORD))
        {
            WriteLog(LOG_LEVEL_DEBUG, "connected");
            RUNTIME.mqttConnected = true;

            WriteLog(LOG_LEVEL_INFO, "EspKnxMQTT ONLINE!");

            // subscribe TOPIC_CMD => command from master
            _MQTTClient.subscribe(TOPIC_CMD);

            // subscribe set command topic
            char set_topic[100];
            snprintf_P(set_topic, sizeof(set_topic), PSTR("%s/#"), TOPIC_SWITCH_SET);
            _MQTTClient.subscribe(set_topic);
        }
        else
        {
            char log[250];
            sprintf(log, "failed, rc=%d try again in 5 seconds", (_MQTTClient.state()));
            WriteLog(LOG_LEVEL_DEBUG, log);

            RUNTIME.mqttConnected = false;

            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void MQTT_Callback(char *topic, byte *payload, unsigned int length)
{
    //
    //char log[200];
    //	snprintf_P(log, sizeof(log), "Arrived %d byte on topic %s", length, topic);
    //	WriteLog(LOG_LEVEL_DEBUG, log);

    String received_payload = "";
    for (int i = 0; i < (int)length; i++)
        received_payload = received_payload + (char)payload[i];

    String received_topic = "";
    received_topic = String(topic);

    char log[200];
    snprintf_P(log, sizeof(log), "Topic %s - payload  %s", received_topic.c_str(), received_payload.c_str());
    WriteLog(LOG_LEVEL_DEBUG, log);

    if (received_topic == String(TOPIC_CMD))
    {
        if (received_payload == String("MQTTDiscoveryOn"))
        {
            WriteLog(LOG_LEVEL_INFO, "mqttDiscoveryEnabled = true");
            RUNTIME.mqttDiscoveryEnabled = true;
        }
        else if (received_payload == String("MQTTDiscoveryOff"))
        {
            WriteLog(LOG_LEVEL_INFO, "mqttDiscoveryEnabled = false");
            RUNTIME.mqttDiscoveryEnabled = false;
        }
    }
    else if (received_topic.indexOf(TOPIC_SWITCH_SET) == 0)
    {
        _command receivedCommand;
        if (received_payload == String("ON"))
            receivedCommand.cmdType = CMD_TYPE_ON;
        else
            receivedCommand.cmdType = CMD_TYPE_OFF;

        // knx device address request
        String value1 = getValue(received_topic, '/', 3);

        // copy knx device address
        memcpy(receivedCommand.knxDeviceAddress, value1.c_str(), 4);
        receivedCommand.knxDeviceAddress[KNX_DEVICE_ADDRESS_SIZE - 1] = 0;

        // push in command queue
        commandList.push(&receivedCommand);
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
