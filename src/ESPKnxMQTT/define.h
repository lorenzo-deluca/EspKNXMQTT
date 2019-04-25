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
#define CONFIG_VERSION "CFG00"

#define IP_ADDRESS_LEN 16
#define KNX_DEVICE_ADDRESS_SIZE 5
#define KNX_DEVICE_DESCRIPTION  15

#define MAX_KNX_DEVICES 64

// command list struct
#define CMD_LIST_IMPLEMENTATION FIFO
#define CMD_LIST_SIZE           32

enum LoggingLevels
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_DEBUG_MORE,
    LOG_LEVEL_ALL
};

enum deviceType
{
    TYPE_SWITCH,
    TYPE_SCENE,
    TYPE_RELAY,
    TYPE_DIMMER
};

enum commandType
{
    CMD_TYPE_ON,
    CMD_TYPE_OFF,
    CMD_TYPE_UP,
    CMD_TYPE_DOWN,
    CMD_TYPE_SCENE
};

struct _knxDevice
{
    bool    Configured;
    char    KnxAddress[KNX_DEVICE_ADDRESS_SIZE];
    int     Type;
    int     LastStatus;
    int     RelayAddr;
    char    Description[KNX_DEVICE_DESCRIPTION];
};

struct _syscfgType
{
    // configuration version
    char version[6];

    bool mqttBusTrace;
    bool mqttUpdateEnable;
    bool webServerEnable;

    int serialLogLevel;
    int mqttLogLevel;
    
    // mqtt configuration
    char mqtt_server[20];
    char mqtt_port[6];
    char mqtt_user[8];
    char mqtt_password[10];
    char mqtt_topic_prefix[10];
    
    // optional static ip configuration
    bool static_cfg;
   // char static_ip[IP_ADDRESS_LEN];
   // char static_mask[IP_ADDRESS_LEN];
   // char static_gtw[IP_ADDRESS_LEN];

   _knxDevice KnxDevices[MAX_KNX_DEVICES];
};

struct _runtimeType
{
    bool Configured;

    bool mqttConnected;
    bool wiFiConnected;
    
    bool KnxGateInit;
    bool KnxDiscoveryEnabled;
    bool MQTTDiscoveryEnabled;
};

struct _command
{
    char knxDeviceAddress[KNX_DEVICE_ADDRESS_SIZE];
    int cmdType;
};

// WIFI
#ifndef APSSID
#define APSSID "ESPKnxMQTT_ap"
#define APPSK "12345678"
#endif

// MQTT
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_DEVICE_ID "EspKnxMQTT"

// MQTT Topics
#define TOPIC_PREFIX "knxhome"
#define TOPIC_LOG "knxhome/log"
#define TOPIC_STATE "knxhome/state"
#define TOPIC_BUS "knxhome/bus"
#define TOPIC_CMD "knxhome/cmd"

// Discovery
#define TOPIC_SWITCH_SET "knxhome/switch/set"
#define TOPIC_SWITCH_STATE "knxhome/switch/state"
#define TOPIC_DISCOVERY "homeassistant/switch/%s/config"

#define LOOP_INTERVAL_MILLISEC 120

#define D_PROGRAMNAME "EspKnxMQTT"
#define D_AUTHOR "Lorenzo De Luca"


#endif