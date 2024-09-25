/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, in a file called gpl.txt or license.txt.
 * If not, write to the Free Software Foundation Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */
#include "mqtt.h"

#include <WiFi.h>
#include <atomic>
#include <bitset>
#include <utility>
#include <vector>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

#ifndef BUILD_GIT_BRANCH
#define BUILD_GIT_BRANCH ""
#endif
#ifndef BUILD_GIT_COMMIT_HASH
#define BUILD_GIT_COMMIT_HASH ""
#endif

#include "privates.h"
#include "obd.h"

#define DEBUG_PORT Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
#include "device_sim800.h"

// Select your modem:
#define TINY_GSM_MODEM_SIM800
#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_SIM767XG_S3) or defined(LILYGO_T_A7608X) or defined(LILYGO_T_A7608X_S3) or defined(LILYGO_T_A7608X_DC_S3)
#include "device_simA7670.h"

#define TINY_GSM_MODEM_SIM7600
#endif

#include <TinyGsmClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

std::atomic_bool allDiscoverySend{false};

boolean wifiConnected = false;
// WiFiClient wifiClient;
// PubSubClient mqtt(wifiClient);

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqttClient(client);

MQTT mqtt(mqttClient, mqttBroker, mqttPort);

std::atomic<unsigned long> startTime{0};

std::atomic<unsigned long> lastDebugOutput{0};
std::atomic<unsigned long> lastMQTTDiscoveryOutput{0};
std::atomic<unsigned long> lastMQTTOutput{0};
std::atomic<unsigned long> lastMQTTDiagnosticOutput{0};

std::atomic<int> signalQuality{0};
std::atomic<float> gsmLatitude{0};
std::atomic<float> gsmLongitude{0};
std::atomic<float> gsmAccuracy{0};
std::atomic<float> gpsLatitude{0};
std::atomic<float> gpsLongitude{0};
std::atomic<float> gpsAccuracy{0};

std::atomic<int> load{0};
std::atomic<int> throttle{0};
std::atomic<float> rpm{0};
std::atomic<float> coolantTemp{0};
std::atomic<float> oilTemp{0};
std::atomic<float> ambientAirTemp{0};
std::atomic<int> kph{0};
std::atomic<float> fuelLevel{0};
std::atomic<float> fuelRate{0};
std::atomic<int> fuelType{0};
std::atomic_bool fuelTypeRead{false};
std::atomic<float> mafRate{0};
std::atomic<float> batVoltage{0};
std::atomic<float> intakeAirTemp{0};
std::atomic<int> manifoldPressure{0};
std::atomic<float> timingAdvance{0};
std::atomic<float> pedalPosition{0};

std::atomic<unsigned long> lastReadSpeed{0};
std::atomic<unsigned long> runStartTime{0};
std::atomic<float> curConsumption{0};
std::atomic<float> consumption{0};
std::atomic<float> consumptionPer100{0};
std::atomic<float> distanceDriven{0};
std::atomic<float> avgSpeed{0};

TaskHandle_t outputTaskHdl;
TaskHandle_t stateTaskHdl;
TaskHandle_t mqttTaskHdl;

size_t getESPHeapSize() {
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            DEBUG_PORT.print("WiFi connected! IP address: ");
            DEBUG_PORT.println(WiFi.localIP());
            wifiConnected = true;
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            DEBUG_PORT.println("WiFi lost connection");
            wifiConnected = false;
            break;
        default: break;
    }
}

void connectToWiFi(const char *ssid, const char *pwd) {
    DEBUG_PORT.println("Connecting to WiFi network: " + String(ssid));

    WiFi.disconnect(true);
    WiFi.onEvent(WiFiEvent);

    WiFi.begin(ssid, pwd);

    DEBUG_PORT.println("Waiting for WiFi connection...");
}

void connectToNetwork() {
    Serial.println("Start modem...");

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
    setupModem();

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_SIM767XG_S3) or defined(LILYGO_T_A7608X) or defined(LILYGO_T_A7608X_S3) or defined(LILYGO_T_A7608X_DC_S3)
    // Turn on DC boost to power on the modem
