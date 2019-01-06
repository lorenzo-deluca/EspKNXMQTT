#define D_ASTERIX "********"
#define WIFI_HOSTNAME          "%s-%04d"    // Expands to <MQTT_TOPIC>-<last 4 decimal chars of MAC address>
#define D_WEBLINK          "https://bit.ly/tasmota"
#define VERSION            0x06020100

#define D_PROGRAMNAME      "EspKnxMQTT"
#define D_AUTHOR           "Lorenzo De Luca"


#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


#define WIFI_HOSTNAME          "%s-%04d"    // Expands to <MQTT_TOPIC>-<last 4 decimal chars of MAC address>

#define CONFIG_FILE_SIGN       0xA5         // Configuration file signature
#define CONFIG_FILE_XOR        0x5A         // Configuration file xor (0 = No Xor)
