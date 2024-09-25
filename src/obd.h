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
#pragma once
#include "helper.h"
#include <atomic>
#include <BluetoothSerial.h>

#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT

esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
// or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE to request pincode confirmation
esp_spp_role_t role = ESP_SPP_ROLE_SLAVE; // or ESP_SPP_ROLE_SLAVE | ESP_SPP_ROLE_MASTER

// set default adapter name
#ifndef OBD_ADP_NAME
#define OBD_ADP_NAME        "OBDII"
#endif

// set default OBD protocol
#ifndef OBD_PROTOCOL
#define OBD_PROTOCOL        AUTOMATIC
#endif

#define BT_DISCOVER_TIME    10000

ELM327 myELM327;

// https://stackoverflow.com/questions/17170646/what-is-the-best-way-to-get-fuel-consumption-mpg-using-obd2-parameters
#define AF_RATIO_GAS        17.2
#define AF_RATIO_GASOLINE   14.7
#define AF_RATIO_PROPANE    15.5
#define AF_RATIO_ETHANOL    9
#define AF_RATIO_METHANOL   6.4
#define AF_RATIO_HYDROGEN   34
#define AF_RATIO_DIESEL     14.6

// https://kraftstoff-info.de/vergleich-der-kraftstoffe-nach-energiegehalt-und-dichte
#define DENSITY_GAS         540.0
#define DENSITY_GASOLINE    740.0
#define DENSITY_PROPANE     505.0
#define DENSITY_ETHANOL     789.0
#define DENSITY_METHANOL    792.0
#define DENSITY_HYDROGEN    70.0
#define DENSITY_DIESEL      830.0

// https://en.m.wikipedia.org/w/index.php?title=OBD-II_PIDs
#define FUEL_TYPE_GASOLINE  1
#define FUEL_TYPE_METHANOL  2
#define FUEL_TYPE_ETHANOL   3
#define FUEL_TYPE_DIESEL    4
#define FUEL_TYPE_LPG       5
#define FUEL_TYPE_CNG       6
#define FUEL_TYPE_PROPANE   7
#define FUEL_TYPE_ELECTRIC  8

typedef enum {
    ENG_LOAD,
    RPM,
    COOLANT_TEMP,
    INT_AIR_TEMP,
    AMBIENT_TEMP,
    OIL_TEMP,
    MANIFOLD_PRESSURE,
    IGN_TIMING,
    SPEED,
    THROTTLE,
    MAF_RATE,
    FUEL_LEVEL,
    FUEL_RATE,
    FUEL_T,
    BAT_VOLTAGE
} obd_pid_states;

obd_pid_states obd_state = ENG_LOAD;

// cached PIDs
std::atomic_uint32_t supportedPids_1_20{0};
std::atomic_bool supportedPids_1_20_cached{false};
std::atomic_uint32_t supportedPids_21_40{0};
std::atomic_bool supportedPids_21_40_cached{false};
std::atomic_uint32_t supportedPids_41_60{0};
std::atomic_bool supportedPids_41_60_cached{false};
std::atomic_uint32_t supportedPids_61_80{0};
std::atomic_bool supportedPids_61_80_cached{false};

std::string connectedBTAddress;
std::string VIN;