#ifdef BOARD_POWERON_PIN
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

    // Set modem reset pin ,reset modem
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
    delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
    delay(2600);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

    // Turn on modem
    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(1000);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);

    // Set modem baud
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
#endif

    delay(6000);
restart:
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    DEBUG_PORT.println("Initializing modem...");
    if (!modem.init()) {
        DEBUG_PORT.println("Failed to restart modem, delaying 10s and retrying");
        ESP.restart();
        return;
    }
    // modem.restart();

    String name = modem.getModemName();
    DEBUG_PORT.printf("Modem Name: %s\n", name.c_str());

    String modemInfo = modem.getModemInfo();
    DEBUG_PORT.printf("Modem Info: %s\n", modemInfo.c_str());

#if TINY_GSM_USE_GPRS
    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3) {
        modem.simUnlock(GSM_PIN);
    }
#endif

#if TINY_GSM_USE_WIFI
    // Wifi connection parameters must be set before waiting for the network
    DEBUG_PORT.print(F("Setting SSID/password..."));
    if (!modem.networkConnect(wifiSSID, wifiPass)) {
        DEBUG_PORT.println("...fail");
        delay(10000);
        goto restart;
    }
    DEBUG_PORT.println("...success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
    // The XBee must run the gprsConnect function BEFORE waiting for network!
    DEBUG_PORT.print("Waiting for GPRS connect...");
    modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

    DEBUG_PORT.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        DEBUG_PORT.println("...fail");
        delay(10000);
        goto restart;
    }
    DEBUG_PORT.println("...success");

    if (modem.isNetworkConnected()) {
        DEBUG_PORT.println("Network connected");
    }

#if TINY_GSM_USE_GPRS
    // GPRS connection parameters are usually set after network registration
    DEBUG_PORT.printf("Connecting to %s...", apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        DEBUG_PORT.println("...fail");
        delay(10000);
        goto restart;
    }
    DEBUG_PORT.println("...success");

    if (modem.isGprsConnected()) {
        DEBUG_PORT.println("GPRS connected");
    }

    delay(1000);
#endif
}

bool checkNetwork() {
    // Make sure we're still registered on the network
    if (!modem.isNetworkConnected()) {
        DEBUG_PORT.println("Network disconnected");
        if (!modem.waitForNetwork(20000L, true)) {
            DEBUG_PORT.println("...fail");
            delay(10000);
            return false;
        }
        if (modem.isNetworkConnected()) {
            DEBUG_PORT.println("Network reconnected");
        }

#if TINY_GSM_USE_GPRS
        // and make sure GPRS/EPS is still connected
        if (!modem.isGprsConnected()) {
            mqtt.disconnect();
            DEBUG_PORT.println("GPRS disconnected");
            DEBUG_PORT.printf("Connecting to %s...", apn);
            if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
                DEBUG_PORT.println("...fail");
                delay(10000);
                return false;
            }
            if (modem.isGprsConnected()) {
                DEBUG_PORT.println("GPRS reconnected");
            }
        }
#endif
    }

    return true;
}

