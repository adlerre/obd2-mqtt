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
#include "gsm.h"

#define DEBUG_PORT Serial

boolean wifiConnected = false;
// WiFiClient wifiClient;
// PubSubClient mqtt(wifiClient);

// #define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
GSM gsm(debugger);
#else
GSM gsm(SerialAT);
#endif

PubSubClient mqttClient(gsm.client);

MQTT mqtt(mqttClient, mqttBroker, mqttPort);

#define MQTT_DISCOVERY_INTERVAL             300000L
#define MQTT_DATA_INTERVAL                  1000;
#define MQTT_DIAGNOSTIC_INTERVAL            30000L
#define MQTT_STATIC_DIAGNOSTIC_INTERVAL     60000L
#define LOCATION_INTERVAL                   30000L

std::atomic<unsigned long> startTime{0};

std::atomic_bool allDiscoverySend{false};
std::atomic_bool allDiagnosticDiscoverySend{false};
std::atomic_bool allStaticDiagnosticDiscoverySend{false};

std::atomic<unsigned long> lastDebugOutput{0};
std::atomic<unsigned long> lastMQTTDiscoveryOutput{0};
std::atomic<unsigned long> lastMQTTDiagnosticDiscoveryOutput{0};
std::atomic<unsigned long> lastMQTTStaticDiagnosticDiscoveryOutput{0};
std::atomic<unsigned long> lastMQTTOutput{0};
std::atomic<unsigned long> lastMQTTDiagnosticOutput{0};
std::atomic<unsigned long> lastMQTTStaticDiagnosticOutput{0};

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
std::atomic<uint8_t> fuelType{0};
std::atomic_bool fuelTypeRead{false};
std::atomic<float> mafRate{0};
std::atomic<float> batVoltage{0};
std::atomic<float> intakeAirTemp{0};
std::atomic<uint8_t> manifoldPressure{0};
std::atomic<float> timingAdvance{0};
std::atomic<float> pedalPosition{0};
std::atomic<u32_t> monitorStatus{0};
std::atomic_bool milState{false};

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
TaskHandle_t locationTaskHdl;

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

