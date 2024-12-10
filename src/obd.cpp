/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program, in a file called gpl.txt or license.txt.
 *  If not, write to the Free Software Foundation Inc.,
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */

#include "obd.h"

#include <OBDStates.h>
#include <ExprParser.h>
#include "helper.h"

OBDClass::OBDClass(): OBDStates(&elm327), elm327() {
    protocol = AUTOMATIC;

    addCustomFunction("afRatio", [](const double fuelType) {
        switch (static_cast<int>(fuelType)) {
            case FUEL_TYPE_METHANOL:
                return AF_RATIO_METHANOL;
            case FUEL_TYPE_ETHANOL:
                return AF_RATIO_ETHANOL;
            case FUEL_TYPE_DIESEL:
                return AF_RATIO_DIESEL;
            case FUEL_TYPE_LPG:
            case FUEL_TYPE_CNG:
                return AF_RATIO_GAS;
            case FUEL_TYPE_PROPANE:
                return AF_RATIO_PROPANE;
            case FUEL_TYPE_ELECTRIC:
                return 0.0;
            default:
                return AF_RATIO_GASOLINE;
        }
    });
    addCustomFunction("density", [](const double fuelType)-> double {
        switch (static_cast<int>(fuelType)) {
            case FUEL_TYPE_METHANOL:
                return DENSITY_METHANOL;
            case FUEL_TYPE_ETHANOL:
                return DENSITY_ETHANOL;
            case FUEL_TYPE_DIESEL:
                return DENSITY_DIESEL;
            case FUEL_TYPE_LPG:
            case FUEL_TYPE_CNG:
                return DENSITY_GAS;
            case FUEL_TYPE_PROPANE:
                return DENSITY_PROPANE;
            case FUEL_TYPE_ELECTRIC:
                return 0.0;
            default:
                return DENSITY_GASOLINE;
        }
    });
    addCustomFunction("numDTCs", [&](const double numCodes)-> double {
        int numDTCs = 0;
        if (static_cast<u_int8_t>(numCodes) > 0) {
            elm327.currentDTCCodes();
            if (elm327.nb_rx_state == ELM_SUCCESS) {
                numDTCs = static_cast<int>(elm327.DTC_Response.codesFound);
                if (numDTCs > 0) {
                    Serial.println("\nDTCs Found: ");
                    for (int i = 0; i < numDTCs; i++) {
                        Serial.println(elm327.DTC_Response.codes[i]);
                    }
                }
            }
        }
        return numDTCs;
    });

    setVariableResolveFunction([&](const char *varName)-> double {
        if (varName != nullptr) {
            if (varName[0] == '$') {
                varName++;
            }

            if (std::strcmp(varName, "millis") == 0) {
                return millis();
            }
            size_t pos = 0;
            string vstr = varName;
            if ((pos = vstr.find('.')) != string::npos) {
                string vn = vstr.substr(0, pos);
                string op = vstr.substr(pos + 1);

                auto *state = getStateByName(vn.c_str());
                if (state != nullptr) {
                    if (op == "pu") {
                        return state->getPreviousUpdate();
                    }
                    if (op == "lu") {
                        return state->getLastUpdate();
                    }

                    if (op == "ov" ||
                        op == "a" || op == "b" || op == "c" || op == "d"
                        && state->valueType() == "int") {
                        auto *is = reinterpret_cast<OBDStateInt *>(state);
                        if (op == "a")
                            return is->getValue() & 0xFF;
                        if (op == "b")
                            return (is->getValue() >> 8) & 0xFF;
                        if (op == "c")
                            return (is->getValue() >> 16) & 0xFF;
                        if (op == "d")
                            return (is->getValue() >> 24) & 0xFF;
                        if (op == "ov")
                            return is->getOldValue();
                    }
                    if (op == "ov" && state->valueType() == "float") {
                        auto *is = reinterpret_cast<OBDStateFloat *>(state);
                        return is->getOldValue();
                    }
                    if (op == "ov" && state->valueType() == "bool") {
                        auto *is = reinterpret_cast<OBDStateBool *>(state);
                        return is->getOldValue();
                    }
                }
            } else {
                return getStateValue(varName);
            }
        }

        return 0.0;
    });
}

bool OBDClass::parseJSON(std::string &json) {
    bool success = false;
    JsonDocument doc;
    if (!deserializeJson(doc, json)) {
        readJSON(doc);
        success = true;
    }

    return success;
}