void readStates() {
    if (!ELM_PORT.isClosed()) {
        switch (obd_state) {
            case ENG_LOAD: {
                if (isPidSupported(ENGINE_LOAD)) {
                    setStateIntValue(load, THROTTLE, static_cast<int>(myELM327.engineLoad()));
                } else {
                    obd_state = THROTTLE;
                }
                break;
            }
            case THROTTLE:
                if (isPidSupported(THROTTLE_POSITION)) {
                    setStateIntValue(throttle, RPM, static_cast<int>(myELM327.throttle()));
                } else {
                    obd_state = RPM;
                }
                break;
            case RPM: {
                if (isPidSupported(ENGINE_RPM)) {
                    setStateFloatValue(rpm, COOLANT_TEMP, myELM327.rpm());
                } else {
                    obd_state = COOLANT_TEMP;
                }
                break;
            }
            case COOLANT_TEMP: {
                if (isPidSupported(ENGINE_COOLANT_TEMP)) {
                    setStateFloatValue(coolantTemp, OIL_TEMP, myELM327.engineCoolantTemp());
                } else {
                    obd_state = OIL_TEMP;
                }
                break;
            }
            case OIL_TEMP: {
                if (isPidSupported(ENGINE_OIL_TEMP)) {
                    setStateFloatValue(oilTemp, AMBIENT_TEMP, myELM327.oilTemp());
                } else {
                    obd_state = AMBIENT_TEMP;
                }
                break;
            }
            case AMBIENT_TEMP: {
                if (isPidSupported(AMBIENT_AIR_TEMP)) {
                    setStateFloatValue(ambientAirTemp, SPEED, myELM327.ambientAirTemp());
                } else {
                    obd_state = SPEED;
                }
                break;
            }
            case SPEED: {
                if (isPidSupported(VEHICLE_SPEED)) {
                    int kphBefore = kph;
                    setStateIntValue(kph, MAF_RATE, myELM327.kph());

                    if (runStartTime == 0 & kph > 0) {
                        runStartTime = millis();
                    }

                    distanceDriven = distanceDriven +
                                     calcDistance(
                                         (kphBefore + kph) / 2.0f,
                                         static_cast<float>(millis() - lastReadSpeed) / 1000.0f
                                     );
                    consumption = consumption + calcConsumption(fuelType, kph, mafRate) / 3600.0f * static_cast<float>(
                                      millis() - lastReadSpeed) /
                                  1000.0f;
                    lastReadSpeed = millis();
                } else {
                    obd_state = MAF_RATE;
                }
                break;
            }
            case MAF_RATE: {
                if (isPidSupported(MAF_FLOW_RATE)) {
                    setStateFloatValue(mafRate, FUEL_LEVEL, myELM327.mafRate());
                } else {
                    obd_state = FUEL_LEVEL;
                }
                break;
            }
            case FUEL_LEVEL: {
                if (isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
                    setStateFloatValue(fuelLevel, FUEL_RATE, myELM327.fuelLevel());
                } else {
                    obd_state = FUEL_RATE;
                }
                break;
            }
            case FUEL_RATE: {
                if (isPidSupported(ENGINE_FUEL_RATE)) {
                    setStateFloatValue(fuelRate, FUEL_T, myELM327.fuelRate());
                } else {
                    obd_state = FUEL_T;
                }
                break;
            }
            case FUEL_T: {
                if (isPidSupported(FUEL_TYPE) && !fuelTypeRead) {
                    setStateIntValue(fuelType, BAT_VOLTAGE, myELM327.fuelType());
                    fuelTypeRead = true;
                } else {
                    obd_state = BAT_VOLTAGE;
                }
                break;
            }
            case BAT_VOLTAGE: {
                setStateFloatValue(batVoltage, INT_AIR_TEMP, myELM327.batteryVoltage());
                break;
            }
            case INT_AIR_TEMP: {
                if (isPidSupported(INTAKE_AIR_TEMP)) {
                    setStateFloatValue(intakeAirTemp, MANIFOLD_PRESSURE, myELM327.intakeAirTemp());
                } else {
                    obd_state = MANIFOLD_PRESSURE;
                }
                break;
            }
            case MANIFOLD_PRESSURE: {
                if (isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
                    setStateIntValue(manifoldPressure, IGN_TIMING, myELM327.manifoldPressure());
                } else {
                    obd_state = IGN_TIMING;
                }
                break;
            }
            case IGN_TIMING: {
                if (isPidSupported(TIMING_ADVANCE)) {
                    setStateFloatValue(timingAdvance, PEDAL_POS, myELM327.timingAdvance());
                } else {
                    obd_state = PEDAL_POS;
                }
                break;
            }
            case PEDAL_POS: {
                if (isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
                    setStateFloatValue(pedalPosition, ENG_LOAD, myELM327.relativePedalPos());
                } else {
                    obd_state = ENG_LOAD;
                }
                break;
            }
        }

        avgSpeed = distanceDriven / (static_cast<float>(millis() - runStartTime) / 1000.0f) * 3600.0f;
        // curConsumption = calcCurrentConsumption(fuelType, kph, mafRate);
        consumptionPer100 = consumption / distanceDriven * 100.0f;
    } else {
        delay(500);
    }
}

