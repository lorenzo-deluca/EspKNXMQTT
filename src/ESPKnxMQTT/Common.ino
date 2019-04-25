/*
  Common.ino - ESPKNXMQTT

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

#include <EEPROM.h>

void storeStruct(void *data_source, size_t size)
{
	EEPROM.begin(size * 2);

	// clear EEPROM before write
	//for (size_t i = 0 ; i < EEPROM.length() ; i++)
	//	EEPROM.write(i, 0);

	for(size_t i = 0; i < size; i++)
	{
		char data = ((char *)data_source)[i];
		EEPROM.write(i, data);
	}

	EEPROM.commit();
}

void loadStruct(void *data_dest, size_t size)
{
	EEPROM.begin(size * 2);
	for(size_t i = 0; i < size; i++)
	{
		char data = EEPROM.read(i);
		((char *)data_dest)[i] = data;
	}
}

/*
void WriteLog(int msgLevel, const char *msgLog)
{
	if (SYSCONFIG.serialLogLevel >= msgLevel)
		Serial.println(msgLog);

	if (SYSCONFIG.mqttLogLevel >= msgLevel)
	{
		char log[250];
		snprintf_P(log, sizeof(log), "%lu - %s", millis(), msgLog);
		MQTT_Publish(TOPIC_LOG, log);
	}
}
*/

void WriteLog_Internal(int msgLevel, const char *msgLog)
{
	if (SYSCONFIG.serialLogLevel >= msgLevel)
		Serial.println(msgLog);

	if (SYSCONFIG.mqttLogLevel >= msgLevel)
	{
		char log[250];
		snprintf_P(log, sizeof(log), "%lu - %s", millis(), msgLog);
		MQTT_Publish(TOPIC_LOG, log);
	}
}

void WriteLog(int msgLevel, const char * str, ...)
{
	char msgLog[150];

	// componing variable message
 	va_list arglist;
    va_start(arglist, str);
	vsnprintf_P(msgLog, sizeof(msgLog), str, arglist);
	va_end(arglist);

	// send log
	WriteLog_Internal(msgLevel, msgLog);
}

int StrToInt(String str, int def = -1)
{
	int ret = str.toInt();
	if (ret < 0)
		ret = def;

	return ret;
}