template<typename T>
void OBDClass::fromJSON(T *state, JsonDocument &doc) {
    state->setEnabled(doc["enabled"].as<bool>());
    state->setVisible(doc["visible"].as<bool>());

    if (state->getType() == READ) {
        if (!doc["readFunc"].isNull()) {
            setReadFuncByName<T>(doc["readFunc"].as<std::string>().c_str(), state);
        } else if (!doc["pid"].isNull()) {
            state->setPIDSettings(
                doc["pid"]["service"].as<uint8_t>(),
                doc["pid"]["pid"].as<uint16_t>(),
                doc["pid"]["numResponses"].as<uint8_t>(),
                doc["pid"]["numExpectedBytes"].as<uint8_t>(),
                !doc["pid"]["scaleFactor"].isNull() ? doc["pid"]["scaleFactor"].as<std::string>().c_str() : "1",
                doc["pid"]["bias"].as<float>()
            );
        }
    } else if (state->getType() == CALC) {
        if (!doc["calcExpression"].isNull()) {
            state->setCalcExpression(doc["calcExpression"].as<std::string>().c_str());
        }
    }

    if (!doc["value"]["format"].isNull()) {
        state->setValueFormat(doc["value"]["format"].as<std::string>().c_str());
    }
    if (!doc["value"]["func"].isNull()) {
        setFormatFuncByName<T>(doc["value"]["func"].as<std::string>().c_str(), state);
    } else if (!doc["value"]["expression"].isNull()) {
        state->setValueFormatExpression(doc["value"]["expression"].as<std::string>().c_str());
    }

    // is reset by setPIDSettings
    state->setUpdateInterval(doc["updateInterval"].as<long>());
}

bool OBDClass::readStates(FS &fs) {
    bool success = false;

    File file = fs.open(STATES_FILE, FILE_READ);
    if (file && !file.isDirectory()) {
        JsonDocument doc;
        if (!deserializeJson(doc, file)) {
            readJSON(doc);
            success = true;
        }
        file.close();
    }

    return success;
}

std::string OBDClass::buildJSON() {
    std::string payload;

    JsonDocument doc;
    writeJSON(doc);
    serializeJson(doc, payload);

    return payload;
}

void OBDClass::readJSON(JsonDocument &doc) {
    clearStates();
    const JsonArray array = doc.as<JsonArray>();
    for (JsonDocument stateObj: array) {
        if (stateObj["valueType"] == "bool") {
            auto *state = new OBDStateBool(
                stateObj["type"].as<OBDStateType>(),
                stateObj["name"].as<std::string>().c_str(),
                stateObj["description"].as<std::string>().c_str(),
                !stateObj["icon"].isNull() ? stateObj["icon"].as<std::string>().c_str() : "",
                !stateObj["unit"].isNull() ? stateObj["unit"].as<std::string>().c_str() : "",
                !stateObj["deviceClass"].isNull() ? stateObj["deviceClass"].as<std::string>().c_str() : "",
                stateObj["measurement"].as<bool>(),
                stateObj["diagnostic"].as<bool>()
            );
            fromJSON(state, stateObj);
            addState(state);
        } else if (stateObj["valueType"] == "float") {
            auto *state = new OBDStateFloat(
                stateObj["type"].as<OBDStateType>(),
                stateObj["name"].as<std::string>().c_str(),
                stateObj["description"].as<std::string>().c_str(),
                !stateObj["icon"].isNull() ? stateObj["icon"].as<std::string>().c_str() : "",
                !stateObj["unit"].isNull() ? stateObj["unit"].as<std::string>().c_str() : "",
                !stateObj["deviceClass"].isNull() ? stateObj["deviceClass"].as<std::string>().c_str() : "",
                stateObj["measurement"].as<bool>(),
                stateObj["diagnostic"].as<bool>()
            );
            fromJSON(state, stateObj);
            addState(state);
        } else if (stateObj["valueType"] == "int") {
            auto *state = new OBDStateInt(
                stateObj["type"].as<OBDStateType>(),
                stateObj["name"].as<std::string>().c_str(),
                stateObj["description"].as<std::string>().c_str(),
                !stateObj["icon"].isNull() ? stateObj["icon"].as<std::string>().c_str() : "",
                !stateObj["unit"].isNull() ? stateObj["unit"].as<std::string>().c_str() : "",
                !stateObj["deviceClass"].isNull() ? stateObj["deviceClass"].as<std::string>().c_str() : "",
                stateObj["measurement"].as<bool>(),
                stateObj["diagnostic"].as<bool>()
            );
            fromJSON(state, stateObj);
            addState(state);
        }
    }
}

