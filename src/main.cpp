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

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED) && !defined(USE_BLE)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

#ifndef BUILD_GIT_BRANCH
#define BUILD_GIT_BRANCH ""
#endif
#ifndef BUILD_GIT_COMMIT_HASH
#define BUILD_GIT_COMMIT_HASH ""
#endif

#define MIN_VOLTAGE_LEVEL       3300
#define LOW_VOLTAGE_LEVEL       3600            // Sleep shutdown voltage

#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

#define DISCOVERED_DEVICES_FILE "/discovered_devices.json"

#define MIME_TYPE_JSON          "application/json"
#define MIME_TYPE_PLAIN         "text/plain"

#define HA_T_CPUTEMP            "cpuTemp"
#define HA_T_FREEMEM            "freeMem"
#define HA_T_UPTIME             "uptime"
#define HA_T_RECONNECTS         "reconnects"
#define HA_T_IP_ADDR            "ipAddress"
#define HA_T_SQ                 "signalQuality"
#define HA_T_BAT_VOL            "internalBatteryVoltage"
#define HA_T_GSM_LOC            "gsmLocation"
#define HA_T_GPS_LOC            "gpsLocation"
#define HAT_T_DTC               "dtc"
#define HAT_T_CLEAR_DTC         "clearDTC"

#include <numeric>

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

MQTT mqtt = MQTT();

std::atomic_bool wifiAPStarted{false};
std::atomic_bool wifiAPInUse{false};
std::atomic<unsigned int> wifiAPStaConnected{0};

std::atomic_bool obdConnected{false};
std::atomic<int> obdConnectErrors{0};

std::atomic<unsigned long> startTime{0};

std::vector<uint32_t> batteryVoltages;
std::atomic<unsigned long> batteryTime{0};

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
std::atomic<unsigned long> lastMQTTDTCDiagnosticOutput{0};
std::atomic<unsigned long> lastMQTTLocationOutput{0};

std::atomic<int> signalQuality{0};
std::atomic<float> gsmLatitude{0};
std::atomic<float> gsmLongitude{0};
std::atomic<float> gsmAccuracy{0};
std::atomic<float> gpsLatitude{0};
std::atomic<float> gpsLongitude{0};
std::atomic<float> gpsAccuracy{0};

std::atomic_bool clearDTC{false};

TaskHandle_t outputTaskHdl;
TaskHandle_t stateTaskHdl;

size_t getESPHeapSize() {
    return heap_caps_get_free_size(MALLOC_CAP_8BIT);
}

void deepSleep(const int sec) {
    log_d("Prepare nap...");
    WiFi.disconnect(true);
    OBD.end();
    gsm.powerOff();
    if (outputTaskHdl != nullptr) {
        vTaskDelete(outputTaskHdl);
    }
    if (stateTaskHdl != nullptr) {
        vTaskDelete(stateTaskHdl);
    }
    log_d("...ZzZzZz.");
    GSM::deepSleep(sec * 1000);
}

void consoleSendHeader(const char *str) {
    DEBUG_PORT.printf("Send %s data...", str);
}

void consoleSendFooter(const bool success, const unsigned long time) {
    DEBUG_PORT.printf("...%s (%lums)\n", success ? "done" : "failed", time);
}

std::string buildDTCPayload(DTCs *dtcs) {
    JsonDocument doc;
    JsonArray a = doc["dtc"].to<JsonArray>();
    for (int i = 0; i < dtcs->getCount(); ++i) {
        a.add(dtcs->getCode(i)->c_str());
    }
    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

void WiFiAPStart(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiAPStarted = true;
    DEBUG_PORT.println("AP started.");

    DEBUG_PORT.printf("AP - IP address: %s\n", WiFi.softAPIP().toString().c_str());
}

void WiFiAPStop(WiFiEvent_t event, WiFiEventInfo_t info) {
    wifiAPStarted = false;
    wifiAPInUse = false;
    wifiAPStaConnected = 0;
    DEBUG_PORT.println("AP stopped.");
}

void WiFiAPStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    ++wifiAPStaConnected;
    wifiAPInUse = true;

    if (wifiAPStaConnected == 1) {
        DEBUG_PORT.println("AP in use.");
        OBD.end();
    }
}

void WiFiAPStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (wifiAPStaConnected != 0) {
        --wifiAPStaConnected;
    }

    if (wifiAPStaConnected == 0) {
        DEBUG_PORT.println("AP all clients disconnected.");
        OBD.begin(Settings.OBD2.getName(OBD_ADP_NAME), Settings.OBD2.getMAC(), Settings.OBD2.getProtocol(),
                  Settings.OBD2.getCheckPIDSupport(), Settings.OBD2.getDebug(), Settings.OBD2.getSpecifyNumResponses());
        OBD.connect(true);
        wifiAPInUse = false;
    }
}

void startWiFiAP() {
    DEBUG_PORT.print("Start AP...");

    WiFi.disconnect(true);

    WiFi.mode(WIFI_AP);

    WiFi.onEvent(WiFiAPStart, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_START);
    WiFi.onEvent(WiFiAPStop, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STOP);
    WiFi.onEvent(WiFiAPStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent(WiFiAPStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

    String ssid = Settings.WiFi.getAPSSID();
    if (ssid.isEmpty()) {
        ssid = "OBD2-MQTT-" + String(stripChars(WiFi.macAddress().c_str()).c_str());
        Settings.WiFi.setAPSSID(ssid.c_str());
    }
    WiFi.softAP(
        ssid.c_str(),
        Settings.WiFi.getAPPassword()
    );
}

void startHttpServer() {
    server.on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, MIME_TYPE_PLAIN, getVersion());
    });

    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, MIME_TYPE_JSON, Settings.buildJson().c_str());
    });

    server.on(
        "/api/settings",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (request->contentType() == MIME_TYPE_JSON) {
                if (!index) {
                    request->_tempObject = malloc(total);
                }

                if (request->_tempObject != nullptr) {
                    memcpy(static_cast<uint8_t *>(request->_tempObject) + index, data, len);

                    if (index + len == total) {
                        auto json = std::string(static_cast<const char *>(request->_tempObject), total);
                        if (Settings.parseJson(json)) {
                            if (Settings.writeSettings(LittleFS)) {
                                request->send(200);
                            }
                        } else {
                            request->send(500);
                        }
                    }
                }
            } else {
                request->send(406);
            }
        }
    );

    server.on("/api/states", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, MIME_TYPE_JSON, OBD.buildJSON().c_str());
    });

    server.on(
        "/api/states",
        HTTP_PUT,
        [](AsyncWebServerRequest *request) {
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (request->contentType() == MIME_TYPE_JSON) {
                if (!index) {
                    request->_tempObject = malloc(total);
                }

                if (request->_tempObject != nullptr) {
                    memcpy(static_cast<uint8_t *>(request->_tempObject) + index, data, len);

                    if (index + len == total) {
                        auto json = std::string(static_cast<const char *>(request->_tempObject), total);
                        if (OBD.parseJSON(json)) {
                            if (OBD.writeStates(LittleFS)) {
                                request->send(200);
                            }
                        } else {
                            request->send(500);
                        }
                    }
                }
            } else {
                request->send(406);
            }
        }
    );

    server.on("/api/hasBattery", HTTP_GET, [](AsyncWebServerRequest *request) {
        std::string payload;
        JsonDocument doc;

        doc["hasBattery"] = GSM::hasBattery();

        serializeJson(doc, payload);

        request->send(200, MIME_TYPE_JSON, payload.c_str());
    });

    server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        std::string payload;
        JsonDocument wifiInfo;

        wifiInfo["hostname"] = WiFi.softAPgetHostname();
        wifiInfo["SSID"] = WiFi.softAPSSID();
        wifiInfo["ip"] = WiFi.softAPIP().toString();
        wifiInfo["mac"] = WiFi.macAddress();

        serializeJson(wifiInfo, payload);

        request->send(200, MIME_TYPE_JSON, payload.c_str());
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

        request->send(200, MIME_TYPE_JSON, payload.c_str());
    });

    server.on("/api/discoveredDevices", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = LittleFS.open(DISCOVERED_DEVICES_FILE, FILE_READ);
        if (file && !file.isDirectory()) {
            JsonDocument doc;
            if (!deserializeJson(doc, file)) {
                std::string payload;
                serializeJson(doc, payload);
                request->send(200, MIME_TYPE_JSON, payload.c_str());
            } else {
                request->send(500);
            }
            file.close();
        } else {
            request->send(404);
        }
    });

    server.on("/api/DTCs", HTTP_GET, [](AsyncWebServerRequest *request) {
        DTCs *dtcs = OBD.getDTCs();
        if (dtcs != nullptr && dtcs->getCount() != 0) {
            request->send(200, MIME_TYPE_JSON, buildDTCPayload(dtcs).c_str());
        } else {
            request->send(404);
        }
    });

    server.begin(LittleFS);
}

