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
#include "helper.h"

OBDClass::OBDClass(): elm327() {
    protocol = AUTOMATIC;
}

void OBDClass::BTEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    if (event == ESP_SPP_CLOSE_EVT) {
        Serial.println("Bluetooth disconnected.");
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

template<typename T>
bool OBDClass::setStateValue(T &var, obd_pid_states nextState, T value) {
    if (elm327.nb_rx_state == ELM_SUCCESS) {
        var = value;
        obd_state = nextState;
        return true;
    }

    if (elm327.nb_rx_state != ELM_GETTING_MSG && elm327.nb_rx_state != ELM_SUCCESS) {
        elm327.printError();
    }

    if (elm327.nb_rx_state == ELM_NO_DATA) {
        var = 0;
        obd_state = nextState;
    }

    return false;
}

void OBDClass::begin(const String &devName, const String &devMac, const char protocol, bool checkPidSupport) {
    this->devName = devName;
    this->devMac = devMac;
    this->protocol = protocol;
    this->checkPidSupport = checkPidSupport;
    stopConnect = false;
}

void OBDClass::end() {
    stopConnect = true;
    serialBt.end();
}

void OBDClass::connect(bool reconnect) {
    stopConnect = false;
connect:
    serialBt.register_callback(BTEvent);

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
        Serial.println("Cache supported PIDs...");
        isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED);
        isPidSupported(DISTANCE_TRAVELED_WITH_MIL_ON);
        isPidSupported(MONITOR_STATUS_THIS_DRIVE_CYCLE);
        isPidSupported(DEMANDED_ENGINE_PERCENT_TORQUE);
        Serial.println("...done.");

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
    if (!serialBt.isClosed()) {
        switch (obd_state) {
            case ENG_LOAD: {
                if (isPidSupported(ENGINE_LOAD)) {
                    setStateValue(load, THROTTLE, static_cast<int>(elm327.engineLoad()));
                } else {
                    obd_state = THROTTLE;
                }
                break;
            }
            case THROTTLE:
                if (isPidSupported(THROTTLE_POSITION)) {
                    setStateValue(throttle, RPM, static_cast<int>(elm327.throttle()));
                } else {
                    obd_state = RPM;
                }
                break;
            case RPM: {
                if (isPidSupported(ENGINE_RPM)) {
                    setStateValue(rpm, COOLANT_TEMP, elm327.rpm());
                } else {
                    obd_state = COOLANT_TEMP;
                }
                break;
            }
            case COOLANT_TEMP: {
                if (isPidSupported(ENGINE_COOLANT_TEMP)) {
                    setStateValue(coolantTemp, OIL_TEMP, elm327.engineCoolantTemp());
                } else {
                    obd_state = OIL_TEMP;
                }
                break;
            }
            case OIL_TEMP: {
                if (isPidSupported(ENGINE_OIL_TEMP)) {
                    setStateValue(oilTemp, AMBIENT_TEMP, elm327.oilTemp());
                } else {
                    obd_state = AMBIENT_TEMP;
                }
                break;
            }
            case AMBIENT_TEMP: {
                if (isPidSupported(AMBIENT_AIR_TEMP)) {
                    setStateValue(ambientAirTemp, SPEED, elm327.ambientAirTemp());
                } else {
                    obd_state = SPEED;
                }
                break;
            }
            case SPEED: {
                if (isPidSupported(VEHICLE_SPEED)) {
                    int kphBefore = kph;
                    setStateValue(kph, MAF_RATE, elm327.kph());

                    if (runStartTime == 0 & kph > 0) {
                        runStartTime = millis();
                    }

                    if (kph > topSpeed) {
                        topSpeed = kph;
                    }

                    distanceDriven = distanceDriven +
                                     calcDistance(
                                         (kphBefore + kph) / 2,
                                         static_cast<float>(millis() - lastReadSpeed) / 1000.0f
                                     );
                    consumption = consumption + calcConsumption(fuelType, kph, mafRate) / 3600.0f *
                                  static_cast<float>(millis() - lastReadSpeed) / 1000.0f;
                    lastReadSpeed = millis();
                } else {
                    obd_state = MAF_RATE;
                }
                break;
            }
            case MAF_RATE: {
                if (isPidSupported(MAF_FLOW_RATE)) {
                    setStateValue(mafRate, FUEL_LEVEL, elm327.mafRate());
                } else {
                    obd_state = FUEL_LEVEL;
                }
                break;
            }
            case FUEL_LEVEL: {
                if (isPidSupported(FUEL_TANK_LEVEL_INPUT)) {
                    setStateValue(fuelLevel, FUEL_RATE, elm327.fuelLevel());
                } else {
                    obd_state = FUEL_RATE;
                }
                break;
            }
            case FUEL_RATE: {
                if (isPidSupported(ENGINE_FUEL_RATE)) {
                    setStateValue(fuelRate, FUEL_T, elm327.fuelRate());
                } else {
                    obd_state = FUEL_T;
                }
                break;
            }
            case FUEL_T: {
                if (isPidSupported(FUEL_TYPE) && !fuelTypeRead) {
                    setStateValue(fuelType, BAT_VOLTAGE, elm327.fuelType());
                    fuelTypeRead = true;
                } else {
                    obd_state = BAT_VOLTAGE;
                }
                break;
            }
            case BAT_VOLTAGE: {
                setStateValue(batVoltage, INT_AIR_TEMP, elm327.batteryVoltage());
                break;
            }
            case INT_AIR_TEMP: {
                if (isPidSupported(INTAKE_AIR_TEMP)) {
                    setStateValue(intakeAirTemp, MANIFOLD_PRESSURE, elm327.intakeAirTemp());
                } else {
                    obd_state = MANIFOLD_PRESSURE;
                }
                break;
            }
            case MANIFOLD_PRESSURE: {
                if (isPidSupported(INTAKE_MANIFOLD_ABS_PRESSURE)) {
                    setStateValue(manifoldPressure, IGN_TIMING, elm327.manifoldPressure());
                } else {
                    obd_state = IGN_TIMING;
                }
                break;
            }
            case IGN_TIMING: {
                if (isPidSupported(TIMING_ADVANCE)) {
                    setStateValue(timingAdvance, PEDAL_POS, elm327.timingAdvance());
                } else {
                    obd_state = PEDAL_POS;
                }
                break;
            }
            case PEDAL_POS: {
                if (isPidSupported(RELATIVE_ACCELERATOR_PEDAL_POS)) {
                    setStateValue(pedalPosition, MILSTATUS, elm327.relativePedalPos());
                } else {
                    obd_state = MILSTATUS;
                }
                break;
            }
            case MILSTATUS: {
                if (isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED)) {
                    setStateValue(monitorStatus, ENG_LOAD, elm327.monitorStatus());
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

void OBDClass::onDevicesDiscovered(const std::function<void(BTScanResults *scanResult)> &callable) {
    devDiscoveredCallback = callable;
}

bool OBDClass::isPidSupported(uint8_t pid) {
    bool cached = false;
    uint32_t response = 0;
    uint8_t pidInterval = (pid / PID_INTERVAL_OFFSET) * PID_INTERVAL_OFFSET;

    int retries = 0;
    do {
        switch (pidInterval) {
            case SUPPORTED_PIDS_1_20:
                response = !supportedPids_1_20_cached
                               ? elm327.supportedPIDs_1_20()
                               : supportedPids_1_20;
                cached = supportedPids_1_20_cached;
                break;
            case SUPPORTED_PIDS_21_40:
                response = !supportedPids_21_40_cached
                               ? elm327.supportedPIDs_21_40()
                               : supportedPids_21_40;
                pid = (pid - SUPPORTED_PIDS_21_40);
                cached = supportedPids_21_40_cached;
                break;
            case SUPPORTED_PIDS_41_60:
                response = !supportedPids_41_60_cached
                               ? elm327.supportedPIDs_41_60()
                               : supportedPids_41_60;
                pid = (pid - SUPPORTED_PIDS_41_60);
                cached = supportedPids_41_60_cached;
                break;
            case SUPPORTED_PIDS_61_80:
                response = !supportedPids_61_80_cached
                               ? elm327.supportedPIDs_61_80()
                               : supportedPids_61_80;
                pid = (pid - SUPPORTED_PIDS_61_80);
                cached = supportedPids_61_80_cached;
                break;
            default:
                break;
        }
        if (!cached) {
            if (elm327.nb_rx_state == ELM_GETTING_MSG) {
                Serial.print(".");
                delay(500);
                retries++;
            } else if (elm327.nb_rx_state != ELM_SUCCESS) {
                Serial.print("x");
                delay(500);
                retries++;
            }
        }
    } while (!cached && response == 0 && elm327.nb_rx_state != ELM_SUCCESS && retries < 10);
    if (!cached) {
        Serial.println("");
    }

    if (!cached && response != 0 && elm327.nb_rx_state == ELM_SUCCESS) {
        switch (pidInterval) {
            case SUPPORTED_PIDS_1_20:
                supportedPids_1_20 = response;
                supportedPids_1_20_cached = true;
                break;
            case SUPPORTED_PIDS_21_40:
                supportedPids_21_40 = response;
                supportedPids_21_40_cached = true;
                break;
            case SUPPORTED_PIDS_41_60:
                supportedPids_41_60 = response;
                supportedPids_41_60_cached = true;
                break;
            case SUPPORTED_PIDS_61_80:
                supportedPids_61_80 = response;
                supportedPids_61_80_cached = true;
                break;
            default:
                break;
        }
    } else if (!cached && !checkPidSupport) {
        switch (pidInterval) {
            case SUPPORTED_PIDS_1_20:
                supportedPids_1_20 = 0xFFFFFFFF;
                supportedPids_1_20_cached = true;
                break;
            case SUPPORTED_PIDS_21_40:
                supportedPids_21_40 = 0xFFFFFFFF;
                supportedPids_21_40_cached = true;
                break;
            case SUPPORTED_PIDS_41_60:
                supportedPids_41_60 = 0xFFFFFFFF;
                supportedPids_41_60_cached = true;
                break;
            case SUPPORTED_PIDS_61_80:
                supportedPids_61_80 = 0xFFFFFFFF;
                supportedPids_61_80_cached = true;
                break;
            default:
                break;
        }
        response = 0xFFFFFFFF;
    }

    return ((response >> (32 - pid)) & 0x1);
}

uint32_t OBDClass::getSupportedPids1To20() const {
    return supportedPids_1_20;
}

uint32_t OBDClass::getSupportedPids21To40() const {
    return supportedPids_21_40;
}

uint32_t OBDClass::getSupportedPids41To60() const {
    return supportedPids_41_60;
}

uint32_t OBDClass::getSupportedPids61To80() const {
    return supportedPids_61_80;
}

int OBDClass::getLoad() const {
    return load;
}

int OBDClass::getThrottle() const {
    return throttle;
}

float OBDClass::getRPM() const {
    return rpm;
}

float OBDClass::getCoolantTemp() const {
    return coolantTemp;
}

float OBDClass::getOilTemp() const {
    return oilTemp;
}

float OBDClass::getAmbientAirTemp() const {
    return ambientAirTemp;
}

int OBDClass::getSpeed(measurementSystem system) const {
    return system == METRIC ? kph : kph / KPH_TO_MPH;
}

float OBDClass::getFuelLevel() const {
    return fuelLevel;
}

float OBDClass::getFuelRate(measurementSystem system) const {
    return system == METRIC ? fuelRate : fuelRate / LITER_TO_GALLON;
}

uint8_t OBDClass::getFuelType() const {
    return fuelType;
}

bool OBDClass::getFuelTypeRead() const {
    return fuelTypeRead;
}

float OBDClass::getMafRate() const {
    return mafRate;
}

float OBDClass::getBatVoltage() const {
    return batVoltage;
}

float OBDClass::getIntakeAirTemp() const {
    return intakeAirTemp;
}

uint8_t OBDClass::getManifoldPressure() const {
    return manifoldPressure;
}

float OBDClass::getTimingAdvance() const {
    return timingAdvance;
}

float OBDClass::getPedalPosition() const {
    return pedalPosition;
}

uint32_t OBDClass::getMonitorStatus() const {
    return monitorStatus;
}

bool OBDClass::getMilState() const {
    return milState;
}

unsigned long OBDClass::getLastReadSpeed() const {
    return lastReadSpeed;
}

unsigned long OBDClass::getRunStartTime() const {
    return runStartTime;
}

float OBDClass::getCurConsumption(measurementSystem system) const {
    return system == METRIC ? curConsumption : curConsumption / LITER_TO_GALLON;
}

float OBDClass::getConsumption(measurementSystem system) const {
    return system == METRIC ? consumption : consumption / LITER_TO_GALLON;
}

float OBDClass::getConsumptionForMeasurement(measurementSystem system) const {
    return system == METRIC ? consumptionPer100 : 235.214583333333f / consumptionPer100;
}

float OBDClass::getDistanceDriven(measurementSystem system) const {
    return system == METRIC ? distanceDriven : distanceDriven / KPH_TO_MPH;
}

float OBDClass::getAvgSpeed(measurementSystem system) const {
    return system == METRIC ? avgSpeed : avgSpeed / KPH_TO_MPH;
}

int OBDClass::getTopSpeed(measurementSystem system) const {
    return system == METRIC ? topSpeed : topSpeed / KPH_TO_MPH;
}

std::string OBDClass::getConnectedBTAddress() const {
    return connectedBTAddress;
}

std::string OBDClass::vin() const {
    return VIN;
}

float OBDClass::calcCurrentConsumption(const int fuelType, const int kph, const float mafRate) {
    if (kph != 0 && mafRate != 0) {
        float afRatio = 0;
        switch (fuelType) {
            case FUEL_TYPE_METHANOL:
                afRatio = AF_RATIO_METHANOL;
                break;
            case FUEL_TYPE_ETHANOL:
                afRatio = AF_RATIO_ETHANOL;
                break;
            case FUEL_TYPE_DIESEL:
                afRatio = AF_RATIO_DIESEL;
                break;
            case FUEL_TYPE_LPG:
            case FUEL_TYPE_CNG:
                afRatio = AF_RATIO_GAS;
                break;
            case FUEL_TYPE_PROPANE:
                afRatio = AF_RATIO_PROPANE;
                break;
            case FUEL_TYPE_ELECTRIC:
                return 0;
            default:
                afRatio = AF_RATIO_GASOLINE;
                break;
        }
        return static_cast<float>(kph) / (mafRate / afRatio);
    }

    return 0.0;
}

float OBDClass::calcConsumption(const int fuelType, const int kph, const float mafRate) {
    if (kph != 0 && mafRate != 0) {
        float afRatio = 0;
        float density = 0;
        switch (fuelType) {
            case FUEL_TYPE_METHANOL:
                afRatio = AF_RATIO_METHANOL;
                density = DENSITY_METHANOL;
                break;
            case FUEL_TYPE_ETHANOL:
                afRatio = AF_RATIO_ETHANOL;
                density = DENSITY_ETHANOL;
                break;
            case FUEL_TYPE_DIESEL:
                afRatio = AF_RATIO_DIESEL;
                density = DENSITY_DIESEL;
                break;
            case FUEL_TYPE_LPG:
            case FUEL_TYPE_CNG:
                afRatio = AF_RATIO_GAS;
                density = DENSITY_GAS;
                break;
            case FUEL_TYPE_PROPANE:
                afRatio = AF_RATIO_PROPANE;
                density = DENSITY_PROPANE;
                break;
            case FUEL_TYPE_ELECTRIC:
                return 0;
            default:
                afRatio = AF_RATIO_GASOLINE;
                density = DENSITY_GASOLINE;
                break;
        }
        return mafRate * 3600.0f / (afRatio * density);
    }

    return 0.0;
}

float OBDClass::calcDistance(const int kph, const float time) {
    return kph > 0 ? static_cast<float>(kph) / 3600.0f * time : 0.0f;
}

OBDClass OBD;
