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

#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

#define DISCOVERED_DEVICES_FILE "/discovered_devices.json"

#include "settings.h"
#include "helper.h"
#include "obd.h"
#include "gsm.h"
#include "http.h"

HTTPServer server(80);

#define DEBUG_PORT Serial

// #define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
GSM gsm(debugger);
#else
GSM gsm(SerialAT);
#endif

MQTT mqtt(gsm.client);

std::atomic_bool wifiAPStarted{false};
std::atomic_bool wifiAPInUse{false};
std::atomic<unsigned int> wifiAPStaConnected{0};

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
std::atomic<unsigned long> lastMQTTLocationOutput{0};

std::atomic<int> signalQuality{0};
std::atomic<float> gsmLatitude{0};
std::atomic<float> gsmLongitude{0};
std::atomic<float> gsmAccuracy{0};
std::atomic<float> gpsLatitude{0};
std::atomic<float> gpsLongitude{0};
std::atomic<float> gpsAccuracy{0};

TaskHandle_t outputTaskHdl;
TaskHandle_t stateTaskHdl;
TaskHandle_t mqttTaskHdl;

size_t getESPHeapSize() {
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

void WiFiAPStart(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiAPStarted = true;
    DEBUG_PORT.println("WiFi AP started.");

    DEBUG_PORT.printf("AP - IP address: %s\n", WiFi.softAPIP().toString().c_str());
}

void WiFiAPStop(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiAPStarted = false;
    wifiAPInUse = false;
    wifiAPStaConnected = 0;
    DEBUG_PORT.println("WiFi AP stopped.");
}

void WiFiAPStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    ++wifiAPStaConnected;
    wifiAPInUse = true;

    if (wifiAPStaConnected == 1) {
        DEBUG_PORT.println("WiFi AP in use. Stop all other task.");
        OBD.end();
    }
}

void WiFiAPStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (wifiAPStaConnected != 0) {
        --wifiAPStaConnected;
    }

    if (wifiAPStaConnected == 0) {
        DEBUG_PORT.println("WiFi AP all clients disconnected. Start all other task.");
        OBD.begin(Settings.getOBD2Name(OBD_ADP_NAME), Settings.getOBD2MAC(), Settings.getOBD2Protocol(),
                  Settings.getOBD2CheckPIDSupport());
        OBD.connect(true);
        wifiAPInUse = false;
    }
}

void startWiFiAP() {
    DEBUG_PORT.print("Start Access Point...");

    WiFi.disconnect(true);

    WiFi.mode(WIFI_AP);

    WiFi.onEvent(WiFiAPStart, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_START);
    WiFi.onEvent(WiFiAPStop, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STOP);
    WiFi.onEvent(WiFiAPStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent(WiFiAPStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

    String ssid = Settings.getWiFiAPSSID();
    if (ssid.isEmpty()) {
        ssid = "OBD2-MQTT-" + String(stripChars(WiFi.macAddress().c_str()).c_str());
        Settings.setWiFiAPSSID(ssid.c_str());
    }
    WiFi.softAP(
        ssid.c_str(),
        Settings.getWiFiAPPassword()
    );
}

void startHttpServer() {
    server.on(
        "/api/reboot",
        HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (request->hasParam("reboot")) {
                request->send(200);
                DEBUG_PORT.println("Rebooting...");
                delay(2000);
                ESP.restart();
            }
            request->send(406);
        }
    );

    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", Settings.buildJson().c_str());
    });

    server.on(
        "/api/settings",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (request->contentType() == "application/json") {
                std::string json;
                for (size_t i = 0; i < len; i++) {
                    json += static_cast<char>(data[i]);
                }
                if (Settings.parseJson(json)) {
                    if (Settings.writeSettings(LittleFS)) {
                        request->send(200);
                        return;
                    }
                }
                request->send(500);
            }
            request->send(406);
        }
    );

    server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        std::string payload;
        JsonDocument wifiInfo;

        wifiInfo["hostname"] = WiFi.softAPgetHostname();
        wifiInfo["SSID"] = WiFi.softAPSSID();
        wifiInfo["ip"] = WiFi.softAPIP().toString();
        wifiInfo["mac"] = WiFi.macAddress();

        serializeJson(wifiInfo, payload);

        request->send(200, "application/json", payload.c_str());
    });

    server.on("/api/modem", HTTP_GET, [](AsyncWebServerRequest *request) {
        std::string payload;
        JsonDocument modemInfo;

        modemInfo["name"] = gsm.modem.getModemName();
        modemInfo["info"] = gsm.modem.getModemInfo();
        modemInfo["signalQuality"] = gsm.modem.getSignalQuality();
        modemInfo["ip"] = gsm.modem.getLocalIP();
        modemInfo["IMEI"] = gsm.modem.getIMEI();
        modemInfo["IMSI"] = gsm.modem.getIMSI();
        modemInfo["CCID"] = gsm.modem.getSimCCID();
        modemInfo["operator"] = gsm.modem.getOperator();

        serializeJson(modemInfo, payload);

        request->send(200, "application/json", payload.c_str());
    });

    server.on("/api/discoveredDevices", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;

        File file = LittleFS.open(DISCOVERED_DEVICES_FILE, FILE_READ);
        if (file && !file.isDirectory()) {
            if (!deserializeJson(doc, file)) {
                std::string payload;
                serializeJson(doc, payload);
                request->send(200, "application/json", payload.c_str());
            } else {
                request->send(500);
            }
            file.close();
        } else {
            request->send(404);
        }
    });

    server.begin(LittleFS);
}

