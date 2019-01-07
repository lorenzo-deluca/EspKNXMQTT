/*
  const.h - ESPKNXMQTT

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

// configuration version
#define CONFIG_VERSION "CFG01"

// where in EEPROM
#define CONFIG_START 32

typedef struct
{
    char version[6]; // detect if setting actually are written

    bool networkConfig;
    bool mqttEnable;

    int serialLogLevel;
    int mqttLogLevel;

} syscfg_type;

enum LoggingLevels
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_DEBUG_MORE,
    LOG_LEVEL_ALL
};

// WIFI
#define wifi_ssid "WiFiLDL_iOT"
#define wifi_password "pswiOT2017!"

// MQTT
#define MQTT_SERVER "192.168.8.2"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define mqtt_device "EspKnxMQTT"

// MQTT Topics
#define TOPIC_LOG "knxhome/log"
#define TOPIC_STATE "knxhome/state"
#define TOPIC_BUS "knxhome/bus"
#define TOPIC_CMD "knxhome/cmd"

#define TOPIC_DISCOVERY "homeassistant/switch/%s/config"

#define D_ASTERIX "********"
#define WIFI_HOSTNAME "%s-%04d" // Expands to <MQTT_TOPIC>-<last 4 decimal chars of MAC address>
#define D_WEBLINK "https://bit.ly/tasmota"
#define VERSION 0x06020100

#define D_PROGRAMNAME "EspKnxMQTT"
#define D_AUTHOR "Lorenzo De Luca"