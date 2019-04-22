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

	// clear EEPROM before write
	for (size_t i = 0 ; i < EEPROM.length() ; i++)
		EEPROM.write(i, 0);

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
void saveConfigCallback () 
{  	
	WriteLog(LOG_LEVEL_INFO, "saveConfigCallback");

	strcpy(SYSCONFIG.mqtt_server, cfg_mqtt_server.getValue());
	strcpy(SYSCONFIG.mqtt_port, cfg_mqtt_port.getValue());
	strcpy(SYSCONFIG.mqtt_user, cfg_mqtt_user.getValue());
	strcpy(SYSCONFIG.mqtt_password, cfg_mqtt_password.getValue());

	SYSCONFIG.static_cfg = 
		(strlen(cfg_static_ip.getValue()) > 0 && strlen(cfg_static_mask.getValue()) > 0 && strlen(cfg_static_gtw.getValue()) > 0);

 	if(SYSCONFIG.static_cfg)
	{
		strcpy(SYSCONFIG.static_ip, cfg_static_ip.getValue());
		strcpy(SYSCONFIG.static_mask, cfg_static_mask.getValue());
		strcpy(SYSCONFIG.static_gtw, cfg_static_gtw.getValue());
	}
	
	storeStruct(&SYSCONFIG, sizeof(SYSCONFIG));

	Serial.println("saveConfigCallback - end");

	// restart ESP after save configuration

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
		//settings.static_ip[IP_ADDRESS_LEN - 1] = 0;
		//settings.static_mask[IP_ADDRESS_LEN - 1] = 0;
		//settings.static_gtw[IP_ADDRESS_LEN- 1] = 0;

		// Config OK, copy to SYSCONFIG
    		memcpy(&SYSCONFIG, &settings, sizeof(SYSCONFIG));
		return true;
	}
	else
	{
		// no valid Configuration Find !
		return false;
	}
}

bool Configuration_Load()
{
	if(!Configuration_Read())
	{
		WriteLog(LOG_LEVEL_ERROR, "Configuration_Load NOK");
		return false;
	}

	WiFiManager_SetParameters();

	WriteLog(LOG_LEVEL_INFO, "Configuration_Load OK");
	return true;
}

void WiFiManager_SetParameters()
{
	// set reading parameters into WifiManager parameters
	new (&cfg_mqtt_server) 		WiFiManagerParameter("mqtt_server", "mqtt server", 	SYSCONFIG.mqtt_server, 		20);
	new (&cfg_mqtt_port)		WiFiManagerParameter("mqtt_port", 	"mqtt port", 		SYSCONFIG.mqtt_port, 		6);
	new (&cfg_mqtt_user)		WiFiManagerParameter("mqtt_user", 	"mqtt user", 		SYSCONFIG.mqtt_user, 		8);
	new (&cfg_mqtt_password)		WiFiManagerParameter("mqtt_password", "mqtt password", SYSCONFIG.mqtt_password, 	10);

	new (&cfg_static_ip)		WiFiManagerParameter("static_ip", 	"static ip", 		SYSCONFIG.static_ip, 		IP_ADDRESS_LEN);
	new (&cfg_static_mask)		WiFiManagerParameter("static_mask", "static mask",	SYSCONFIG.static_mask,	 	IP_ADDRESS_LEN);
	new (&cfg_static_gtw)		WiFiManagerParameter("static_gtw", "static gtw", 		SYSCONFIG.static_gtw, 		IP_ADDRESS_LEN);

	if(SYSCONFIG.static_cfg)
	{
		IPAddress _ip, _mask, _gtw;
		_ip.fromString(SYSCONFIG.static_ip);
		_mask.fromString(SYSCONFIG.static_mask);
		_gtw.fromString(SYSCONFIG.static_gtw);

		wifiManager.setSTAStaticIPConfig(_ip, _gtw, _mask);
	}
}

void WiFiManager_Setup()
{
	wifiManager.addParameter(&cfg_mqtt_server);
	wifiManager.addParameter(&cfg_mqtt_port);
	wifiManager.addParameter(&cfg_mqtt_user);
	wifiManager.addParameter(&cfg_mqtt_password);

	wifiManager.addParameter(&cfg_static_ip);
	wifiManager.addParameter(&cfg_static_mask);
	wifiManager.addParameter(&cfg_static_gtw);

	wifiManager.setBreakAfterConfig(true);

	wifiManager.setSaveConfigCallback(saveConfigCallback);
}