void onOBDConnected() {
    obdConnected = true;
    obdConnectErrors = 0;
}

void onOBDConnectError() {
    obdConnected = false;
    ++obdConnectErrors;
    if (GSM::hasBattery() && GSM::isBatteryUsed() && obdConnectErrors > 5) {
        deepSleep(Settings.General.getSleepDuration());
    }
}

#ifdef USE_BLE
void onBLEDevicesDiscovered(BLEScanResultsSet *btDeviceList) {
    JsonDocument devices;

    File file = LittleFS.open(DISCOVERED_DEVICES_FILE, FILE_WRITE);
    if (!file) {
        log_d("Failed to open file discovered_devices.json for writing.");
        return;
    }

    for (int i = 0; i < btDeviceList->getCount(); i++) {
        JsonDocument dev;
        BLEAdvertisedDevice *device = btDeviceList->getDevice(i);
        if (device && !device->getName().empty()) {
            dev["name"] = device->getName();
            dev["mac"] = device->getAddress().toString();
            devices["device"].add(dev);
        }
    }

    serializeJson(devices, file);

    file.close();
}

#else
void onBTDevicesDiscovered(BTScanResults *btDeviceList) {
    JsonDocument devices;

    File file = LittleFS.open(DISCOVERED_DEVICES_FILE, FILE_WRITE);
    if (!file) {
        log_d("Failed to open file discovered_devices.json for writing.");
        return;
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
#endif

bool sendDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;
    bool allowOffline = Settings.MQTT.getAllowOffline();

    consoleSendHeader("discovery");

    std::vector<OBDState *> states{};
    OBD.getStates([](const OBDState *state) {
        return state->isVisible() && state->isEnabled() && state->isSupported() && !(
                   state->isDiagnostic() && state->getUpdateInterval() == -1);
    }, states);
    if (!states.empty()) {
        for (auto &state: states) {
            allSendsSuccessed |= mqtt.sendTopicConfig(state->getName(), state->getDescription(), state->getIcon(),
                                                      state->getUnit(), state->getDeviceClass(),
                                                      state->isMeasurement() ? SC_MEASUREMENT : "",
                                                      state->isDiagnostic() ? EC_DIAGNOSTIC : "",
                                                      state->valueType() == "bool" ? TT_B_SENSOR : TT_SENSOR,
                                                      "", allowOffline);
        }
    } else {
        allSendsSuccessed = true;
    }

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

bool sendDiagnosticDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    consoleSendHeader("diagnostic discovery");

    allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_CPUTEMP, "CPU Temperature", "thermometer", "°C", "temperature",
                                              SC_MEASUREMENT, EC_DIAGNOSTIC);
    allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_FREEMEM, "Free Memory", "memory", "B", "", SC_MEASUREMENT,
                                              EC_DIAGNOSTIC);
    allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_UPTIME, "Uptime", "timer-play", "sec", "", SC_MEASUREMENT,
                                              EC_DIAGNOSTIC);
    allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_RECONNECTS, "Number of reconnects", "connection", "", "",
                                              SC_MEASUREMENT, EC_DIAGNOSTIC);

    if (!gsm.getIpAddress().empty()) {
        allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_IP_ADDR, "IP Address", "network-outline", "", "", "",
                                                  EC_DIAGNOSTIC);
    }

    if (GSM::isUseGPRS()) {
        allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_SQ, "Signal Quality", "signal", "dBm",
                                                  "signal_strength", "", EC_DIAGNOSTIC);
    }

    if (GSM::hasGSMLocation()) {
        allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_GSM_LOC, "GSM Location", "crosshairs-gps", "", "", "",
                                                  EC_DIAGNOSTIC, TT_D_TRACKER, "gps", true);
    }

    if (GSM::hasGPSLocation()) {
        allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_GPS_LOC, "GPS Location", "crosshairs-gps", "", "", "",
                                                  EC_DIAGNOSTIC, TT_D_TRACKER, "gps", true);
    }

    if (GSM::hasBattery()) {
        allSendsSuccessed |= mqtt.sendTopicConfig(HA_T_BAT_VOL, "Internal Battery Voltage", "battery",
                                                  "mV", "voltage", "", EC_DIAGNOSTIC);
    }

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

