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

#ifndef _MY_HEADER_H
#define _MY_HEADER_H


// configuration version
#define CONFIG_VERSION "CFG01"

// where in EEPROM
#define CONFIG_START 32

struct _syscfgType
{
    // configuration version
    char version[6];

    bool networkConfig;
    bool mqttEnable;
    bool mqttUpdateEnable;

    int serialLogLevel;
    int mqttLogLevel;
};

struct _runtimeType
{
    bool mqttConnected;
    bool wiFiConnected;
    bool KnxGateInit;
    bool mqttDiscoveryEnabled;
};

enum LoggingLevels
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_DEBUG_MORE,
    LOG_LEVEL_ALL
};

enum commandType
{
    CMD_TYPE_ON,
    CMD_TYPE_OFF,
    CMD_TYPE_UP,
    CMD_TYPE_DOWN
};

// command list struct
#define CMD_LIST_IMPLEMENTATION FIFO
#define CMD_LIST_SIZE 20

#define KNX_DEVICE_ADDRESS_SIZE 5

struct _command
{
    char knxDeviceAddress[KNX_DEVICE_ADDRESS_SIZE];
    int cmdType;
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
#define TOPIC_PREFIX ""
#define TOPIC_LOG "knxhome/log"
#define TOPIC_STATE "knxhome/state"
#define TOPIC_BUS "knxhome/bus"
#define TOPIC_CMD "knxhome/cmd"

// Discovery
#define TOPIC_SWITCH_SET "knxhome/switch/set"
#define TOPIC_SWITCH_STATE "knxhome/switch/state"
#define TOPIC_DISCOVERY "homeassistant/switch/%s/config"

#define LOOP_INTERVAL_MILLISEC 250

#define D_PROGRAMNAME "EspKnxMQTT"
#define D_AUTHOR "Lorenzo De Luca"

#endif