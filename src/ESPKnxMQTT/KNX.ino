/*
  KNX.ino - ESPKNXMQTT

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

#include "Arduino.h"

#include "./define.h"

#define INNERWAIT 80  // inner loop delay
#define OUTERWAIT 100 // outer loop delay

void bytes_to_char(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
        buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
    }
    buffer[len * 2] = '\0';
}

String KNX_Receive()
{
    byte replyBuffer[120];
    unsigned char replyLen;

    replyLen = 0;
    if (Serial.available())
    {
        while (Serial.available() && (replyLen < 255))
        {
            while (Serial.available() && (replyLen < 255))
            {
                replyBuffer[replyLen++] = Serial.read(); // receive from serial USB
                delayMicroseconds(INNERWAIT);
            }
            delayMicroseconds(OUTERWAIT);
        }
    }

    if (replyLen > 1)
    {
        char res[240];
        bytes_to_char(replyBuffer, replyLen, res);

        if (SYSCONFIG.mqttEnable)
        {
            char log[250];
            snprintf_P(log, sizeof(log), "RX << [%s]", res);
            MQTT_Publish(TOPIC_BUS, log);
        }

        return res;
    }

    return "";
}

String KNX_Send(String msg, bool waitresponse = false, bool trace = true, int millsdelay = 50)
{
    Serial.flush();

    for (unsigned int i = 0; i < msg.length(); i++)
    {
        Serial.write(msg[i]);
        delayMicroseconds(100);
    }
    //Serial.write(msg.c_str());

    if (SYSCONFIG.mqttEnable && trace)
    {
        char log[250];
        snprintf_P(log, sizeof(log), "TX >> [%s]", msg.c_str());
        MQTT_Publish(TOPIC_BUS, log);
    }

    //wait for response
    delay(millsdelay);

    if (waitresponse)
    {
        return KNX_Receive();
    }
    else
        return "";
}

bool KNX_Send(char ch, bool waitresponse = false, bool trace = true, int millsdelay = 5)
{
    char *p = &ch;
    return KNX_Send(p, waitresponse, trace, millsdelay);
}

bool KNX_Init()
{
    //
    KNX_Send("@q", true);

    // set led lamps
    KNX_Send("@");

    // set led lamps low-freq (client mode)
    KNX_Send((char)0xF0);

    // Set KNXgate hex mode
    KNX_Send("@MX");

    // clear @A filter
    KNX_Send("@A0");

    // clear @B filter
    KNX_Send("@B0");

    // filter ACK messages
    KNX_Send("@F2");

    //
    KNX_Send("@c");

    // clear SCSgate/KNXgate buffer
    KNX_Send("@b");

    Serial.setTimeout(10); // timeout is 10mS

    return true;
}

bool KNX_CheckTelegram(String knxTelegram)
{
    return true;
}

byte KNX_CRC(byte telegram[], int length)
{
    byte crc = 0xFF;

    for (int i = 0; i < length; i++)
        crc = crc ^ (byte)telegram[i];

    return crc;
}

byte KNX_ConvertByte(char cByte_1, char cByte_2)
{
    byte bByte;

    char addr[3];
    addr[0] = cByte_1;
    addr[1] = cByte_2;
    addr[2] = 0;
    bByte = byte(strtol(addr, NULL, 16));

    return bByte;
}

bool KNX_SendCommand(byte command[], int len)
{

    return true;
}

bool KNX_ExeCommand(char knxDeviceAddress[], int cmdType)
{
    char log[250];
    snprintf_P(log, sizeof(log), "KNX_ExeCommand - type [%d] to Device [%s]", cmdType, knxDeviceAddress);
    WriteLog(LOG_LEVEL_INFO, log);

    byte cmd[20];

    // B4 10 0B 0B 25 E1 00 81 1E
    cmd[0] = 0xB4;
    cmd[1] = 0x10;
    cmd[2] = 0x0B;

    // destination device address
    cmd[3] = KNX_ConvertByte(knxDeviceAddress[0], knxDeviceAddress[1]);
    cmd[4] = KNX_ConvertByte(knxDeviceAddress[2], knxDeviceAddress[3]);

    cmd[5] = 0xE1;
    cmd[6] = 0x00;

    if (cmdType == CMD_TYPE_ON)
        cmd[7] = 0x81;
    else
        cmd[7] = 0x80;

    // calculate telegram CRC
    cmd[8] = KNX_CRC(cmd, 8);
    cmd[9] = 0;

    Serial.write('@');
    delayMicroseconds(100);
    Serial.write('W');
    delayMicroseconds(100);
    Serial.write(0x09);
    delayMicroseconds(100);

    for (unsigned int i = 0; i <= 9; i++)
    {
        Serial.write(cmd[i]);
        delayMicroseconds(100);
    }

    Serial.flush();

    if (SYSCONFIG.mqttEnable)
    {
        char msg[100];
        bytes_to_char(cmd, 9, msg);
        char log[200];
        snprintf_P(log, sizeof(log), "TX >> [%s]", msg);
        MQTT_Publish(TOPIC_BUS, log);
    }

    char mqtt_data[200];
    char mqtt_topic[200];

    snprintf_P(mqtt_topic, sizeof(mqtt_topic), PSTR("%s/%s"), TOPIC_SWITCH_STATE, knxDeviceAddress);

    if (cmdType == CMD_TYPE_ON)
        snprintf_P(mqtt_data, sizeof(mqtt_data), "ON");
    else
        snprintf_P(mqtt_data, sizeof(mqtt_data), "OFF");

    MQTT_Publish(mqtt_topic, mqtt_data);

    delay(100);

    return true;
}

void KNX_ReadBus()
{
    String knxRead;
    knxRead = KNX_Send("@r", true, false);

    /*
        KNX Example telegram (from Vimar By-Me bus)
        
        // group 0B25 ON
        09 B4 100B 0B25 E100 81 1E => 
            09  = not implemented
            B4  = VIMAR Prefix
            100B = Group Source address
        --> 0B25 = Group Destination Address <-- 
            E100 = not implemented
            81  = ON COMMAND
            1E = CRC

        // activate scene 0F05
        0A B4 1007 0F05 E200 80 05 31
    */

    // exclude first byte
    knxRead = knxRead.substring(2);

    // check telegram length
    if (knxRead.length() == 18 || knxRead.length() == 20)
    {
        // check telegram header
        if (knxRead[0] == 'B' && knxRead[1] == '4')
        {
            // get Knx device address
            char knxDeviceAddress[5];
            knxDeviceAddress[0] = knxRead[6];
            knxDeviceAddress[1] = knxRead[7];
            knxDeviceAddress[2] = knxRead[8];
            knxDeviceAddress[3] = knxRead[9];
            knxDeviceAddress[4] = 0;

            char mqtt_data[200];
            char mqtt_topic[100];

            if (RUNTIME.mqttDiscoveryEnabled)
            {
                snprintf_P(mqtt_topic, sizeof(mqtt_topic), TOPIC_DISCOVERY, knxDeviceAddress);

                snprintf_P(mqtt_data, sizeof(mqtt_data), "{\"name\": \"%s\", \"command_topic\": \"%s/%s\", \"state_topic\":\"%s/%s\"}",
                           knxDeviceAddress,
                           TOPIC_SWITCH_SET,
                           knxDeviceAddress,
                           TOPIC_SWITCH_STATE,
                           knxDeviceAddress);
            }
            else
            {
                snprintf_P(mqtt_topic, sizeof(mqtt_topic), PSTR("%s/%s"), TOPIC_SWITCH_STATE, knxDeviceAddress);

                if (knxRead[14] == '8' && knxRead[15] == '1')
                    snprintf_P(mqtt_data, sizeof(mqtt_data), "ON");
                else
                    snprintf_P(mqtt_data, sizeof(mqtt_data), "OFF");
            }

            MQTT_Publish(mqtt_topic, mqtt_data);
        }
    }
}

void KNX_Loop()
{

    // check
    for (int i = 0; i < commandList.getCount(); i++)
    {
        _command cmd;
        commandList.pop(&cmd);

        KNX_ExeCommand(cmd.knxDeviceAddress, cmd.cmdType);
    }

    // read bus
    KNX_ReadBus();
}