bool sendStaticDiagnosticDiscoveryData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    consoleSendHeader("static diagnostic discovery");

    std::vector<OBDState *> states{};
    OBD.getStates([](const OBDState *state) {
        return state->isVisible() && state->isEnabled() && state->isSupported() && state->isDiagnostic() && state->
               getUpdateInterval() == -1;
    }, states);
    if (!states.empty()) {
        for (auto &state: states) {
            allSendsSuccessed |= mqtt.sendTopicConfig(state->getName(), state->getDescription(), state->getIcon(),
                                                      state->getUnit(), state->getDeviceClass(),
                                                      state->isMeasurement() ? SC_MEASUREMENT : "",
                                                      state->isDiagnostic() ? EC_DIAGNOSTIC : "",
                                                      state->valueType() == "bool" ? TT_B_SENSOR : TT_SENSOR);
        }
    } else {
        allSendsSuccessed = true;
    }

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

bool sendStates(std::vector<OBDState *> &states, bool allSendsSuccessed) {
    if (!states.empty()) {
        for (auto &state: states) {
            const size_t len = OBD.getPayloadLength() < 64 ? 64 : OBD.getPayloadLength() + 1;
            char tmp_char[len];
            if (state->getLastUpdate() + state->getUpdateInterval() > millis()) {
                continue;
            }

            if (state->valueType() == "int") {
                auto *is = reinterpret_cast<OBDStateInt *>(state);
                char *str = is->formatValue();
                strncpy(tmp_char, str, len);
                free(str);
            } else if (state->valueType() == "float") {
                auto *is = reinterpret_cast<OBDStateFloat *>(state);
                char *str = is->formatValue();
                strncpy(tmp_char, str, len);
                free(str);
            } else if (state->valueType() == "bool") {
                auto *is = reinterpret_cast<OBDStateBool *>(state);
                char *str = is->formatValue();
                strncpy(tmp_char, str, len);
                free(str);
            }

            allSendsSuccessed |= mqtt.sendTopicUpdate(state->getName(), std::string(tmp_char));
        }
    } else {
        allSendsSuccessed = true;
    }

    return allSendsSuccessed;
}

bool sendOBDData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    consoleSendHeader("OBD");

    allSendsSuccessed |= mqtt.sendTopicUpdate(LWT_TOPIC, LWT_CONNECTED);

    std::vector<OBDState *> states{};
    OBD.getStates([](const OBDState *state) {
        return state->isVisible() && state->isEnabled() && state->isSupported() && !(
                   state->isDiagnostic() && state->getUpdateInterval() == -1);
    }, states);
    allSendsSuccessed = sendStates(states, allSendsSuccessed);

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

bool sendDiagnosticData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;
    char tmp_char[50];

    consoleSendHeader("diagnostic");

    sprintf(tmp_char, "%d", static_cast<int>(temperatureRead()));
    allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_CPUTEMP, std::string(tmp_char));

    sprintf(tmp_char, "%lu", static_cast<long>(getESPHeapSize()));
    allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_FREEMEM, std::string(tmp_char));

    sprintf(tmp_char, "%lu", (millis() - startTime) / 1000);
    allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_UPTIME, std::string(tmp_char));

    sprintf(tmp_char, "%d", mqtt.reconnectAttemps());
    allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_RECONNECTS, std::string(tmp_char));

    if (!gsm.getIpAddress().empty()) {
        sprintf(tmp_char, "%s", gsm.getIpAddress().c_str());
        allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_IP_ADDR, std::string(tmp_char));
    }

    if (GSM::isUseGPRS() && signalQuality != SQ_NOT_KNOWN) {
        sprintf(tmp_char, "%d", GSM::convertSQToRSSI(signalQuality));
        allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_SQ, std::string(tmp_char));
    }

    if (GSM::hasBattery()) {
        sprintf(tmp_char, "%d", GSM::getBatteryVoltage());
        allSendsSuccessed |= mqtt.sendTopicUpdate(HA_T_BAT_VOL, std::string(tmp_char));
    }

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