void onBTDevicesDiscovered(BTScanResults *btDeviceList) {
    JsonDocument devices;

    File file = LittleFS.open(DISCOVERED_DEVICES_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file discovered_devices.json for writing.");
    }

    for (int i = 0; i < btDeviceList->getCount(); i++) {
        JsonDocument dev;
        BTAdvertisedDevice *device = btDeviceList->getDevice(i);
        dev["name"] = device->getName();
        dev["mac"] = device->getAddress().toString();
        devices["device"].add(dev);
    }

    serializeJson(devices, file);

    file.close();
}

bool sendDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    DEBUG_PORT.print("Send discovery data...");

    allSendsSuccessed |= mqtt.sendTopicConfig("", "engineRunning", "Engine Running", "engine", "", "", "", "",
                                              "binary_sensor");

    if (OBD.isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "mil", "Check Engine Light", "engine-off", "", "", "", "",
                                                  "binary_sensor");
    }

    if (OBD.isPidSupported(ENGINE_LOAD)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "load", "Engine Load", "engine", "%", "", "", "");
    }

    if (OBD.isPidSupported(THROTTLE_POSITION)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "throttle", "Throttle", "gauge", "%", "", "", "");
    }

    if (OBD.isPidSupported(ENGINE_COOLANT_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "coolantTemp", "Engine Coolant Temperature", "thermometer", "°C",
                                                  "temperature", "measurement", "");
    }

    if (OBD.isPidSupported(ENGINE_OIL_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "oilTemp", "Oil Temperature", "thermometer", "°C",
                                                  "temperature", "measurement", "");
    }

    if (OBD.isPidSupported(AMBIENT_AIR_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "ambientAirTemp", "Ambient Temperature", "thermometer",
                                                  "°C", "temperature", "measurement", "");
    }

    if (OBD.isPidSupported(INTAKE_AIR_TEMP)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "intakeAirTemp", "Intake Air Temperature", "thermometer", "°C",
                                                  "temperature", "measurement", "");
    }

    // if (OBD.isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
    //     allSendsSuccessed |= mqtt.sendTopicConfig("", "manifoldPressure", "Manifold Pressure", "", "kPa",
    //                                               "pressure", "measurement", "");
    // }

    if (OBD.isPidSupported(MAF_FLOW_RATE)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "mafRate", "Mass Air Flow", "air-filter", "g/s", "",
                                                  "measurement", "");
    }

    // if (OBD.isPidSupported(TIMING_ADVANCE)) {
    //     allSendsSuccessed |= mqtt.sendTopicConfig("", "timingAdvance", "Timing Advance", "axis-x-rotate-clockwise", "°",
    //                                               "", "measurement", "");
    // }

    if (OBD.isPidSupported(ENGINE_RPM)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "rpm", "Rounds per minute", "engine", "", "", "measurement", "");
    }

    if (OBD.isPidSupported(VEHICLE_SPEED)) {
        if (Settings.getMeasurementSystem() == METRIC) {
            allSendsSuccessed |= mqtt.sendTopicConfig("", "kph", "Kilometer per Hour", "speedometer", "km/h", "speed",
                                                      "measurement", "");
        } else {
            allSendsSuccessed |= mqtt.sendTopicConfig("", "mph", "Miles per Hour", "speedometer", "mph", "speed",
                                                      "measurement", "");
        }
    }

    if (OBD.isPidSupported(ENGINE_FUEL_RATE)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "fuelRate", "Fuel Rate", "fuel",
                                                  Settings.getMeasurementSystem() == METRIC ? "L/h" : "gal/h", "",
                                                  "measurement", "");
    }

    if (OBD.isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "fuelLevel", "Fuel Level", "fuel", "%", "", "measurement", "");
    }

    allSendsSuccessed |= mqtt.sendTopicConfig("", "batVoltage", "Battery Voltage", "battery", "V", "voltage",
                                              "measurement", "");

    if (OBD.isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "pedalPosition", "Pedal Position", "seat-recline-extra", "%", "",
                                                  "measurement", "");
    }

    allSendsSuccessed |= mqtt.sendTopicConfig("", "topSpeed", "Top Speed", "speedometer",
                                              Settings.getMeasurementSystem() == METRIC ? "km/h" : "mph", "speed",
                                              "measurement", "");

    // allSendsSuccessed |= mqtt.sendTopicConfig("", "curConsumption", "Calculated current consumption",
    //                                           "gas-station-outline", "l/100km", "", "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "consumption", "Calculated consumption",
                                              "gas-station", Settings.getMeasurementSystem() == METRIC ? "L" : "gal",
                                              "volume", "", "");

    if (Settings.getMeasurementSystem() == METRIC) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "consumptionPer100", "Calculated consumption per 100km",
                                                  "gas-station", "l/100km", "", "measurement", "");
    } else {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "mpg", "Calculated Miles per gallon",
                                                  "gas-station", "mpg", "", "measurement", "");
    }

    allSendsSuccessed |= mqtt.sendTopicConfig("", "distanceDriven", "Calculated driven distance",
                                              "map-marker-distance",
                                              Settings.getMeasurementSystem() == METRIC ? "km" : "mi", "distance",
                                              "measurement", "");
    allSendsSuccessed |= mqtt.sendTopicConfig("", "avgSpeed", "Calculated average speed",
                                              "speedometer-medium",
                                              Settings.getMeasurementSystem() == METRIC ? "km/h" : "mph", "speed",
                                              "measurement", "");

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

    DEBUG_PORT.print("Send static diagnostic discovery data...");;
    if (OBD.getSupportedPids1To20() != 0 || OBD.getSupportedPids21To40() != 0 || OBD.getSupportedPids41To60() != 0
        || OBD.getSupportedPids61To80() != 0) {
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_1_20", "Supported PIDs 1-20", "", "", "", "",
                                                  "diagnostic");
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_21_40", "Supported PIDs 21-40", "", "", "", "",
                                                  "diagnostic");
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_41_60", "Supported PIDs 41-60", "", "", "", "",
                                                  "diagnostic");
        allSendsSuccessed |= mqtt.sendTopicConfig("", "supportedPids_61_80", "Supported PIDs 61-80", "", "", "", "",
                                                  "diagnostic");
    }

    if (!OBD.vin().empty()) {
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

    if (OBD.isPidSupported(ENGINE_RPM)) {
        allSendsSuccessed |= mqtt.sendTopicUpdate("engineRunning", OBD.getRPM() > 300 ? "on" : "off");
    }

    if (OBD.isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED)) {
        allSendsSuccessed |= mqtt.sendTopicUpdate("mil", OBD.getMilState() ? "on" : "off");
    }

    if (OBD.isPidSupported(ENGINE_LOAD)) {
        sprintf(tmp_char, "%d", OBD.getLoad());
        allSendsSuccessed |= mqtt.sendTopicUpdate("load", std::string(tmp_char));
    }

    if (OBD.isPidSupported(THROTTLE_POSITION)) {
        sprintf(tmp_char, "%d", OBD.getThrottle());
        allSendsSuccessed |= mqtt.sendTopicUpdate("throttle", std::string(tmp_char));
    }

    if (OBD.isPidSupported(ENGINE_COOLANT_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getCoolantTemp()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("coolantTemp", std::string(tmp_char));
    }

    if (OBD.isPidSupported(ENGINE_OIL_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getOilTemp()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("oilTemp", std::string(tmp_char));
    }

    if (OBD.isPidSupported(AMBIENT_AIR_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getAmbientAirTemp()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("ambientAirTemp", std::string(tmp_char));
    }

    if (OBD.isPidSupported(INTAKE_AIR_TEMP)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getIntakeAirTemp()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("intakeAirTemp", std::string(tmp_char));
    }

    // if (OBD.isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
    //     sprintf(tmp_char, "%d", static_cast<int>(manifoldPressure));
    //     allSendsSuccessed |= mqtt.sendTopicUpdate("manifoldPressure", std::string(tmp_char));
    // }

    if (OBD.isPidSupported(MAF_FLOW_RATE)) {
        sprintf(tmp_char, "%4.2f", OBD.getMafRate());
        allSendsSuccessed |= mqtt.sendTopicUpdate("mafRate", std::string(tmp_char));
    }

    // if (OBD.isPidSupported(TIMING_ADVANCE)) {
    //     sprintf(tmp_char, "%d", static_cast<int>(timingAdvance));
    //     allSendsSuccessed |= mqtt.sendTopicUpdate("timingAdvance", std::string(tmp_char));
    // }

    if (OBD.isPidSupported(ENGINE_RPM)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getRPM()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("rpm", std::string(tmp_char));
    }

    if (OBD.isPidSupported(VEHICLE_SPEED)) {
        if (Settings.getMeasurementSystem() == METRIC) {
            sprintf(tmp_char, "%d", OBD.getSpeed());
            allSendsSuccessed |= mqtt.sendTopicUpdate("kph", std::string(tmp_char));
        } else {
            sprintf(tmp_char, "%d", OBD.getSpeed(IMPERIAL));
            allSendsSuccessed |= mqtt.sendTopicUpdate("mph", std::string(tmp_char));
        }
    }

    if (OBD.isPidSupported(ENGINE_FUEL_RATE)) {
        sprintf(tmp_char, "%4.2f", OBD.getFuelRate(static_cast<measurementSystem>(Settings.getMeasurementSystem())));
        allSendsSuccessed |= mqtt.sendTopicUpdate("fuelRate", std::string(tmp_char));
    }

    if (OBD.isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getFuelLevel()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("fuelLevel", std::string(tmp_char));
    }

    sprintf(tmp_char, "%4.2f", OBD.getBatVoltage());
    allSendsSuccessed |= mqtt.sendTopicUpdate("batVoltage", std::string(tmp_char));

    if (OBD.isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
        sprintf(tmp_char, "%d", static_cast<int>(OBD.getPedalPosition()));
        allSendsSuccessed |= mqtt.sendTopicUpdate("pedalPosition", std::string(tmp_char));
    }

    sprintf(tmp_char, "%d", OBD.getTopSpeed(static_cast<measurementSystem>(Settings.getMeasurementSystem())));
    allSendsSuccessed |= mqtt.sendTopicUpdate("topSpeed", std::string(tmp_char));

    // sprintf(tmp_char, "%4.2f", static_cast<float>(curConsumption));
    // allSendsSuccessed |= mqtt.sendTopicUpdate("curConsumption", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", OBD.getConsumption(static_cast<measurementSystem>(Settings.getMeasurementSystem())));
    allSendsSuccessed |= mqtt.sendTopicUpdate("consumption", std::string(tmp_char));

    if (Settings.getMeasurementSystem() == METRIC) {
        sprintf(tmp_char, "%4.2f", OBD.getConsumptionForMeasurement());
        allSendsSuccessed |= mqtt.sendTopicUpdate("consumptionPer100", std::string(tmp_char));
    } else {
        sprintf(tmp_char, "%4.2f",
                OBD.getConsumptionForMeasurement(static_cast<measurementSystem>(Settings.getMeasurementSystem())));
        allSendsSuccessed |= mqtt.sendTopicUpdate("mpg", std::string(tmp_char));
    }

    sprintf(tmp_char, "%4.2f", OBD.getDistanceDriven(static_cast<measurementSystem>(Settings.getMeasurementSystem())));
    allSendsSuccessed |= mqtt.sendTopicUpdate("distanceDriven", std::string(tmp_char));

    sprintf(tmp_char, "%4.2f", OBD.getAvgSpeed(static_cast<measurementSystem>(Settings.getMeasurementSystem())));
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

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendStaticDiagnosticData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    char tmp_char[50];
    DEBUG_PORT.print("Send static diagnostic data...");

    if (OBD.getSupportedPids1To20() != 0 || OBD.getSupportedPids21To40() != 0 || OBD.getSupportedPids41To60() != 0
        || OBD.getSupportedPids61To80() != 0) {
        sprintf(tmp_char, "%s", std::bitset<32>(OBD.getSupportedPids1To20()).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_1_20", std::string(tmp_char));

        sprintf(tmp_char, "%s", std::bitset<32>(OBD.getSupportedPids21To40()).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_21_40", std::string(tmp_char));

        sprintf(tmp_char, "%s", std::bitset<32>(OBD.getSupportedPids41To60()).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_41_60", std::string(tmp_char));

        sprintf(tmp_char, "%s", std::bitset<32>(OBD.getSupportedPids61To80()).to_string().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("supportedPids_61_80", std::string(tmp_char));
    } else {
        allSendsSuccessed = true;
    }

    if (!OBD.vin().empty()) {
        sprintf(tmp_char, "%s", OBD.vin().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate("VIN", std::string(tmp_char));
    }

    DEBUG_PORT.printf("...%s (%dms)\n", allSendsSuccessed ? "done" : "failed", millis() - start);

    return allSendsSuccessed;
}

bool sendLocationData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    DEBUG_PORT.print("Send location data...");

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
                lastMQTTDiscoveryOutput = millis() + Settings.getMQTTDiscoveryInterval() * 1000L;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTDiagnosticDiscoveryOutput) {
            allDiagnosticDiscoverySend = false;
        }

        if (!allDiagnosticDiscoverySend) {
            if ((allDiagnosticDiscoverySend = sendDiagnosticDiscoveryData())) {
                lastMQTTDiagnosticDiscoveryOutput = millis() + Settings.getMQTTDiscoveryInterval() * 1000L;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTStaticDiagnosticDiscoveryOutput) {
            allStaticDiagnosticDiscoverySend = false;
        }

        if (!allStaticDiagnosticDiscoverySend) {
            if ((allStaticDiagnosticDiscoverySend = sendStaticDiagnosticDiscoveryData())) {
                lastMQTTStaticDiagnosticDiscoveryOutput = millis() + Settings.getMQTTDiscoveryInterval() * 1000L;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTDiagnosticOutput) {
            if (sendDiagnosticData()) {
                lastMQTTDiagnosticOutput = millis() + Settings.getMQTTDiagnosticInterval() * 1000L;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTStaticDiagnosticOutput) {
            if (sendStaticDiagnosticData()) {
                lastMQTTStaticDiagnosticOutput = millis() + Settings.getMQTTDiagnosticInterval() * 2 * 1000L;
            } else {
                return;
            }
        }

        if (millis() > lastMQTTLocationOutput) {
            if (sendLocationData()) {
                lastMQTTLocationOutput = millis() + Settings.getMQTTLocationInterval() * 1000L;
            } else {
                return;
            }
        }

        if (sendOBDData()) {
            lastMQTTOutput = millis() + Settings.getMQTTDataInterval() * 1000L;
        }
    } else {
        delay(500);
    }
}

void readStatesTask(void *parameters) {
    for (;;) {
        if (!wifiAPInUse) {
            // readStates();
            OBD.loop();
        }
        delay(10);
    }
}

void outputTask(void *parameters) {
    unsigned long checkInterval = 0;
    for (;;) {
        if (!wifiAPInUse) {
            if (!gsm.checkNetwork()) {
                continue;
            }

            // debugOutputStates();

            if ((GSM::hasGSMLocation() || GSM::hasGPSLocation()) && millis() > checkInterval) {
                unsigned long start = millis();
                bool allReadSuccessed = false;

                DEBUG_PORT.print("Read location...");
                if (gsm.isNetworkConnected()) {
                    float gsm_latitude = 0;
                    float gsm_longitude = 0;
                    float gsm_accuracy = 0;

                    if (allReadSuccessed |= gsm.readGSMLocation(gsm_latitude, gsm_longitude, gsm_accuracy)) {
                        gsmLatitude = gsm_latitude;
                        gsmLongitude = gsm_longitude;
                        gsmAccuracy = gsm_accuracy;
                    }
                }

                if (GSM::hasGPSLocation()) {
                    float gps_latitude = 0;
                    float gps_longitude = 0;
                    float gps_accuracy = 0;

                    if (!(allReadSuccessed |= gsm.readGPSLocation(gps_latitude, gps_longitude, gps_accuracy))) {
                        gsm.checkGPS();
                    } else {
                        gpsLatitude = gps_latitude;
                        gpsLongitude = gps_longitude;
                        gpsAccuracy = gps_accuracy;
                    }
                }

                checkInterval = millis() + Settings.getMQTTLocationInterval() * 1000L;
                DEBUG_PORT.printf("...%s (%dms)\n", allReadSuccessed ? "done" : "failed", millis() - start);
            }

            if (GSM::isUseGPRS()) {
                signalQuality = gsm.getSignalQuality();
            }

            if (!mqtt.connected()) {
                auto client_id = String(MQTT_CLIENT_ID) + "-" + stripChars(OBD.getConnectedBTAddress()).c_str();
                if (!mqtt.connect(
                    client_id.c_str(),
                    Settings.getMQTTHostname().c_str(),
                    Settings.getMQTTPort(),
                    Settings.getMQTTUsername().c_str(),
                    Settings.getMQTTPassword().c_str(),
                    static_cast<mqttProtocol>(Settings.getMQTTProtocol())
                )) {
                    gsm.checkNetwork(true);
                }
            } else {
                mqttSendData();
            }
        }
        delay(100);
    }
}

void mqttTask(void *parameters) {
    for (;;) {
        if (!wifiAPInUse) {
            mqtt.loop();
        }
        delay(100);
    }
}

void setup() {
    startTime = millis();

    DEBUG_PORT.begin(115200);

    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        DEBUG_PORT.println("LittleFS Mount Failed");
        return;
    }

    Settings.readSettings(LittleFS);

    startWiFiAP();
    startHttpServer();

    gsm.connectToNetwork();
    gsm.enableGPS();

    OBD.begin(Settings.getOBD2Name(OBD_ADP_NAME), Settings.getOBD2MAC(), Settings.getOBD2Protocol(),
              Settings.getOBD2CheckPIDSupport());
    OBD.onDevicesDiscovered(onBTDevicesDiscovered);
    OBD.connect();

    if (!Settings.getMQTTHostname().isEmpty()) {
        mqtt.setIdentifier(!stripChars(OBD.vin()).empty() ? OBD.vin() : OBD.getConnectedBTAddress());

        // disable Watch Dog for Core 0 - should fix crashes
        disableCore0WDT();

        xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 8192, nullptr, 1, &mqttTaskHdl, 0);
        xTaskCreatePinnedToCore(outputTask, "OutputTask", 8192, nullptr, 10, &outputTaskHdl, 0);
    }

    xTaskCreatePinnedToCore(readStatesTask, "ReadStatesTask", 8192, nullptr, 1, &stateTaskHdl, 1);
}

void loop() {
    vTaskDelete(nullptr);
}