/**
* Checks if PID is supported.
*
* @param pid the PID
*/
inline bool isPidSupported(uint8_t pid) {
    bool cached = false;
    uint32_t response = 0;
    uint8_t pidInterval = (pid / PID_INTERVAL_OFFSET) * PID_INTERVAL_OFFSET;

    int retries = 0;
    do {
        switch (pidInterval) {
            case SUPPORTED_PIDS_1_20:
                response = !supportedPids_1_20_cached
                               ? myELM327.supportedPIDs_1_20()
                               : static_cast<uint32_t>(supportedPids_1_20);
                cached = supportedPids_1_20_cached;
                break;
            case SUPPORTED_PIDS_21_40:
                response = !supportedPids_21_40_cached
                               ? myELM327.supportedPIDs_21_40()
                               : static_cast<uint32_t>(supportedPids_21_40);
                pid = (pid - SUPPORTED_PIDS_21_40);
                cached = supportedPids_21_40_cached;
                break;
            case SUPPORTED_PIDS_41_60:
                response = !supportedPids_41_60_cached
                               ? myELM327.supportedPIDs_41_60()
                               : static_cast<uint32_t>(supportedPids_41_60);
                pid = (pid - SUPPORTED_PIDS_41_60);
                cached = supportedPids_41_60_cached;
                break;
            case SUPPORTED_PIDS_61_80:
                response = !supportedPids_61_80_cached
                               ? myELM327.supportedPIDs_61_80()
                               : static_cast<uint32_t>(supportedPids_61_80);
                pid = (pid - SUPPORTED_PIDS_61_80);
                cached = supportedPids_61_80_cached;
                break;
            default:
                break;
        }
        if (!cached) {
            if (myELM327.nb_rx_state == ELM_GETTING_MSG) {
                Serial.print(".");
                delay(500);
                retries++;
            } else if (myELM327.nb_rx_state != ELM_SUCCESS) {
                Serial.print("x");
                delay(500);
                retries++;
            }
        }
    } while (!cached && response == 0 && myELM327.nb_rx_state != ELM_SUCCESS && retries < 10);
    if (!cached) {
        Serial.println("");
    }

    if (!cached && response != 0 && myELM327.nb_rx_state == ELM_SUCCESS) {
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
    } else if (!cached && OBD_DEV_MODE) {
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

inline BTScanResults *discoverBtDevices() {
    Serial.println("Discover Bluetooth devices...");

    BTScanResults *btDeviceList = ELM_PORT.getScanResults(); // maybe accessing from different threads!
    if (ELM_PORT.discoverAsync([](BTAdvertisedDevice *pDevice) {
        Serial.printf(">>>>>>>>>>>Found a new device: %s\n", pDevice->toString().c_str());
    })) {
        delay(BT_DISCOVER_TIME);
        Serial.print("Stopping discover...");
        ELM_PORT.discoverAsyncStop();
        Serial.println("stopped");
        delay(5000);

        if (btDeviceList->getCount() > 0) {
            return btDeviceList;
        }
    }

    return nullptr;
}

inline void BTEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    if (event == ESP_SPP_CLOSE_EVT) {
        Serial.println("Bluetooth disconnected.");
        ESP.restart();
    }
}

inline void connectToOBD() {
connect:
    ELM_PORT.register_callback(BTEvent);

    if (!ELM_PORT.begin("OBD2MQTT", true)) {
        Serial.println("========== serialBT failed!");
        ESP.restart();
    }

#if not defined OBD_ADP_MAC
    BTScanResults *btDeviceList = discoverBtDevices();

    if (btDeviceList == nullptr) {
        Serial.println("Didn't find any devices");
    } else {
        BTAddress addr;
        int channel = 0;

        Serial.printf("Search device: %s\n", OBD_ADP_NAME);
        for (int i = 0; i < btDeviceList->getCount(); i++) {
            BTAdvertisedDevice *device = btDeviceList->getDevice(i);
            if (device->getName() == OBD_ADP_NAME) {
                Serial.printf(" ----- %s  %s %d\n", device->getAddress().toString().c_str(),
                              device->getName().c_str(), device->getRSSI());
                std::map<int, std::string> channels = ELM_PORT.getChannels(device->getAddress());
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

        if (addr) {
            Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
            ELM_PORT.connect(addr, channel, sec_mask, role);
        }
    }
#else
    byte mac[6];
    parseBytes(OBD_ADP_MAC, ':', mac, 6, 16);
    BTAddress addr = mac;
    int channel = 0;

    std::map<int, std::string> channels = ELM_PORT.getChannels(addr);
    Serial.printf("scanned for services, found %d\n", channels.size());
    for (auto const &entry: channels) {
        Serial.printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
    }

    if (!channels.empty()) {
        channel = channels.begin()->first;
    }

    if (addr) {
        Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
        if (ELM_PORT.connect(addr, channel, sec_mask, role)) {
            connectedBTAddress = addr.toString().c_str();
        }
    }
#endif

    if (!ELM_PORT.isClosed() && ELM_PORT.connected()) {
        int retryCount = 0;
        while (!myELM327.begin(ELM_PORT, false, 2000, OBD_PROTOCOL) && retryCount < 3) {
            Serial.println("Couldn't connect to OBD scanner - Phase 2");
            delay(BT_DISCOVER_TIME);
            retryCount++;
        }
    } else {
        Serial.println("Couldn't connect to OBD scanner - Phase 1");
    }

    if (!myELM327.connected) {
        delay(BT_DISCOVER_TIME);
        Serial.println("Restarting OBD connect.");
        ELM_PORT.end();
        goto connect;
    }

    Serial.println("Connected to ELM327");

    Serial.println("Cache supported PIDs...");
    isPidSupported(MONITOR_STATUS_SINCE_DTC_CLEARED);
    isPidSupported(DISTANCE_TRAVELED_WITH_MIL_ON);
    isPidSupported(MONITOR_STATUS_THIS_DRIVE_CYCLE);
    isPidSupported(DEMANDED_ENGINE_PERCENT_TORQUE);
    Serial.println("...done.");

    Serial.println("Try to get VIN...");
    char vin[18];
    int retryCount = 0;
    while (myELM327.get_vin_blocking(vin) != ELM_SUCCESS && retryCount < 3) {
        Serial.println("...failed to obtain VIN...");
        delay(500);
        retryCount++;
    }

    VIN = std::string(vin);
    if (!VIN.empty()) {
        Serial.printf("...VIN %s obtained.\n", VIN.c_str());
    } else {
        Serial.println("...VIN is empty.");
    }
}

/**
* Set (int) value and next state.
*
* @param var the variable to set
* @param nextState the next state
* @param value the value to set
* @return <code>true</code> on success
*/
inline bool setStateIntValue(std::atomic<int> &var, obd_pid_states nextState, int value) {
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        var = value;
        obd_state = nextState;
        return true;
    }

    if (myELM327.nb_rx_state != ELM_GETTING_MSG && myELM327.nb_rx_state != ELM_SUCCESS) {
        myELM327.printError();
    }

    if (myELM327.nb_rx_state == ELM_NO_DATA) {
        var = 0;
        obd_state = nextState;
    }

    return false;
}

/**
* Set (float) value and next state.
*
* @param var the variable to set
* @param nextState the next state
* @param value the value to set
* @return <code>true</code> on success
*/
inline bool setStateFloatValue(std::atomic<float> &var, obd_pid_states nextState, float value) {
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        var = value;
        obd_state = nextState;
        return true;
    }

    if (myELM327.nb_rx_state != ELM_GETTING_MSG && myELM327.nb_rx_state != ELM_SUCCESS) {
        myELM327.printError();
    }

    if (myELM327.nb_rx_state == ELM_NO_DATA) {
        var = 0;
        obd_state = nextState;
    }

    return false;
}

/**
 * Calculate the current consumption from MAF Rate.
 *
 * @param fuelType the fuel type
 * @param kph the km/h
 * @param mafRate the MAF rate
 * @return the consumption
 */
inline float calcCurrentConsumption(const int fuelType, const int kph, const float mafRate) {
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

/**
 * Calculate the consumption from MAF Rate.
 *
 * @param fuelType the fuel type
 * @param kph the km/h
 * @param mafRate the MAF rate
 * @return the consumption
 */
inline float calcConsumption(const int fuelType, const int kph, const float mafRate) {
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
        return (mafRate * 3600.0f) / (afRatio * density);
    }

    return 0.0;
}

/**
 * Calculate distance from given km/h and time.
 *
 * @param kph the km/h
 * @param time the time in seconds
 * @return the distance
 */
inline float calcDistance(const int kph, const float time) {
    return kph > 0 ? static_cast<float>(kph) / 3600.0f * time : 0.0f;
}