bool sendStaticDiagnosticData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    consoleSendHeader("static diagnostic");

    std::vector<OBDState *> states{};
    OBD.getStates([](const OBDState *state) {
        return state->isVisible() && state->isEnabled() && state->isSupported() && state->isDiagnostic() && state->
               getUpdateInterval() == -1;
    }, states);
    allSendsSuccessed = sendStates(states, allSendsSuccessed);

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

bool sendDTCDiagnosticData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    consoleSendHeader("DTC diagnostic");

    DTCs *dtcs = OBD.getDTCs();
    if (dtcs != nullptr) {
        allSendsSuccessed |= mqtt.sendTopicConfig(HAT_T_DTC, "DTC", "engine",
                                                  "", "", "", EC_DIAGNOSTIC, TT_SENSOR, "", false,
                                                  "{{ value_json.dtc | join(\",\") }}");
        if (dtcs->getCount() != 0) {
            allSendsSuccessed |= mqtt.sendTopicConfig(HAT_T_CLEAR_DTC, "Clear DTC", "",
                                                      "", "", "", EC_DIAGNOSTIC, TT_BUTTON, "", false);

            mqtt.subscribe(HAT_T_CLEAR_DTC, [](const char *) {
                clearDTC = true;
            });
        }

        allSendsSuccessed |= mqtt.sendTopicUpdate(HAT_T_DTC, buildDTCPayload(dtcs));
    } else {
        allSendsSuccessed = true;
    }

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

std::string buildLocationAttrib(const float lat, const float lon, const float acc) {
    std::string payload;
    JsonDocument attribs;

    attribs["latitude"] = lat;
    attribs["longitude"] = lon;
    attribs["gps_accuracy"] = acc;

    serializeJson(attribs, payload);

    return payload;
}

bool sendLocationData() {
    const unsigned long start = millis();
    bool allSendsSuccessed = false;

    consoleSendHeader("location");

    if (GSM::hasGSMLocation()) {
        allSendsSuccessed |= mqtt.sendTopicUpdate(
            HA_T_GSM_LOC,
            buildLocationAttrib(gsmLatitude, gsmLongitude, gsmAccuracy),
            true
        );
    }
    if (GSM::hasGPSLocation()) {
        allSendsSuccessed |= mqtt.sendTopicUpdate(
            HA_T_GPS_LOC,
            buildLocationAttrib(gpsLatitude, gpsLongitude, gpsAccuracy),
            true
        );
    }

    consoleSendFooter(allSendsSuccessed, millis() - start);

    return allSendsSuccessed;
}

