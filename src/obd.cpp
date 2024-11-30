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

void OBDClass::initStates() {
    // onetime states
    addState((new OBDStateInt(CALC, "startTime", "Start Time", "", "", "", false, true))
        ->withVisible(false)
        ->withUpdateInterval(-1)
        ->withCalcExpression("$millis"));
    addState((new OBDStateInt(READ, "supportedPids_1_20", "Supported PIDs 1-20", "", "", "", false, true))
        ->withUpdateInterval(-1)
        ->withPIDSettings(SERVICE_01, SUPPORTED_PIDS_1_20, 1, 4)
        ->withValueFormatFunc(toBitStr));
    addState((new OBDStateInt(READ, "supportedPids_21_40", "Supported PIDs 21-40", "", "", "", false, true))
        ->withUpdateInterval(-1)
        ->withPIDSettings(SERVICE_01, SUPPORTED_PIDS_21_40, 1, 4)
        ->withValueFormatFunc(toBitStr));
    addState((new OBDStateInt(READ, "supportedPids_41_60", "Supported PIDs 41-60", "", "", "", false, true))
        ->withUpdateInterval(-1)
        ->withPIDSettings(SERVICE_01, SUPPORTED_PIDS_41_60, 1, 4)
        ->withValueFormatFunc(toBitStr));
    addState((new OBDStateInt(READ, "supportedPids_61_80", "Supported PIDs 61-80", "", "", "", false, true))
        ->withUpdateInterval(-1)
        ->withPIDSettings(SERVICE_01, SUPPORTED_PIDS_61_80, 1, 4)
        ->withValueFormatFunc(toBitStr));

    addState((new OBDStateInt(READ, "engineLoad", "Engine Load", "engine", "%", ""))
        ->withPIDSettings(SERVICE_01, ENGINE_LOAD, 1, 1, 100.0 / 255.0));
    addState((new OBDStateInt(READ, "throttle", "Throttle", "gauge", "%", ""))
        ->withPIDSettings(SERVICE_01, THROTTLE_POSITION, 1, 1, 100.0 / 255.0));
    addState((new OBDStateInt(READ, "rpm", "Rounds per minute", "engine", ""))
        ->withPIDSettings(SERVICE_01, ENGINE_RPM, 1, 2, 1.0 / 4.0));
    addState((new OBDStateInt(READ, "speed",
                              system == METRIC ? "Kilometer per Hour" : "Miles per Hour", "speedometer",
                              system == METRIC ? "km/h" : "mph", "speed"))
        ->withPIDSettings(SERVICE_01, VEHICLE_SPEED, 1, 1)
        ->withValueFormatFunc(toMilesInt));
    addState(
        (new OBDStateInt(READ, "engineCoolantTemp", "Engine Coolant Temperature", "thermometer", "°C", "temperature"))
        ->withPIDSettings(SERVICE_01, ENGINE_COOLANT_TEMP, 1, 1, 1, -40.0));
    addState(
        (new OBDStateInt(READ, "oilTemp", "Oil Temperature", "thermometer", "°C", "temperature"))
        ->withPIDSettings(SERVICE_01, ENGINE_OIL_TEMP, 1, 1, 1, -40.0));
    addState(
        (new OBDStateInt(READ, "ambientAirTemp", "Ambient Temperature", "thermometer", "°C", "temperature"))
        ->withPIDSettings(SERVICE_01, AMBIENT_AIR_TEMP, 1, 1, 1, -40)
    );
    addState(
        (new OBDStateFloat(READ, "mafRate", "Mass Air Flow", "air-filter", "g/s"))
        ->withPIDSettings(SERVICE_01, MAF_FLOW_RATE, 1, 2, 1.0 / 100.0));
    addState(
        (new OBDStateInt(READ, "fuelLevel", "Fuel Level", "fuel", "%", ""))
        ->withPIDSettings(SERVICE_01, FUEL_TANK_LEVEL_INPUT, 1, 1, 100.0 / 255.0)
        ->withUpdateInterval(30000));
    addState(
        (new OBDStateFloat(READ, "fuelRate", "fuelRate", "Fuel Rate", "fuel", system == METRIC ? "L/h" : "gal/h"))
        ->withPIDSettings(SERVICE_01, ENGINE_FUEL_RATE, 1, 2, 1.0 / 20.0)
        ->withValueFormatFunc(toGallons));
    addState(
        (new OBDStateInt(READ, "fuelType", "Fuel Type", "water-opacity", "", "", false, true))
        ->withPIDSettings(SERVICE_01, FUEL_TYPE, 1, 1)
        ->withUpdateInterval(30000));
    addState((new OBDStateFloat(READ, "batteryVoltage", "Battery Voltage", "battery", "V", "voltage"))
        ->withReadFunc([&]() {
            return elm327.batteryVoltage();
        })
        ->withUpdateInterval(30000));
    addState(
        (new OBDStateInt(READ, "intakeAirTemp", "Intake Air Temperature", "thermometer", "°C", "temperature"))
        ->withPIDSettings(SERVICE_01, INTAKE_AIR_TEMP, 1, 1, 1, -40.0));
    addState(
        (new OBDStateInt(READ, "manifoldPressure", "Manifold Pressure", "", "kPa", "pressure"))
        ->withPIDSettings(SERVICE_01, INTAKE_MANIFOLD_ABS_PRESSURE, 1, 1)
        ->withEnabled(false));
    addState(
        (new OBDStateFloat(READ, "timingAdvance", "Timing Advance", "axis-x-rotate-clockwise", "°"))
        ->withPIDSettings(SERVICE_01, TIMING_ADVANCE, 1, 1, 1.0 / 2.0, -64.0)
        ->withEnabled(false));
    addState((new OBDStateInt(READ, "relativePedalPos", "Pedal Position", "seat-recline-extra", "%"))
        ->withPIDSettings(SERVICE_01, RELATIVE_ACCELERATOR_PEDAL_POS, 1, 1, 100.0 / 255.0));
    addState(
        (new OBDStateInt(READ, "monitorStatus", "Monitor Status", "", "", "", false, true))
        ->withPIDSettings(SERVICE_01, MONITOR_STATUS_SINCE_DTC_CLEARED, 1, 4)
        ->withUpdateInterval(60000)
        ->withValueFormatFunc(toBitStr));
    addState((new OBDStateInt(READ, "odometer", "Odometer", "counter", system == METRIC ? "km" : "mi", ""))
        ->withPIDSettings(SERVICE_01, 0xA6, 1, 4, 1.0 / 10.0)
        ->withValueFormatFunc(toMilesInt));

    // calculated states
    addState((new OBDStateBool(CALC, "engineRunning", "Engine Running", "engine", "", "", false))
        ->withCalcExpression("max($rpm, 300) - 300"));
    addState((new OBDStateFloat(CALC, "distanceDriven", "Calculated driven distance", "map-marker-distance",
                                system == METRIC ? "km" : "mi", "distance"))
        ->withCalcExpression(
            "$distanceDriven + ($speed.ov + $speed) / 2 / 3600 * ($millis - $distanceDriven.lu) / 1000")
        ->withValueFormatFunc(toMiles));
    addState((new OBDStateFloat(CALC, "consumption", "Calculated consumption", "gas-station",
                                system == METRIC ? "L" : "gal", "volume"))
        ->withCalcExpression(
            "$consumption + ($mafRate * 3600 / (afRatio($fuelType) * density($fuelType))) / 3600 * ($millis - $consumption.lu) / 1000")
        ->withValueFormatFunc(toGallons));
    addState((new OBDStateFloat(CALC, "consumptionReadable",
                                system == METRIC ? "Calculated consumption per 100km" : "Calculated Miles per gallon",
                                "gas-station", system == METRIC ? "l/100km" : "mpg"))
        ->withCalcExpression("($consumption / $distanceDriven) * 100")
        ->withValueFormatFunc(toMPG));
    addState((new OBDStateInt(CALC, "topSpeed", "Top Speed", "speedometer", system == METRIC ? "km/h" : "mph", "speed"))
        ->withCalcExpression("max($topSpeed, $speed)")
        ->withValueFormatFunc(toMilesInt));
    addState(
        (new OBDStateFloat(CALC, "avgSpeed", "Calculated average speed", "speedometer-medium",
                           system == METRIC ? "km/h" : "mph", "speed"))
        ->withCalcExpression("$distanceDriven / (($millis - $startTime) / 1000) * 3600")
        ->withValueFormatFunc(toMiles));
    addState((new OBDStateBool(CALC, "milState", "Check Engine Light", "engine-off", "", "", false))
        ->withUpdateInterval(60000)
        ->withCalcExpression("$monitorStatus.c & 128"));
    addState((new OBDStateInt(CALC, "numDTCs", "Number of DTCs", "code-array", "", "", false, true))
        ->withUpdateInterval(60000)
        ->withCalcExpression("numDTCs($monitorStatus.c - 128)"));
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

void OBDClass::begin(const String &devName, const String &devMac, const char protocol, const bool checkPidSupport,
                     measurementSystem system) {
    this->devName = devName;
    this->devMac = devMac;
    this->protocol = protocol;
    this->checkPidSupport = checkPidSupport;
    this->system = system;
    stopConnect = false;
    serialBt.register_callback(BTEvent);
    setCheckPidSupport(this->checkPidSupport);
}

void OBDClass::end() {
    stopConnect = true;
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
        initStates();

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

        if (runStartTime == 0) {
            runStartTime = millis();
        }

        initDone = true;
    }
}

void OBDClass::loop() {
    if (!serialBt.isClosed()) {
        nextState();
    } else {
        delay(500);
    }
}

void OBDClass::onDevicesDiscovered(const std::function<void(BTScanResults *scanResult)> &callable) {
    devDiscoveredCallback = callable;
}

unsigned long OBDClass::getRunStartTime() const {
    return runStartTime;
}

std::string OBDClass::getConnectedBTAddress() const {
    return connectedBTAddress;
}

std::string OBDClass::vin() const {
    return VIN;
}

OBDClass OBD;
