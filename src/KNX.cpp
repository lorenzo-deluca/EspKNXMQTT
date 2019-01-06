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

bool KNX_Send(char *msg, bool waitresponse = false, int millidelay = 50)
{
    Serial.flush();
    Serial.write(msg);

    delay(millidelay);
    yield();

    MQTT_Publish(TOPIC_BUS, msg);
}

void KNX_Receive()
{
    char replyBuffer[255];
    unsigned char replyLen;

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
        replyBuffer[replyLen] = 0;
    }
}

void KNX_Init()
{
    KNX_Send("q");
}
