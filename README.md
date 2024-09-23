#  OBD2 to MQTT for Home Assistant <a href="https://www.buymeacoffee.com/adlerre" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 30px !important;width: 108px !important;" ></a>

> __!!!WARNING!!!__
>
> I'm not a C/C++ programmer therefore i'm not responsible for any headaches nor other WTF moments. ;)

## What you need?

* a installed Home Assistant with Mosquitto Broker
* a installed PlatformIO IDE or other IDE 
* a ESP32 with [SIM800L](https://de.aliexpress.com/item/33045221960.html) or [A7670](https://de.aliexpress.com/item/1005006477044118.html)
  * (optional) [RP-SMA to IPX cable](https://www.amazon.de/dp/B0B9RXDLNN)
  * (optional) [Antenna](https://www.amazon.de/dp/B0B2DCXL5N) or other (work's for me)
  * (optional) a 3D Printer for [case](3d-files)
* a [ELM327 OBD Bluetooth Adapter](https://de.aliexpress.com/item/1005005775562398.html) or any other
* a SIM Card - i'm use one from [fraenk](https://fraenk.page.link/?link=https%3A%2F%2Ffraenk.de%2Fdeeplink%2Fmgm%3FfriendCode%3DRENA45&apn=de.congstar.fraenk&amv=1040000&imv=1.4&isi=1493980266&ibi=de.congstar.fraenk&ius=fraenk&ofl=https%3A%2F%2Ffraenk.de)
* and the most important thing, a car

## Getting started

* change [src/privates.h](src/privates.h) to your needs
  * let __OBD_DEV_MODE__ on true for now
  * set Bluetooth adapter MAC on __OBD_ADP_MAC__
    * or let it empty to discover adapter, by default a adapter with name OBDII is used
  * define your device
  * SIM Pin
  * APN
  * MQTT Broker settings
  * (optional) Wifi - only used for development
* and compile and upload to device

## ToDos

* better consumption/distance calculation
* support more PIDs (odometer,...)
* get vehicle informations (working)