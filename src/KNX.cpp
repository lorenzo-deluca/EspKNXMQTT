/*
  KNX.cpp - ESPKNXMQTT

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

#include <include.h>

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

String KNX_Send(char *msg, bool waitresponse = false, bool trace = true, int millsdelay = 5)
{
    Serial.flush();
    for (int i = 0; i < strlen(msg); i++)
    {
        Serial.write(msg[i]);
        delayMicroseconds(100);
    }

    if (SYSCONFIG.mqttEnable && trace)
    {
        char log[250];
        snprintf_P(log, sizeof(log), "TX >> [%s]", msg);
        MQTT_Publish(TOPIC_BUS, log);
    }

    delay(millsdelay);
    yield();

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

void KNX_Init()
{
    // set led lamps
    KNX_Send("@");

    // set led lampùs low-freq (client mode)
    KNX_Send((char)0xF0);

    // Set KNXgate hex mode
    KNX_Send("@MX", true);

    KNX_Send("@A0", true);

    KNX_Send("@B0", true);

    KNX_Send("@F2", true);

    KNX_Send("@c", true);

    // clear SCSgate/KNXgate buffer
    KNX_Send("@b", true);

    Serial.setTimeout(10); // timeout is 10mS
}

void KNX_ReadBus()
{
    String knxRead;
    knxRead = KNX_Send("@r", true, false);

    if (knxRead.length() == 20) // 09 B4 100B 0B25 E100 81 1E
    {
        if (knxRead[2] == 'B' && knxRead[3] == '4')
        {
            char devKnxAddress[5];
            devKnxAddress[0] = knxRead[8];
            devKnxAddress[1] = knxRead[9];
            devKnxAddress[2] = knxRead[10];
            devKnxAddress[3] = knxRead[11];
            devKnxAddress[4] = 0;

            if (mqttDiscoveryOn)
            {
                char mqtt_data[200];
                char mqtt_topic[200];
                sprintf(mqtt_topic, TOPIC_DISCOVERY, devKnxAddress);

                snprintf_P(mqtt_data, sizeof(mqtt_data), "{\"name\": \"%s\", \"command_topic\": \"knxhome/switch/%s/set\", \"state_topic\":\"knxhome/switch/%s/state\"}",
                        devKnxAddress, devKnxAddress, devKnxAddress);
                MQTT_Publish(mqtt_topic, mqtt_data);

                WriteLog(LOG_LEVEL_DEBUG, mqtt_topic);
                WriteLog(LOG_LEVEL_DEBUG, mqtt_data);
            }
        }
    }
}