void OBDClass::writeJSON(JsonDocument &doc) {
    std::vector<OBDState *> states{};
    getStates([](const OBDState *) {
        return true;
    }, states);
    for (OBDState *state: states) {
        JsonDocument stateObj;
        state->toJSON(stateObj);
        doc.add(stateObj);
    }
}

bool OBDClass::writeStates(FS &fs) {
    bool success = false;

    File file = fs.open(STATES_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file settings.json for writing.");
        return false;
    }

    JsonDocument doc;
    writeJSON(doc);
    success = serializeJson(doc, file);

    file.close();

    return success;
}

template<typename T>
T *OBDClass::setReadFuncByName(const char *funcName, T *state) {
    if (strcmp(funcName, "batteryVoltage") == 0 && strcmp(state->valueType(), "float") == 0) {
        state
                ->withReadFuncName("batteryVoltage")
                ->withReadFunc([&]() {
                    return elm327.batteryVoltage();
                });
    }

    return state;
}

template<typename T>
T *OBDClass::setFormatFuncByName(const char *funcName, T *state) {
    if (strcmp(state->valueType(), "int") == 0) {
        auto *is = reinterpret_cast<OBDStateInt *>(state);
        if (strcmp(funcName, "toBitStr") == 0) {
            is
                    ->withValueFormatFuncName("toBitStr")
                    ->withValueFormatFunc([](const int value) {
                        char str[33];
                        snprintf(str, sizeof(str), "%s", std::bitset<32>(value).to_string().c_str());
                        return strdup(str);
                    });
        } else if (strcmp(funcName, "toMiles") == 0) {
            is
                    ->withValueFormatFuncName("toMiles")
                    ->withValueFormatFunc([&](const int value) {
                        char str[16];
                        snprintf(str, sizeof(str), "%d", static_cast<int>(static_cast<float>(value) / KPH_TO_MPH));
                        return strdup(str);
                    });
        }
    } else if (strcmp(state->valueType(), "float") == 0) {
        auto *is = reinterpret_cast<OBDStateFloat *>(state);
        if (strcmp(funcName, "toMiles") == 0) {
            is
                    ->withValueFormatFuncName("toMiles")
                    ->withValueFormatFunc([&](const float value) {
                        char str[16];
                        snprintf(str, sizeof(str), "%4.2f", value / KPH_TO_MPH);
                        return strdup(str);
                    });
        } else if (strcmp(funcName, "toGallons") == 0) {
            is
                    ->withValueFormatFuncName("toGallons")
                    ->withValueFormatFunc([&](const float value) {
                        char str[16];
                        snprintf(str, sizeof(str), "%4.2f", value / LITER_TO_GALLON);
                        return strdup(str);
                    });
        } else if (strcmp(funcName, "toMPG") == 0) {
            is
                    ->withValueFormatFuncName("toMPG")
                    ->withValueFormatFunc([&](const float value) {
                        char str[16];
                        snprintf(str, sizeof(str), "%4.2f", value == 0.0f ? 0.0f : 235.214583333333f / value);
                        return strdup(str);
                    });
        }
    }
    return state;
}

void OBDClass::BTEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    if (event == ESP_SPP_CLOSE_EVT) {
        Serial.println("Bluetooth disconnected.");

        if (OBD.initDone && !OBD.stopConnect) {
            // FIXME get reconnect working - failed with "getChannels() failed timeout"
            // OBD.connect(true);
            ESP.restart();
        }
    }
}

BTScanResults *OBDClass::discoverBtDevices() {
    serialBt.discoverClear();

    Serial.println("Discover Bluetooth devices...");

    BTScanResults *btDeviceList = serialBt.getScanResults(); // maybe accessing from different threads!
    if (serialBt.discoverAsync([](BTAdvertisedDevice *pDevice) {
        Serial.printf(">>>>>>>>>>>Found a new device: %s\n", pDevice->toString().c_str());
    })) {
        delay(BT_DISCOVER_TIME);
        Serial.print("Stopping discover...");
        serialBt.discoverAsyncStop();
        Serial.println("stopped");
        delay(5000);

        if (btDeviceList->getCount() > 0) {
            return btDeviceList;
        }
    }

    return nullptr;
}

void OBDClass::begin(const String &devName, const String &devMac, FS &fs, const char protocol,
                     const bool checkPidSupport) {
    this->devName = devName;
    this->devMac = devMac;
    this->fs = &fs;
    this->protocol = protocol;
    this->checkPidSupport = checkPidSupport;
    stopConnect = false;
    serialBt.register_callback(BTEvent);
}

