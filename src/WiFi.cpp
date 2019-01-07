/*
  WiFi.cpp - ESPKNXMQTT

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

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
#include <include.h>

String IpAddress2String(const IPAddress &ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

char *string2char(String command)
{
    if (command.length() != 0)
    {
        char *p = const_cast<char *>(command.c_str());
        return p;
    }
    else
        return NULL;
}

void WiFi_Startup()
{
    delay(10);

    char Log[250];

    snprintf_P(Log, sizeof(Log), "Connecting to %s", wifi_ssid);
    WriteLog(LOG_LEVEL_DEBUG, Log);

    WiFi.disconnect();
    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        WriteLog(LOG_LEVEL_DEBUG, ".");
    }

    randomSeed(micros());

    WriteLog(LOG_LEVEL_DEBUG, "WiFi connected - IPv4 =");
    WriteLog(LOG_LEVEL_DEBUG, string2char(IpAddress2String(WiFi.localIP())));

    wiFiConnected = true;
}