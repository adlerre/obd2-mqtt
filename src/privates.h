// should dev mode enabled?
#define OBD_DEV_MODE    true

// the OBD (ELM327) adapter MAC if not set discovery used
// #define OBD_ADP_MAC     "11:22:33:44:aa:ff"

// set GSM PIN, if any
#define GSM_PIN ""

// Your WiFi connection credentials, if applicable
constexpr char wifiSSID[] = "";
constexpr char wifiPass[] = "";

// Your GPRS credentials, if any
constexpr char apn[] = "";
constexpr char gprsUser[] = "";
constexpr char gprsPass[] = "";

// MQTT Broker
constexpr char mqttBroker[] = "";
constexpr char mqttUsername[] = "";
constexpr char mqttPassword[] = "";
constexpr int mqttPort = 1883;
