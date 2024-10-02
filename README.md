# OBD2 to MQTT for Home Assistant

## What you need?

* an installed Home Assistant with Mosquitto Broker
* an installed PlatformIO IDE or other IDE
* a ESP32 with [SIM800L](https://de.aliexpress.com/item/33045221960.html)
  or [A7670](https://de.aliexpress.com/item/1005006477044118.html)
    * (optional) [RP-SMA to IPX cable](https://www.amazon.de/dp/B0B9RXDLNN)
    * (optional) [Antenna](https://www.amazon.de/dp/B0B2DCXL5N) or other (work's for me)
    * (optional) a 3D Printer for the [case](3d-files)
* a [ELM327 OBD Bluetooth Adapter](https://de.aliexpress.com/item/1005005775562398.html) or any other
* a SIM Card - i'm use one
  from [fraenk](https://fraenk.page.link/?link=https%3A%2F%2Ffraenk.de%2Fdeeplink%2Fmgm%3FfriendCode%3DRENA45&apn=de.congstar.fraenk&amv=1040000&imv=1.4&isi=1493980266&ibi=de.congstar.fraenk&ius=fraenk&ofl=https%3A%2F%2Ffraenk.de)
* and the most important thing, a car

## Getting started

* change [src/privates.h](src/privates.h) to your needs
    * let __OBD_DEV_MODE__ on true for now
    * set Bluetooth adapter MAC on __OBD_ADP_MAC__
        * or let it empty to discover adapter, by default an adapter with name OBDII is used
    * define your device (see [platformio.ini](platformio.ini))
    * SIM Pin
    * APN
    * MQTT Broker settings
    * (optional) Wi-Fi - only used for development
* and compile and upload to device

## Supported Sensors

This sensors only available if your car support.

* Ambient Temperature
* Battery Voltage
* Calculated average speed
* Calculated consumption
* Calculated consumption per 100km
* Calculated driven distance
* Check Engine Light
* Engine Coolant Temperature
* Engine Load
* Engine Running
* Fuel Level
* Fuel Rate
* Intake Air Temperature
* Kilometer per Hour
* Mass Air Flow
* Oil Temperature
* Pedal Position
* Rounds per minute
* Throttle

Diagnostic Output:

* CPU Temperature (ESP)
* Free Memory (ESP)
* GPS Location (only for A76xx)
* GSM Location
* Signal Quality
* Uptime

## ToDos

* support more PIDs (odometer,...)
* get vehicle information (working)