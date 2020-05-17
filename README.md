# ESPKnxMQTT
Bridge for Vimar Byme BUS

## MQTT Topics

Topic  | Function
------------- | -------------
`knxhome/cmd`  | Commands
`knxhome/log` | Application Log
`knxhome/bus` | By Me Bus
`knxhome/switch/set` | MQTT Discovery - Set State
`knxhome/switch/state` | MQTT Discovery - Get State
`homeassistant/switch/<KnxAddress>/config` | MQTT Discovery - Config

#### CONFIG PAYLOAD
```json
{
    "name": "<KNXAddress>", 
    "command_topic": "{BaseTopic}/switch/<KNXAddress>/set", 
    "state_topic":"{BaseTopic}/switch/<KNXAddress>/state"
}
```

## MQTT Commands

Publish **Command** in `knxhome/cmd/<Command>` Topic

Topic  | Payload
------------- | -------------
`MQTTDiscovery` | `ON` - `OFF` Enable - Disable MQTT Discovery
`KNXDiscovery`  | `ON` - `OFF` Enable - Disable KNX Discovery 
`MQTTBusTrace`  | `ON` - `OFF` Enable - Disable Bus Trace on `knxhome/bus` topic
`KNXDeviceConfig/<KnxAddress>` | `<Description>`,`<Type>`, `<RelayAddress>`, response in `knxhome/log`
`PrintKNXDevices` | Print KNX Devices in `knxhome/log`
`MQTTUpdateDiscovery` | Repulish in `homeassistant/switch/%s/config` all configuration 
`Save` | Save current configuration and restart
`Restart` | Restart without save
