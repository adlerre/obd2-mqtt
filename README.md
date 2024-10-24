# OBD2 to MQTT for Home Assistant

## What you need?

* an installed Home Assistant with Mosquitto Broker
* (optional) an installed PlatformIO
* (optional) an installed NodeJS and NPM
* a ESP32 with [SIM800L](https://de.aliexpress.com/item/33045221960.html)
  or [A7670](https://de.aliexpress.com/item/1005006477044118.html)
    * (optional) [RP-SMA to IPX cable](https://www.amazon.de/dp/B0B9RXDLNN)
    * (optional) [Antenna](https://www.amazon.de/dp/B0B2DCXL5N) (work's for me) or other
    * (optional) a 3D Printer for the [case](3d-files)
* a [ELM327 OBD Bluetooth Adapter](https://de.aliexpress.com/item/1005005775562398.html) or any other
* a SIM Card - i use one
  from [fraenk](https://fraenk.page.link/?link=https%3A%2F%2Ffraenk.de%2Fdeeplink%2Fmgm%3FfriendCode%3DRENA45&apn=de.congstar.fraenk&amv=1040000&imv=1.4&isi=1493980266&ibi=de.congstar.fraenk&ius=fraenk&ofl=https%3A%2F%2Ffraenk.de)
* and the most important thing, a car

## Getting started

### Upload via Web Installer (ESP Web Tools)

If you don't want to install PlatformIO and compile by your own, use
the [Web Installer](https://adlerre.github.io/obd2-mqtt/).

### Update Settings or Firmware & Filesystem

* connect to WiFi Access Point starts with name OBD2-MQTT- followed from device MAC
* open Browser and navigate to http://192.168.4.1
* change settings to your needs and reboot afterward __OR__ update to new firmware and filesystem

<p>
<img width="200" alt="Info" src="assets/obd2-mqtt-ui-info-01.png">
<img width="200" alt="Settings1" src="assets/obd2-mqtt-ui-settings-01.png">
<img width="200" alt="Settings2" src="assets/obd2-mqtt-ui-settings-02.png">
</p>

### Build

Build firmware.bin

```bash
pio run [-e OPTIONAL ENV]
```

Build littlefs.bin

```bash
pio run --target buildfs [-e OPTIONAL ENV]
```

### Upload

Build and upload firmware.bin to device

```bash
pio run --target upload -e T-Call-A7670X-V1-0
```

Build and upload littlefs.bin to device

```bash
# connect to AP and save current settings
curl http://192.168.4.1/api/settings -o settings.json

pio run --target uploadfs -e T-Call-A7670X-V1-0

# after reboot connect to AP
curl -X PUT -H "Content-Type: application/json" -d @settings.json http://192.168.4.1/api/settings
```

## Supported Sensors

This sensors only available if your car support.

* Ambient Temperature
* Battery Voltage
* Calculated average speed
* Calculated consumption
* Calculated consumption per 100km or MPG
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
* Top Speed ;-)

Diagnostic Output:

* CPU Temperature (ESP)
* Free Memory (ESP)
* GPS Location (only for A76xx)
* GSM Location
* Signal Quality
* Uptime

<p>
<img width="200" alt="Sensors1" src="assets/obd2-mqtt-ha-01.png">
<img width="200" alt="Sensors2" src="assets/obd2-mqtt-ha-02.png">
</p>

## If you are afraid of the Internet...

...and don't want to expose your MQTT broker on the Internet, use a free MQTT provider and take a look
on [mqtt-proxy](tools/mqtt-proxy) tool.

## ToDos

* support more PIDs (odometer,...)
* get vehicle information (working)