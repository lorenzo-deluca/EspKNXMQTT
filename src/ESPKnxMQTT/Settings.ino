/*
  Settings.ino - ESPKNXMQTT

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

//callback notifying us of the need to save config
void saveConfigCallback () {
  	
	WriteLog(LOG_LEVEL_INFO, "saveConfigCallback");

	strcpy(SYSCONFIG.mqtt_server, custom_mqtt_server.getValue());
	strcpy(SYSCONFIG.mqtt_port, custom_mqtt_port.getValue());

	storeStruct(&SYSCONFIG, sizeof(SYSCONFIG));
		
	Serial.println("saveConfigCallback - end");

	delay(1000);
	ESP.restart();
}

// load whats in EEPROM in to the local CONFIGURATION if it is a valid setting
bool Configuration_Read() 
{
	_syscfgType settings;

    // Carico nella struttura temporanea
    loadStruct(&settings, sizeof(settings));

	if(memcmp(settings.version, CONFIG_VERSION, 6) == 0) 
	{
        memcpy(&SYSCONFIG, &settings, sizeof(SYSCONFIG));
		return true; // return 1 if config loaded
	}
	else
	{
		return false; // return 0 if config NOT loaded
	}
}

bool Configuration_Load()
{
	if(!Configuration_Read())
	{
		WriteLog(LOG_LEVEL_ERROR, "LoadConfig NOK");
		return false;
	}

	new (&custom_mqtt_server) WiFiManagerParameter("mqtt_server", "mqtt server", SYSCONFIG.mqtt_server, 40);

	WriteLog(LOG_LEVEL_INFO, "LoadConfig OK");
	return true;
}