void readStates() {
    if (!ELM_PORT.isClosed()) {
        switch (obd_state) {
            case ENG_LOAD: {
                if (isPidSupported(ENGINE_LOAD)) {
                    setStateValue(load, THROTTLE, static_cast<int>(myELM327.engineLoad()));
                } else {
                    obd_state = THROTTLE;
                }
                break;
            }
            case THROTTLE:
                if (isPidSupported(THROTTLE_POSITION)) {
                    setStateValue(throttle, RPM, static_cast<int>(myELM327.throttle()));
                } else {
                    obd_state = RPM;
                }
                break;
            case RPM: {
                if (isPidSupported(ENGINE_RPM)) {
                    setStateValue(rpm, COOLANT_TEMP, myELM327.rpm());
                } else {
                    obd_state = COOLANT_TEMP;
                }
                break;
            }
            case COOLANT_TEMP: {
                if (isPidSupported(ENGINE_COOLANT_TEMP)) {
                    setStateValue(coolantTemp, OIL_TEMP, myELM327.engineCoolantTemp());
                } else {
                    obd_state = OIL_TEMP;
                }
                break;
            }
            case OIL_TEMP: {
                if (isPidSupported(ENGINE_OIL_TEMP)) {
                    setStateValue(oilTemp, AMBIENT_TEMP, myELM327.oilTemp());
                } else {
                    obd_state = AMBIENT_TEMP;
                }
                break;
            }
            case AMBIENT_TEMP: {
                if (isPidSupported(AMBIENT_AIR_TEMP)) {
                    setStateValue(ambientAirTemp, SPEED, myELM327.ambientAirTemp());
                } else {
                    obd_state = SPEED;
                }
                break;
            }
            case SPEED: {
                if (isPidSupported(VEHICLE_SPEED)) {
                    int kphBefore = kph;
                    setStateValue(kph, MAF_RATE, myELM327.kph());

                    if (runStartTime == 0 & kph > 0) {
                        runStartTime = millis();
                    }

                    distanceDriven = distanceDriven +
                                     calcDistance(
                                         (kphBefore + kph) / 2,
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
                    setStateValue(mafRate, FUEL_LEVEL, myELM327.mafRate());
                } else {
                    obd_state = FUEL_LEVEL;
                }
                break;
            }
            case FUEL_LEVEL: {
                if (isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
                    setStateValue(fuelLevel, FUEL_RATE, myELM327.fuelLevel());
                } else {
                    obd_state = FUEL_RATE;
                }
                break;
            }
            case FUEL_RATE: {
                if (isPidSupported(ENGINE_FUEL_RATE)) {
                    setStateValue(fuelRate, FUEL_T, myELM327.fuelRate());
                } else {
                    obd_state = FUEL_T;
                }
                break;
            }
            case FUEL_T: {
                if (isPidSupported(FUEL_TYPE) && !fuelTypeRead) {
                    setStateValue(fuelType, BAT_VOLTAGE, myELM327.fuelType());
                    fuelTypeRead = true;
                } else {
                    obd_state = BAT_VOLTAGE;
                }
                break;
            }
            case BAT_VOLTAGE: {
                setStateValue(batVoltage, INT_AIR_TEMP, myELM327.batteryVoltage());
                break;
            }
            case INT_AIR_TEMP: {
                if (isPidSupported(INTAKE_AIR_TEMP)) {
                    setStateValue(intakeAirTemp, MANIFOLD_PRESSURE, myELM327.intakeAirTemp());
                } else {
                    obd_state = MANIFOLD_PRESSURE;
                }
                break;
            }
            case MANIFOLD_PRESSURE: {
                if (isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
                    setStateValue(manifoldPressure, IGN_TIMING, myELM327.manifoldPressure());
                } else {
                    obd_state = IGN_TIMING;
                }
                break;
            }
            case IGN_TIMING: {
                if (isPidSupported(TIMING_ADVANCE)) {
                    setStateValue(timingAdvance, PEDAL_POS, myELM327.timingAdvance());
                } else {
                    obd_state = PEDAL_POS;
                }
                break;
            }
            case PEDAL_POS: {
                if (isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
                    setStateValue(pedalPosition, MILSTATUS, myELM327.relativePedalPos());
                } else {
                    obd_state = MILSTATUS;
                }
                break;
            }
            case MILSTATUS: {
                if (isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED)) {
                    setStateValue(monitorStatus, ENG_LOAD, myELM327.monitorStatus());
                    milState = ((monitorStatus >> 16) & 0xFF) & 0x80;
                } else {
                    obd_state = ENG_LOAD;
                }
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

    if (isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "mil", "Check Engine Light", "engine-off", "", "", "", "",
                                                  "binary_sensor");
    }

    if (isPidSupported(ENGINE_LOAD)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "load", "Engine Load", "engine", "%", "", "", "");
    }

    if (isPidSupported(THROTTLE_POSITION)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "throttle", "Throttle", "gauge", "%", "", "", "");
    }

    if (isPidSupported(ENGINE_COOLANT_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "coolantTemp", "Engine Coolant Temperature", "thermometer", "°C",
                                                  "temperature", "measurement", "");
    }

    if (isPidSupported(ENGINE_OIL_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "oilTemp", "Oil Temperature", "thermometer", "°C",
                                                  "temperature", "measurement", "");
    }

    if (isPidSupported(AMBIENT_AIR_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "ambientAirTemp", "Ambient Temperature", "thermometer",
                                                  "°C", "temperature", "measurement", "");
    }

    if (isPidSupported(INTAKE_AIR_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "intakeAirTemp", "Intake Air Temperature", "thermometer", "°C",
                                                  "temperature", "measurement", "");
    }

    // if (isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
    //     allSendsSuccessed |= mqtt.sendTopicConfig("", "manifoldPressure", "Manifold Pressure", "", "kPa",
    //                                               "pressure", "measurement", "");
    // }

    if (isPidSupported(MAF_FLOW_RATE)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "mafRate", "Mass Air Flow", "air-filter", "g/s", "",
                                                  "measurement", "");
    }

    // if (isPidSupported(TIMING_ADVANCE)) {
    //     allSendsSuccessed |= mqtt.sendTopicConfig("", "timingAdvance", "Timing Advance", "axis-x-rotate-clockwise", "°",
    //                                               "", "measurement", "");
    // }

    if (isPidSupported(ENGINE_RPM)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "rpm", "Rounds per minute", "engine", "", "", "measurement", "");
    }

    if (isPidSupported(VEHICLE_SPEED)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "kph", "Kilometer per Hour", "speedometer", "km/h", "speed",
                                                  "measurement", "");
    }

    if (isPidSupported(ENGINE_FUEL_RATE)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "fuelRate", "Fuel Rate", "fuel", "L/h", "", "measurement", "");
    }

    if (isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "fuelLevel", "Fuel Level", "fuel", "%", "", "measurement", "");
    }

    allSendsSuccessed |= mqtt.sendTopicConfig("", "batVoltage", "Battery Voltage", "battery", "V", "voltage",
                                              "measurement", "");

    if (isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "pedalPosition", "Pedal Position", "seat-recline-extra", "%", "",
                                                  "measurement", "");
    }

    // allSendsSuccessed |= mqtt.sendTopicConfig("", "curConsumption", "Calculated current consumption",
    //                                           "gas-station-outline", "l/100km", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "consumption", "Calculated consumption",
                                              "gas-station", "L", "volume", "", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "consumptionPer100", "Calculated consumption per 100km",
                                              "gas-station", "l/100km", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "distanceDriven", "Calculated driven distance",
                                              "map-marker-distance", "km", "distance", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "avgSpeed", "Calculated average speed",
                                              "speedometer-medium", "km/h", "speed", "measurement", "");

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendDiagnosticDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    DEBUG_PORT.print("Send diagnostic discovery data...");

    allSendsSuccessed |= mqtt.sendTopicConfig("", "cpuTemp", "CPU Temperature", "thermometer", "°C", "temperature",
                                              "measurement", "diagnostic");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "freeMem", "Free Memory", "memory", "B", "", "measurement",
                                              "diagnostic");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "uptime", "Uptime", "timer-play", "sec", "", "measurement",
                                              "diagnostic");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "reconnects", "Number of reconnects", "connection", "", "",
                                              "measurement", "diagnostic");

    if (!gsm.getIpAddress().empty()) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "ipAddress", "IP Address", "network-outline", "", "", "", "",
                                                  "diagnostic");
    }

    if (GSM::isUseGPRS()) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "signalQuality", "Signal Quality", "signal", "dBm",
                                                  "signal_strength", "", "diagnostic");
    }

    if (GSM::hasGSMLocation()) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "gsmLocation", "GSM Location", "crosshairs-gps", "", "", "",
                                                  "diagnostic", "device_tracker", "gps", true);
    }

    if (GSM::hasGPSLocation()) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "gpsLocation", "GPS Location", "crosshairs-gps", "", "", "",
                                                  "diagnostic", "device_tracker", "gps", true);
    }

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendStaticDiagnosticDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    DEBUG_PORT.print("Send static diagnostic discovery data...");

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
    }

    if (!VIN.empty()) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "VIN", "Vehicle Identification Number", "car", "", "", "", "",
                                                  "diagnostic");
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

    if (isPidSupported(ENGINE_RPM)) {
        allSendsSuccessed |= mqtt.sendTopicUpdate("engineRunning", rpm > 300 ? "on" : "off");
    }

    if (isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED)) {
        allSendsSuccessed |= mqtt.sendTopicUpdate("mil", milState ? "on" : "off");
    }

    if (isPidSupported(ENGINE_LOAD)) {
        sprintf(tmp_char, "%d", static_cast<int>(load));
        allSendsSuccessed |= mqtt.sendTopicUpdate("load", std::string(tmp_char));
    }

    if (isPidSupported(THROTTLE_POSITION)) {
        sprintf(tmp_char, "%d", static_cast<int>(throttle));
        allSendsSuccessed |= mqtt.sendTopicUpdate("throttle", std::string(tmp_char));
    }

    if (isPidSupported(ENGINE_COOLANT_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(coolantTemp));
        allSendsSuccessed |= mqtt.sendTopicUpdate("coolantTemp", std::string(tmp_char));
    }

    if (isPidSupported(ENGINE_OIL_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(oilTemp));
        allSendsSuccessed |= mqtt.sendTopicUpdate("oilTemp", std::string(tmp_char));
    }

    if (isPidSupported(AMBIENT_AIR_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(ambientAirTemp));
        allSendsSuccessed |= mqtt.sendTopicUpdate("ambientAirTemp", std::string(tmp_char));
    }

    if (isPidSupported(INTAKE_AIR_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(intakeAirTemp));
        allSendsSuccessed |= mqtt.sendTopicUpdate("intakeAirTemp", std::string(tmp_char));
    }

    // if (isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
    //     sprintf(tmp_char, "%d", static_cast<int>(manifoldPressure));
    //     allSendsSuccessed |= mqtt.sendTopicUpdate("manifoldPressure", std::string(tmp_char));
    // }

    if (isPidSupported(MAF_FLOW_RATE)) {
        sprintf(tmp_char, "%4.2f", static_cast<float>(mafRate));
        allSendsSuccessed |= mqtt.sendTopicUpdate("mafRate", std::string(tmp_char));
    }

    // if (isPidSupported(TIMING_ADVANCE)) {
    //     sprintf(tmp_char, "%d", static_cast<int>(timingAdvance));
    //     allSendsSuccessed |= mqtt.sendTopicUpdate("timingAdvance", std::string(tmp_char));
    // }

    if (isPidSupported(ENGINE_RPM)) {
        sprintf(tmp_char, "%d", static_cast<int>(rpm));
        allSendsSuccessed |= mqtt.sendTopicUpdate("rpm", std::string(tmp_char));
    }

    if (isPidSupported(VEHICLE_SPEED)) {
        sprintf(tmp_char, "%d", static_cast<int>(kph));
        allSendsSuccessed |= mqtt.sendTopicUpdate("kph", std::string(tmp_char));
    }

    if (isPidSupported(ENGINE_FUEL_RATE)) {
        sprintf(tmp_char, "%4.2f", static_cast<float>(fuelRate));
        allSendsSuccessed |= mqtt.sendTopicUpdate("fuelRate", std::string(tmp_char));
    }

    if (isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
        sprintf(tmp_char, "%d", static_cast<int>(fuelLevel));
        allSendsSuccessed |= mqtt.sendTopicUpdate("fuelLevel", std::string(tmp_char));
    }

    sprintf(tmp_char, "%4.2f", static_cast<float>(batVoltage));
    allSendsSuccessed |= mqtt.sendTopicUpdate("batVoltage", std::string(tmp_char));

    if (isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
        sprintf(tmp_char, "%d", static_cast<int>(pedalPosition));
        allSendsSuccessed |= mqtt.sendTopicUpdate("pedalPosition", std::string(tmp_char));
    }

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

    if (!gsm.getIpAddress().empty()) {
        sprintf(tmp_char, "%s", gsm.getIpAddress().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("ipAddress", std::string(tmp_char));
    }

    if (GSM::isUseGPRS() && signalQuality != SQ_NOT_KNOWN) {
        sprintf(tmp_char, "%d", GSM::convertSQToRSSI(signalQuality));
        allSendsSuccessed |= mqtt.sendTopicUpdate("signalQuality", std::string(tmp_char));
    }

    std::string payload;
    JsonDocument attribs;

    if (GSM::hasGSMLocation()) {
        attribs["latitude"] = static_cast<float>(gsmLatitude);
        attribs["longitude"] = static_cast<float>(gsmLongitude);
        attribs["gps_accuracy"] = static_cast<float>(gsmAccuracy);
        serializeJson(attribs, payload);

        allSendsSuccessed |= mqtt.sendTopicUpdate("gsmLocation", payload, true);
    }
    if (GSM::hasGPSLocation()) {
        attribs["latitude"] = static_cast<float>(gpsLatitude);
        attribs["longitude"] = static_cast<float>(gpsLongitude);
        attribs["gps_accuracy"] = static_cast<float>(gpsAccuracy);
        serializeJson(attribs, payload);

        allSendsSuccessed |= mqtt.sendTopicUpdate("gpsLocation", payload, true);
    }

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
    }

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