unsigned long calcTimestamp(const unsigned int interval) {
    return millis() + interval * 1000L;
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
                lastMQTTDiscoveryOutput = calcTimestamp(Settings.MQTT.getDiscoveryInterval());
            } else {
                return;
            }
        }

        if (millis() > lastMQTTDiagnosticDiscoveryOutput) {
            allDiagnosticDiscoverySend = false;
        }

        if (!allDiagnosticDiscoverySend) {
            if ((allDiagnosticDiscoverySend = sendDiagnosticDiscoveryData())) {
                lastMQTTDiagnosticDiscoveryOutput = calcTimestamp(Settings.MQTT.getDiscoveryInterval());
            } else {
                return;
            }
        }

        if (millis() > lastMQTTStaticDiagnosticDiscoveryOutput) {
            allStaticDiagnosticDiscoverySend = false;
        }

        if (!allStaticDiagnosticDiscoverySend) {
            if ((allStaticDiagnosticDiscoverySend = sendStaticDiagnosticDiscoveryData())) {
                lastMQTTStaticDiagnosticDiscoveryOutput = calcTimestamp(Settings.MQTT.getDiscoveryInterval());
            } else {
                return;
            }
        }

        if (millis() > lastMQTTLocationOutput) {
            if (sendLocationData()) {
                lastMQTTLocationOutput = calcTimestamp(Settings.MQTT.getLocationInterval());
            } else {
                return;
            }
        }

        if (millis() > lastMQTTDiagnosticOutput) {
            if (sendDiagnosticData()) {
                lastMQTTDiagnosticOutput = calcTimestamp(Settings.MQTT.getDiagnosticInterval());
            } else {
                return;
            }
        }

        if (obdConnected) {
            if (millis() > lastMQTTStaticDiagnosticOutput) {
                if (sendStaticDiagnosticData()) {
                    lastMQTTStaticDiagnosticOutput = calcTimestamp(Settings.MQTT.getDiagnosticInterval() * 2);
                } else {
                    return;
                }
            }

            if (millis() > lastMQTTDTCDiagnosticOutput) {
                if (sendDTCDiagnosticData()) {
                    lastMQTTDTCDiagnosticOutput = calcTimestamp(Settings.MQTT.getDiagnosticInterval());
                } else {
                    return;
                }
            }

            if (sendOBDData()) {
                lastMQTTOutput = calcTimestamp(Settings.MQTT.getDataInterval());
            }
        } else if (mqtt.sendTopicUpdate(LWT_TOPIC, LWT_CONNECTED)) {
            lastMQTTOutput = calcTimestamp(Settings.MQTT.getDataInterval() * 2);
        }
    } else {
        delay(500);
    }
}

[[noreturn]] void readStatesTask(void *parameters) {
    for (;;) {
        if (!wifiAPInUse) {
            if (clearDTC) {
                DEBUG_PORT.print("DTC reset ");
                if (OBD.resetDTCs()) {
                    DEBUG_PORT.println("done.");
                } else {
                    DEBUG_PORT.println("failed.");
                }
                clearDTC = false;
            }

            OBD.loop();
        }
        delay(10);
    }
}

