# MQTT Proxy

Proxies all OBD2 to MQTT related topics from one broker to another. 

## Usage

Clone this repository...

```bash
cd tools/mqtt-proxy
npx mqtt-proxy -h
```

### Commandline Options

* **--srcHostname**<br />

  Set source hostname

* **--srcPort**<br />
  *Default:* `1883`<br />

  Set source port

* **--srcUsername**<br />
  *optional* 

  Set source username

* **--srcPassword**<br />
  *optional*
 
  Set source password

* **--dstHostname**<br />

  Set destination hostname

* **--dstPort**<br />
  *Default:* `1883`<br />

  Set destination port

* **--dstUsername**<br />
  *optional*

  Set destination username

* **--dstPassword**<br />
  *optional*

  Set destination password

* **-h, --help**<bbr />

  Print help (this message) and exit.