void mqttSendData() {
    if (millis() < lastMQTTOutput) {
        return;
    }

    if (mqtt.connected()) {
        if (millis() > lastMQTTDiscoveryOutput) {
            allDiscoverySend = false;
        }

        if (!allDiscoverySend) {
            if ((allDiscoverySend = sendDiscoveryData())) {
                lastMQTTDiscoveryOutput = millis() + MQTT_DISCOVERY_INTERVAL;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTDiagnosticDiscoveryOutput) {
            allDiagnosticDiscoverySend = false;
        }

        if (!allDiagnosticDiscoverySend) {
            if ((allDiagnosticDiscoverySend = sendDiagnosticDiscoveryData())) {
                lastMQTTDiagnosticDiscoveryOutput = millis() + MQTT_DISCOVERY_INTERVAL;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTStaticDiagnosticDiscoveryOutput) {
            allStaticDiagnosticDiscoverySend = false;
        }

        if (!allStaticDiagnosticDiscoverySend) {
            if ((allStaticDiagnosticDiscoverySend = sendStaticDiagnosticDiscoveryData())) {
                lastMQTTStaticDiagnosticDiscoveryOutput = millis() + MQTT_DISCOVERY_INTERVAL;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTDiagnosticOutput) {
            if (sendDiagnosticData()) {
                lastMQTTDiagnosticOutput = millis() + MQTT_DIAGNOSTIC_INTERVAL;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTStaticDiagnosticOutput) {
            if (sendStaticDiagnosticData()) {
                lastMQTTStaticDiagnosticOutput = millis() + MQTT_STATIC_DIAGNOSTIC_INTERVAL;
            } else {
                return;
            }
        }

        if (sendOBDData()) {
            lastMQTTOutput = millis() + MQTT_DATA_INTERVAL;
        }
    } else {
        delay(500);
    }
}

void readStatesTask(void *parameters) {
    for (;;) {
        readStates();
        delay(10);
    }
}

void outputTask(void *parameters) {
    for (;;) {
        if (!gsm.checkNetwork()) {
            continue;
        }
        // debugOutputStates();

        if (GSM::isUseGPRS()) {
            signalQuality = gsm.getSignalQuality();
        }

        if (!mqtt.connected()) {
            auto client_id = String(MQTT_CLIENT_ID) + "-" + MQTT::stripChars(connectedBTAddress).c_str();
            mqtt.connect(client_id.c_str(), mqttUsername, mqttPassword);
        } else {
            mqttSendData();
        }
        delay(100);
    }
}

void mqttTask(void *parameters) {
    for (;;) {
        mqtt.loop();
        delay(100);
    }
}

void locationTask(void *parameters) {
    unsigned long checkInterval = 0;
    for (;;) {
        if (millis() > checkInterval) {
            if (gsm.isNetworkConnected()) {
                float gsm_latitude = 0;
                float gsm_longitude = 0;
                float gsm_accuracy = 0;

                if (gsm.readGSMLocation(gsm_latitude, gsm_longitude, gsm_accuracy)) {
                    gsmLatitude = gsm_latitude;
                    gsmLongitude = gsm_longitude;
                    gsmAccuracy = gsm_accuracy;
                }
            }

            if (GSM::hasGPSLocation()) {
                float gps_latitude = 0;
                float gps_longitude = 0;
                float gps_accuracy = 0;

                if (!gsm.readGPSLocation(gps_latitude, gps_longitude, gps_accuracy)) {
                    gsm.checkGPS();
                } else {
                    gpsLatitude = gps_latitude;
                    gpsLongitude = gps_longitude;
                    gpsAccuracy = gps_accuracy;
                }
            }

            checkInterval = millis() + LOCATION_INTERVAL;
        }
        delay(100);
    }
}

void setup() {
    startTime = millis();

    DEBUG_PORT.begin(115200);

    gsm.connectToNetwork();
    // connectToWiFi(wifiSSID, wifiPass);
    gsm.enableGPS();
    connectToOBD();

    mqtt.setIdentifier(!MQTT::stripChars(VIN).empty() ? VIN : connectedBTAddress);

    // disable Watch Dog for Core 0 - should fix crashes
    disableCore0WDT();

    xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 8192, nullptr, 1, &mqttTaskHdl, 0);
    xTaskCreatePinnedToCore(outputTask, "OutputTask", 8192, nullptr, 10, &outputTaskHdl, 0);

    xTaskCreatePinnedToCore(readStatesTask, "ReadStatesTask", 8192, nullptr, 1, &stateTaskHdl, 1);

    if (GSM::hasGSMLocation() || GSM::hasGPSLocation()) {
        xTaskCreatePinnedToCore(locationTask, "LocationTask", 8192, nullptr, 11, &locationTaskHdl, tskNO_AFFINITY);
    }
}

void loop() {
    vTaskDelete(nullptr);
}