void debugOutputStates() {
    if (millis() - lastDebugOutput < 500) {
        return;
    }

    lastDebugOutput = millis();
    DEBUG_PORT.printf("============================================\n");
    DEBUG_PORT.printf("LOAD              : %7d\n", static_cast<int>(load));
    DEBUG_PORT.printf("THROTTLE          : %7d\n", static_cast<int>(throttle));
    DEBUG_PORT.printf("COOLANT TEMP      : %7d\n", static_cast<int>(coolantTemp));
    DEBUG_PORT.printf("OIL TEMP          : %7d\n", static_cast<int>(oilTemp));
    DEBUG_PORT.printf("AMBIENT TEMP      : %7d\n", static_cast<int>(ambientAirTemp));
    DEBUG_PORT.printf("INTAKE AIR TEMP   : %7d\n", static_cast<int>(intakeAirTemp));
    DEBUG_PORT.printf("MANIFOLD PRESSURE : %7d\n", static_cast<int>(manifoldPressure));
    DEBUG_PORT.printf("MAF FLOW RATE     : %4.2f\n", static_cast<float>(mafRate));
    DEBUG_PORT.printf("IGNITION TIMING   : %4.2f\n", static_cast<float>(timingAdvance));
    DEBUG_PORT.printf("RPM               : %7d\n", static_cast<int>(rpm));
    DEBUG_PORT.printf("KPH               : %7d\n", static_cast<int>(kph));
    DEBUG_PORT.printf("FUEL RATE         : %4.2f\n", static_cast<float>(fuelRate));
    DEBUG_PORT.printf("FUEL LEVEL        : %7d\n", static_cast<int>(fuelLevel));
    DEBUG_PORT.printf("FUEL TYPE         : %7d\n", static_cast<int>(fuelType));
    DEBUG_PORT.printf("BATTERY           : %4.2f\n", static_cast<float>(batVoltage));
}

bool sendDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    DEBUG_PORT.print("Send discovery data...");

    allSendsSuccessed |= mqtt.sendTopicConfig("", "engineRunning", "Engine Running", "engine", "", "", "", "",
                                              "binary_sensor");

    allSendsSuccessed |= mqtt.sendTopicConfig("", "load", "Engine Load", "engine", "%", "", "", "");

    allSendsSuccessed |= mqtt.sendTopicConfig("", "throttle", "Throttle", "gauge", "%", "", "", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "coolantTemp", "Engine Coolant Temperature", "thermometer", "°C",
                                              "temperature", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "oilTemp", "Oil Temperature", "thermometer", "°C",
                                              "temperature", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "ambientAirTemp", "Ambient Temperature", "thermometer",
                                              "°C", "temperature", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "intakeAirTemp", "Intake Air Temperature", "thermometer", "°C",
                                              "temperature", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "manifoldPressure", "Manifold Pressure", "", "kPa",
                                              "pressure", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "mafRate", "Mass Air Flow", "turbine", "g/s", "",
                                              "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "timingAdvance", "Timing Advance", "axis-x-rotate-clockwise", "°", "",
                                              "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "rpm", "Rounds per minute", "engine", "", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "kph", "Kilometer per Hour", "speedometer", "km/h", "speed",
                                              "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "fuelRate", "Fuel Rate", "fuel", "L/h", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "fuelLevel", "Fuel Level", "fuel", "%", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "batVoltage", "Battery Voltage", "battery", "V", "power",
                                              "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "pedalPosition", "Pedal Position", "arrow-up-down", "%", "",
                                              "measurement", "");

    // allSendsSuccessed |= mqtt.sendTopicConfig("", "curConsumption", "Calculated current consumption",
    //                                           "gas-station-outline", "l/100km", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "consumption", "Calculated consumption",
                                              "gas-station", "l", "volume", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "consumptionPer100", "Calculated consumption per 100km",
                                              "gas-station", "l/100km", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "distanceDriven", "Calculated driven distance",
                                              "map-marker-distance", "km", "distance", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "avgSpeed", "Calculated average speed",
                                              "speedometer-medium", "km/h", "speed", "measurement", "");

    allSendsSuccessed |= mqtt.sendTopicConfig("", "cpuTemp", "CPU Temperature", "thermometer", "°C", "temperature",
                                              "measurement", "diagnostic");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "freeMem", "Free Memory", "memory", "B", "", "measurement",
                                              "diagnostic");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "uptime", "Uptime", "timer-play", "sec", "", "measurement",
                                              "diagnostic");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "reconnects", "Number of reconnects", "connection", "", "",
                                              "measurement", "diagnostic");
