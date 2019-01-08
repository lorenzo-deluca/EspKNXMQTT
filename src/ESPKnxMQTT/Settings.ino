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

extern _syscfgType SYSCONFIG;

// load whats in EEPROM in to the local CONFIGURATION if it is a valid setting
int Configuration_Load()
{
	// is it correct?
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
		EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
		EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2] &&
		EEPROM.read(CONFIG_START + 3) == CONFIG_VERSION[3] &&
		EEPROM.read(CONFIG_START + 4) == CONFIG_VERSION[4])
	{
		// load (overwrite) the local configuration struct
		for (unsigned int i = 0; i < sizeof(SYSCONFIG); i++)
		{
			*((char *)&SYSCONFIG + i) = EEPROM.read(CONFIG_START + i);
		}
		return 1; // return 1 if config loaded
	}
	return 0; // return 0 if config NOT loaded
}


// save the CONFIGURATION in to EEPROM
void Configuration_Save()
{
	for (unsigned int i = 0; i < sizeof(SYSCONFIG); i++)
		EEPROM.write(CONFIG_START + i, *((char *)&SYSCONFIG + i));
}
