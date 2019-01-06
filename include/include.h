/*
  include.h - ESPKNXMQTT

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

#include <Arduino.h>

extern "C"
{
#include "user_interface.h"
}

#include <const.h>

//
// PROTOTYPES COMMON FUNCTIONS
//

// configuration manager
extern int Configuration_Load();
extern syscfg_type SYSCONFIG;
extern bool wiFiConnected;

// logger manager
extern void WriteLog(int msgLevel, char *msgLog);
extern void MQTT_Publish(char *topic, char * payload);

// WiFi manager
void WiFi_Startup();

// KNX manager
extern void KNX_Init();