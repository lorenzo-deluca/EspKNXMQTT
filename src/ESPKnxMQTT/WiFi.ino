/*
  WiFi.ino - ESPKNXMQTT

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

bool WiFi_APMode()
{
    delay(1000);

    Serial.println("Configuring access point...");

    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAPdisconnect();
    WiFi.disconnect();
  
    WiFi.mode(WIFI_AP);
   // WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(softAP_ssid, softAP_password);

    delay(500); // Without delay I've seen the IP address blank
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    return true;
}

bool WiFi_ClientMode()
{
    delay(1000);

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
    //WriteLog(LOG_LEVEL_DEBUG, string2char(IpAddress2String(WiFi.localIP())));
    WriteLog(LOG_LEVEL_DEBUG, WiFi.localIP().toString().c_str());

    /*
    if (!MDNS.begin(myHostname))
    {
        WriteLog(LOG_LEVEL_DEBUG, "Error setting up MDNS responder!");
    }
    else
    {
        WriteLog(LOG_LEVEL_DEBUG, "mDNS responder started");
        // Add service to MDNS-SD
       
    }
*/
    return true;
}