#if TINY_GSM_USE_GPRS
    allSendsSuccessed |= mqtt.sendTopicConfig("", "signalQuality", "Signal Quality", "signal", "dBm",
                                              "signal_strength", "", "diagnostic");
#endif
#if defined TINY_GSM_MODEM_HAS_GSM_LOCATION
    allSendsSuccessed |= mqtt.sendTopicConfig("", "gsmLocation", "GSM Location", "crosshairs-gps", "", "", "",
                                              "diagnostic", "device_tracker", "gps", true);
#endif
#if defined TINY_GSM_MODEM_HAS_GPS
    allSendsSuccessed |= mqtt.sendTopicConfig("", "gpsLocation", "GPS Location", "crosshairs-gps", "", "", "",
                                              "diagnostic", "device_tracker", "gps", true);
#endif

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendStaticDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    DEBUG_PORT.print("Send static discovery data...");

    if (supportedPids_1_20 != 0 || supportedPids_21_40 != 0 || supportedPids_41_60 != 0
        || supportedPids_61_80 != 0) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_1_20", "Supported PIDs 1-20", "", "", "", "",
                                                  "diagnostic");
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_21_40", "Supported PIDs 21-40", "", "", "", "",
                                                  "diagnostic");
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_41_60", "Supported PIDs 41-60", "", "", "", "",
                                                  "diagnostic");
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_61_80", "Supported PIDs 61-80", "", "", "", "",
                                                  "diagnostic");
    } else {
        allSendsSuccessed = true;
    }

    if (!VIN.empty()) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "VIN", "Vehicle Identification Number", "car", "", "", "", "",
                                                  "diagnostic");
    } else {
        allSendsSuccessed = true;
    }

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendOBDData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;
    char tmp_char[50];

    DEBUG_PORT.print("Send OBD data...");

    allSendsSuccessed |= mqtt.sendTopicUpdate(LWT_TOPIC, LWT_CONNECTED);
    allSendsSuccessed |= mqtt.sendTopicUpdate("engineRunning", rpm > 300 ? "on" : "off");

    sprintf(tmp_char, "%d", static_cast<int>(load));
    allSendsSuccessed |= mqtt.sendTopicUpdate("load", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(throttle));
    allSendsSuccessed |= mqtt.sendTopicUpdate("throttle", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(coolantTemp));
    allSendsSuccessed |= mqtt.sendTopicUpdate("coolantTemp", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(oilTemp));
    allSendsSuccessed |= mqtt.sendTopicUpdate("oilTemp", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(ambientAirTemp));
    allSendsSuccessed |= mqtt.sendTopicUpdate("ambientAirTemp", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(intakeAirTemp));
    allSendsSuccessed |= mqtt.sendTopicUpdate("intakeAirTemp", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(manifoldPressure));
    allSendsSuccessed |= mqtt.sendTopicUpdate("manifoldPressure", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(mafRate));
    allSendsSuccessed |= mqtt.sendTopicUpdate("mafRate", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(timingAdvance));
    allSendsSuccessed |= mqtt.sendTopicUpdate("timingAdvance", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(rpm));
    allSendsSuccessed |= mqtt.sendTopicUpdate("rpm", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(kph));
    allSendsSuccessed |= mqtt.sendTopicUpdate("kph", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(fuelRate));
    allSendsSuccessed |= mqtt.sendTopicUpdate("fuelRate", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(fuelLevel));
    allSendsSuccessed |= mqtt.sendTopicUpdate("fuelLevel", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(batVoltage));
    allSendsSuccessed |= mqtt.sendTopicUpdate("batVoltage", std::string(tmp_char));

    sprintf(tmp_char, "%d", static_cast<int>(pedalPosition));
    allSendsSuccessed |= mqtt.sendTopicUpdate("pedalPosition", std::string(tmp_char));

    // sprintf(tmp_char, "%4.2f", static_cast<float>(curConsumption));
    // allSendsSuccessed |= mqtt.sendTopicUpdate("curConsumption", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(consumption));
    allSendsSuccessed |= mqtt.sendTopicUpdate("consumption", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(consumptionPer100));
    allSendsSuccessed |= mqtt.sendTopicUpdate("consumptionPer100", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(distanceDriven));
    allSendsSuccessed |= mqtt.sendTopicUpdate("distanceDriven", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", static_cast<float>(avgSpeed));
    allSendsSuccessed |= mqtt.sendTopicUpdate("avgSpeed", std::string(tmp_char));

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendDiagnosticData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;
    char tmp_char[50];

    DEBUG_PORT.print("Send diagnostic data...");

    sprintf(tmp_char, "%d", static_cast<int>(temperatureRead()));
    allSendsSuccessed |= mqtt.sendTopicUpdate("cpuTemp", std::string(tmp_char));

    sprintf(tmp_char, "%lu", static_cast<long>(getESPHeapSize()));
    allSendsSuccessed |= mqtt.sendTopicUpdate("freeMem", std::string(tmp_char));

    sprintf(tmp_char, "%lu", (millis() - startTime) / 1000);
    allSendsSuccessed |= mqtt.sendTopicUpdate("uptime", std::string(tmp_char));

    sprintf(tmp_char, "%d", mqtt.reconnectAttemps());
    allSendsSuccessed |= mqtt.sendTopicUpdate("reconnects", std::string(tmp_char));

#if TINY_GSM_USE_GPRS
    sprintf(tmp_char, "%d", static_cast<int>(signalQuality));
    allSendsSuccessed |= mqtt.sendTopicUpdate("signalQuality", std::string(tmp_char));
#endif

#if defined TINY_GSM_MODEM_HAS_GSM_LOCATION || defined TINY_GSM_MODEM_HAS_GPS
    std::string payload;
    JsonDocument attribs;

#if defined TINY_GSM_MODEM_HAS_GSM_LOCATION
    attribs["latitude"] = static_cast<float>(gsmLatitude);
    attribs["longitude"] = static_cast<float>(gsmLongitude);
    attribs["gps_accuracy"] = static_cast<float>(gsmAccuracy);
    serializeJson(attribs, payload);

    allSendsSuccessed |= mqtt.sendTopicUpdate("gsmLocation", payload, true);
#elif  defined TINY_GSM_MODEM_HAS_GPS
    attribs["latitude"] = static_cast<float>(gpsLatitude);
    attribs["longitude"] = static_cast<float>(gpsLongitude);
    attribs["gps_accuracy"] = static_cast<float>(gpsAccuracy);
    serializeJson(attribs, payload);

    allSendsSuccessed |= mqtt.sendTopicUpdate("gpsLocation", payload, true);
#endif

#endif

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendStaticDiagnosticData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    char tmp_char[50];
    DEBUG_PORT.print("Send static diagnostic data...");

    if (supportedPids_1_20 != 0 || supportedPids_21_40 != 0 || supportedPids_41_60 != 0
        || supportedPids_61_80 != 0) {
        sprintf(tmp_char, "%s", std::bitset<32>(supportedPids_1_20).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_1_20", std::string(tmp_char));

        sprintf(tmp_char, "%s", std::bitset<32>(supportedPids_21_40).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_21_40", std::string(tmp_char));

        sprintf(tmp_char, "%s", std::bitset<32>(supportedPids_41_60).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_41_60", std::string(tmp_char));

        sprintf(tmp_char, "%s", std::bitset<32>(supportedPids_61_80).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_61_80", std::string(tmp_char));
    } else {
        allSendsSuccessed = true;
    }

    if (!VIN.empty()) {
        sprintf(tmp_char, "%s", VIN.c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("VIN", std::string(tmp_char));
    } else {
        allSendsSuccessed = true;
    }

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

void mqttSendData() {
    if (millis() - lastMQTTOutput < 1000) {
        return;
    }

    if (mqtt.connected()) {
        // resend discovery topics
        if (millis() - lastMQTTDiscoveryOutput > 300000) {
            allDiscoverySend = false;
        }

        if (!allDiscoverySend && ((allDiscoverySend = sendDiscoveryData() && sendStaticDiscoveryData()))) {
            lastMQTTDiscoveryOutput = millis();
        }

        sendOBDData();

        if (millis() - lastMQTTDiagnosticOutput > 30000) {
            if (sendDiagnosticData() && sendStaticDiagnosticData()) {
                lastMQTTDiagnosticOutput = millis();
            }
        }

        lastMQTTOutput = millis();
    } else {
        delay(500);
    }
}

void readStatesTask(void *parameters) {
    for (;;) {
        readStates();
        delay(1);
    }
}

void outputTask(void *parameters) {
    for (;;) {
        // debugOutputStates();

        if (!checkNetwork()) {
            continue;
        }

#if TINY_GSM_USE_GPRS
        signalQuality = modem.getSignalQuality();
#endif
#if defined TINY_GSM_MODEM_HAS_GSM_LOCATION
        float gsm_latitude = 0;
        float gsm_longitude = 0;
        float gsm_accuracy = 0;

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
        // lat/lng seams to be swapped
        if (modem.getGsmLocation(&gsm_longitude, &gsm_latitude, &gsm_accuracy)) {
#else
        if (modem.getGsmLocation(&gsm_latitude, &gsm_longitude, &gsm_accuracy)) {
#endif
            gsmLatitude = gsm_latitude;
            gsmLongitude = gsm_longitude;
            gsmAccuracy = gsm_accuracy;
        }
#endif

#if defined TINY_GSM_MODEM_HAS_GPS
        float gps_latitude = 0;
        float gps_longitude = 0;
        float gps_speed = 0;
        float gps_altitude = 0;
        int gps_vsat = 0;
        int gps_usat = 0;
        float gps_accuracy = 0;

        if (modem.getGPS(&gps_latitude, &gps_longitude, &gps_speed, &gps_altitude,
                         &gps_vsat, &gps_usat, &gps_accuracy)) {
            gpsLatitude = gps_latitude;
            gpsLongitude = gps_longitude;
            gpsAccuracy = gps_accuracy;
        }
#endif

        if (!mqtt.connected()) {
            auto client_id = String(MQTT_CLIENT_ID) + "-" + modem.getIMEI();
            mqtt.connect(client_id.c_str(), mqttUsername, mqttPassword);
        } else {
            mqttSendData();
        }

        delay(1);
    }
}

void mqttTask(void *parameters) {
    for (;;) {
        mqtt.loop();
        delay(1);
    }
}


void setup() {
    startTime = millis();

    DEBUG_PORT.begin(115200);

    connectToNetwork();
    // connectToWiFi(wifiSSID, wifiPass);

#if defined TINY_GSM_MODEM_HAS_GPS
#if !defined(TINY_GSM_MODEM_SARAR5)  // not needed for this module
    modem.enableGPS();
#endif
#endif

    connectToOBD();

    mqtt.setIdentifier(!VIN.empty() ? VIN : connectedBTAddress);

    // disable Watch Dog for Core 0 - should fix crashes
    disableCore0WDT();

    xTaskCreatePinnedToCore(outputTask, "OutputTask", 10000,NULL, 10, &outputTaskHdl, 0);
    xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 10000,NULL, 1, &mqttTaskHdl, 0);
    xTaskCreatePinnedToCore(readStatesTask, "ReadStatesTask", 10000,NULL, 0, &stateTaskHdl, 1);
}

void loop() {
    vTaskDelete(NULL);
}
