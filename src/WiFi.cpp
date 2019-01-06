
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