[[noreturn]] void outputTask(void *parameters) {
    unsigned long checkInterval = 0;
    for (;;) {
        if (!wifiAPInUse) {
            if (GSM::hasBattery() && GSM::isBatteryUsed()) {
                const unsigned int batVoltage = GSM::getBatteryVoltage();
                if (batVoltage > MIN_VOLTAGE_LEVEL) {
                    const int sum = std::accumulate(batteryVoltages.begin(), batteryVoltages.end(), 0);
                    const double avgBat = static_cast<double>(sum) / batteryVoltages.size();
                    if (millis() > batteryTime + 5000) {
                        if (batteryVoltages.size() > 10) {
                            batteryVoltages.erase(batteryVoltages.begin());
                        }
                        batteryVoltages.push_back(batVoltage);
                        batteryTime = millis();
                    }
                    const bool drain = batteryVoltages.size() > 10 && batVoltage < (avgBat - 10);

                    const double avgLU = OBD.avgLastUpdate([](const OBDState *state) {
                        return state->isEnabled() &&
                               state->getType() == obd::READ && state->getUpdateInterval() > 0 &&
                               state->getUpdateInterval() <= 5 * 60 * 1000;
                    });

                    if (batVoltage < LOW_VOLTAGE_LEVEL || (
                            drain && avgLU > Settings.General.getSleepTimeout() * 1000)) {
                        if (batVoltage < LOW_VOLTAGE_LEVEL) {
                            log_d("Battery has low voltage.");
                        }
                        deepSleep(Settings.General.getSleepDuration());
                    }
                }
            }

            if (!gsm.checkNetwork()) {
                continue;
            }

            mqtt.loop();

            if ((GSM::hasGSMLocation() || GSM::hasGPSLocation()) && millis() > checkInterval) {
                unsigned long start = millis();
                bool allReadSuccessed = false;

                DEBUG_PORT.print("Read location...");
                if (gsm.isNetworkConnected()) {
                    float gsm_latitude = 0;
                    float gsm_longitude = 0;
                    float gsm_accuracy = 0;

                    if ((allReadSuccessed |= gsm.readGSMLocation(gsm_latitude, gsm_longitude, gsm_accuracy))) {
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

                checkInterval = millis() + Settings.MQTT.getLocationInterval() * 1000L;
                consoleSendFooter(allReadSuccessed, millis() - start);
            }

            if (GSM::isUseGPRS()) {
                signalQuality = gsm.getSignalQuality();
            }

            if (!mqtt.connected()) {
                auto client_id = String(MQTT_CLIENT_ID) + "-" + stripChars(mqtt.getIdentifier()).c_str();
                if (!mqtt.connect(
                    client_id.c_str(),
                    Settings.MQTT.getHostname().c_str(),
                    Settings.MQTT.getPort(),
                    Settings.MQTT.getUsername().c_str(),
                    Settings.MQTT.getPassword().c_str(),
                    static_cast<mqttProtocol>(Settings.MQTT.getProtocol())
                )) {
                    gsm.checkNetwork(true);
                }
            } else {
                mqttSendData();
            }
        }
        delay(50);
    }
}

String buildIdentifier(const char *devMac) {
    String mID = "";
    if (static_cast<MQTTSettings::MQTTIdentifierType>(Settings.MQTT.getIdType()) ==
        MQTTSettings::MQTTIdentifierType::CUSTOM && Settings.MQTT.getIdSuffix().length() > 0) {
        mID = Settings.MQTT.getIdSuffix().c_str();
    } else if (devMac != nullptr && strlen(devMac) > 0) {
        mID = devMac;
        if (static_cast<MQTTSettings::MQTTIdentifierType>(Settings.MQTT.getIdType()) ==
            MQTTSettings::MQTTIdentifierType::MAC_IMEI) {
            mID += "-";
            mID += gsm.modem.getIMEI().substring(gsm.modem.getIMEI().length() - 4).c_str();
        }
    }
    return mID;
}

void startOutputTask(const char *id) {
    if (!Settings.MQTT.getHostname().isEmpty()) {
        mqtt.setClient(gsm.getClient(Settings.MQTT.getSecure()));
        mqtt.setIdentifier(id);

        xTaskCreatePinnedToCore(outputTask, "OutputTask", 9216, nullptr, 10, &outputTaskHdl, 0);
    }
}

void startReadTask() {
#ifdef USE_BLE
    OBD.onDevicesDiscovered(onBLEDevicesDiscovered);
#else
    OBD.onDevicesDiscovered(onBTDevicesDiscovered);
#endif
    OBD.connect();

    xTaskCreatePinnedToCore(readStatesTask, "ReadStatesTask", 9216, nullptr, 1, &stateTaskHdl, 1);
}

void setup() {
    startTime = millis();

    DEBUG_PORT.begin(115200);

    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        log_d("LittleFS Mount Failed");
        return;
    }

    // init the coprozessor
    GSM::ulpInit();

    Settings.readSettings(LittleFS);
    OBD.readStates(LittleFS);

    // disable Watch Dog for Core 0 - should fix crashes
    disableCore0WDT();

    startWiFiAP();
    startHttpServer();

    // will be ignored if the device does not support
    gsm.setNetworkMode(Settings.Mobile.getNetworkMode());
    gsm.connectToNetwork();
    gsm.enableGPS();

    OBD.onConnected(onOBDConnected);
    OBD.onConnectError(onOBDConnectError);
    OBD.begin(Settings.OBD2.getName(OBD_ADP_NAME), Settings.OBD2.getMAC(), Settings.OBD2.getProtocol(),
              Settings.OBD2.getCheckPIDSupport(), Settings.OBD2.getDebug(), Settings.OBD2.getSpecifyNumResponses());

    String mID = buildIdentifier(Settings.OBD2.getMAC().c_str());
    if (!mID.isEmpty()) {
        startOutputTask(mID.c_str());
        startReadTask();
    } else {
        startReadTask();
        mID = buildIdentifier(OBD.getConnectedBTAddress().c_str());
        startOutputTask(mID.c_str());
    }
}

void loop() {
    vTaskDelete(nullptr);
}