void OBDClass::end() {
    stopConnect = true;
    serialBt.disconnect();
    serialBt.end();
}

void OBDClass::connect(bool reconnect) {
    stopConnect = false;

connect:
    if (stopConnect || reconnect && !initDone) {
        return;
    }

    if (!serialBt.begin("OBD2MQTT", true)) {
        Serial.println("========== serialBT failed!");
        ESP.restart();
    }

    if (devMac.isEmpty()) {
        BTScanResults *btDeviceList = discoverBtDevices();

        if (btDeviceList == nullptr) {
            Serial.println("Didn't find any devices");
        } else {
            BTAddress addr;
            int channel = 0;

            if (devDiscoveredCallback != nullptr && btDeviceList->getCount() != 0) {
                devDiscoveredCallback(btDeviceList);
            }

            Serial.printf("Search device: %s\n", devName.c_str());
            for (int i = 0; i < btDeviceList->getCount(); i++) {
                BTAdvertisedDevice *device = btDeviceList->getDevice(i);
                if (device->getName() == devName.c_str()) {
                    Serial.printf(" ----- %s  %s %d\n", device->getAddress().toString().c_str(),
                                  device->getName().c_str(), device->getRSSI());
                    std::map<int, std::string> channels = serialBt.getChannels(device->getAddress());
                    Serial.printf("scanned for services, found %d\n", channels.size());
                    for (auto const &entry: channels) {
                        Serial.printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
                    }
                    if (!channels.empty()) {
                        addr = device->getAddress();
                        channel = channels.begin()->first;
                    }
                }
            }

            if (!stopConnect && addr) {
                Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
                if (serialBt.connect(addr, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE)) {
                    connectedBTAddress = addr.toString().c_str();
                }
            }
        }
    } else {
        byte mac[6];
        parseBytes(devMac.c_str(), ':', mac, 6, 16);
        BTAddress addr = mac;
        int channel = 0;

        std::map<int, std::string> channels = serialBt.getChannels(addr);
        Serial.printf("scanned for services, found %d\n", channels.size());
        for (auto const &entry: channels) {
            Serial.printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
        }

        if (!channels.empty()) {
            channel = channels.begin()->first;
        }

        if (!stopConnect && addr) {
            Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
            if (serialBt.connect(addr, channel, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE)) {
                connectedBTAddress = addr.toString().c_str();
            }
        }
    }

    if (!stopConnect && !serialBt.isClosed() && serialBt.connected()) {
        int retryCount = 0;
        while (!elm327.begin(serialBt, false, 2000, protocol) && retryCount < 3) {
            Serial.println("Couldn't connect to OBD scanner - Phase 2");
            delay(BT_DISCOVER_TIME);
            retryCount++;
        }
    } else if (!stopConnect) {
        Serial.println("Couldn't connect to OBD scanner - Phase 1");
    }

    // if connection stopped (AP connected) wait before reconnect
    while (stopConnect) {
        delay(BT_DISCOVER_TIME);
    }

    if (!elm327.connected) {
        delay(BT_DISCOVER_TIME);
        Serial.println("Restarting OBD connect.");
        serialBt.end();
        goto connect;
    }

    Serial.println("Connected to ELM327");

    if (!reconnect) {
        if (fs != nullptr) {
            readStates(*fs);
            setCheckPidSupport(this->checkPidSupport);
        }

        Serial.println("Try to get VIN...");
        char vin[18];
        int8_t status;
        int retryCount = 0;
        while ((status = elm327.get_vin_blocking(vin)) != ELM_SUCCESS && retryCount < 3) {
            Serial.println("...failed to obtain VIN...");
            delay(500);
            retryCount++;
        }

        if (status == ELM_SUCCESS) {
            VIN = std::string(vin);
            if (!VIN.empty()) {
                Serial.printf("...VIN %s obtained.\n", VIN.c_str());
            } else {
                Serial.println("...VIN is empty.");
            }
        }

        initDone = true;
    }
}

void OBDClass::loop() {
    if (!stopConnect && serialBt && !serialBt.isClosed()) {
        nextState();
    } else {
        delay(500);
    }
}

void OBDClass::onDevicesDiscovered(const std::function<void(BTScanResults *scanResult)> &callable) {
    devDiscoveredCallback = callable;
}

std::string OBDClass::getConnectedBTAddress() const {
    return connectedBTAddress;
}

std::string OBDClass::vin() const {
    return VIN;
}

OBDClass OBD;
