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
#pragma once

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
#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
#include "device_simA76xx.h"
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

// #define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);

class GSM {
    std::string ipAddress;
    unsigned int reconnectAttempts;

    static void resetModem() {
#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
        digitalWrite(MODEM_POWER_ON, LOW);
        delay(1000);
        digitalWrite(MODEM_POWER_ON, HIGH);
#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
#ifdef BOARD_POWERON_PIN
        digitalWrite(BOARD_POWERON_PIN, LOW);
        delay(1000);
        digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif
#endif

        Serial.print("Reset modem...");
        int retry = 0;
        while (!modem.testAT(1000)) {
            Serial.print(".");
            if (retry++ > 10) {
#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
                digitalWrite(MODEM_PWRKEY, LOW);
                delay(100);
                digitalWrite(MODEM_PWRKEY, HIGH);
                delay(1000);
                digitalWrite(MODEM_PWRKEY, LOW);
#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
                digitalWrite(BOARD_PWRKEY_PIN, LOW);
                delay(100);
                digitalWrite(BOARD_PWRKEY_PIN, HIGH);
                delay(1000);
                digitalWrite(BOARD_PWRKEY_PIN, LOW);
#endif
                retry = 0;
            }
        }
        Serial.println("...success");
    }

public:
    GSM() {
        ipAddress = "";
        reconnectAttempts = 0;
    }

    std::string getIpAddress() const {
        return ipAddress;
    }

    bool isUseGPRS() {
#if TINY_GSM_USE_GPRS
        return true;
#else
        return false;
#endif
    }

    void connectToNetwork() {
        Serial.println("Start modem...");

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
        setupModem();

        // Set GSM module baud rate and UART pins
        SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
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

        delay(3000);
    restart:
        // Restart takes quite some time
        // To skip it, call init() instead of restart()
        Serial.println("Initializing modem...");
        if (!modem.init()) {
            Serial.println("Failed to restart modem, delaying 10s and retrying");
            ESP.restart();
            return;
        }
        // modem.restart();

        String name = modem.getModemName();
        Serial.printf("Modem Name: %s\n", name.c_str());

        String modemInfo = modem.getModemInfo();
        Serial.printf("Modem Info: %s\n", modemInfo.c_str());

        if (isUseGPRS()) {
            // Unlock your SIM card with a PIN if needed
            if (GSM_PIN && modem.getSimStatus() != 3) {
                modem.simUnlock(GSM_PIN);
            }
        }

#if TINY_GSM_USE_WIFI
    // Wifi connection parameters must be set before waiting for the network
    Serial.print(F("Setting SSID/password..."));
    if (!modem.networkConnect(wifiSSID, wifiPass)) {
        Serial.println("...fail");
        delay(10000);
        goto restart;
    }
    Serial.println("...success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
    // The XBee must run the gprsConnect function BEFORE waiting for network!
    Serial.print("Waiting for GPRS connect...");
    modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

        Serial.print("Waiting for network...");
        if (!modem.waitForNetwork()) {
            Serial.println("...fail");
            delay(3000);
            goto restart;
        }
        Serial.println("...success");

        if (modem.isNetworkConnected()) {
            Serial.println("Network connected");
        }

        if (isUseGPRS()) {
            // GPRS connection parameters are usually set after network registration
            Serial.printf("Connecting to %s...", apn);
            if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
                Serial.println("...fail");
                delay(3000);
                goto restart;
            }
            Serial.println("...success");

            if (modem.isGprsConnected()) {
                Serial.println("GPRS connected");
            }

            ipAddress = modem.getLocalIP().c_str();
            Serial.printf("IP Address: %s\n", ipAddress.c_str());
            delay(1000);
        }
    }

    bool checkNetwork() {
        // Make sure we're still registered on the network
        if (!modem.isNetworkConnected()) {
            Serial.println("Network disconnected");

            if (reconnectAttempts > 10) {
                resetModem();
                Serial.print("Restart modem...");
                if (!modem.init()) {
                    Serial.println("...fail");
                    return false;
                }
                Serial.println("...success");
                reconnectAttempts = 0;
            }

            Serial.print("Waiting for network...");
            if (!modem.waitForNetwork(20000L, true)) {
                Serial.println("...fail");
                delay(3000);
                ++reconnectAttempts;
                return false;
            }
            if (modem.isNetworkConnected()) {
                reconnectAttempts = 0;
                Serial.println("Network reconnected");
            }

            if (isUseGPRS()) {
                // and make sure GPRS/EPS is still connected
                if (!modem.isGprsConnected()) {
                    Serial.println("GPRS disconnected");
                    Serial.printf("Connecting to %s...", apn);
                    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
                        Serial.println("...fail");
                        delay(3000);
                        return false;
                    }
                    if (modem.isGprsConnected()) {
                        Serial.println("GPRS reconnected");

                        ipAddress = modem.getLocalIP().c_str();
                        Serial.printf("IP Address: %s\n", ipAddress.c_str());
                    }
                }
            }
        }
        return true;
    }

    bool isNetworkConnected() {
        return modem.isNetworkConnected();
    }

    short int getSignalQuality() {
        if (isUseGPRS()) {
            return modem.getSignalQuality();
        }
        return 0;
    }

    bool hasGSMLocation() {
#if defined TINY_GSM_MODEM_HAS_GSM_LOCATION
        return true;
#else
        return false;
#endif
    }

    bool hasGPSLocation() {
#if defined TINY_GSM_MODEM_HAS_GPS
        return true;
#else
        return false;
#endif
    }

    void connectGPS() {
#if defined TINY_GSM_MODEM_HAS_GPS
#if !defined(TINY_GSM_MODEM_SARAR5)  // not needed for this module
            Serial.print("Enabling GPS/GNSS/GLONASS...");
            while (!modem.enableGPS(MODEM_GPS_ENABLE_GPIO)) {
                Serial.print(".");
            }
            Serial.println("...success");

            modem.setGPSBaud(115200);
#endif
#endif
    }

    bool checkGPS() {
#if defined TINY_GSM_MODEM_HAS_GPS
            if (!modem.isEnableGPS()) {
                Serial.println("GPS/GNSS/GLONASS disabled");
                Serial.print("Enabling GPS/GNSS/GLONASS...");
                if (!modem.enableGPS()) {
                    Serial.println("...fail");
                    return false;
                }
                Serial.println("...success");
            }
#endif
        return true;
    }

    bool readGSMLocation(float &gsmLatitude, float &gsmLongitude, float &gsmAccuracy) {
        if (hasGSMLocation()) {
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
            } else {
                return false;
            }
        }
        return true;
    }

    bool readGPSLocation(float &gpsLatitude, float &gpsLongitude, float &gpsAccuracy) {
#if defined TINY_GSM_MODEM_HAS_GPS
            uint8_t status = 0;
            float gps_latitude = 0;
            float gps_longitude = 0;
            float gps_speed = 0;
            float gps_altitude = 0;
            int gps_vsat = 0;
            int gps_usat = 0;
            float gps_accuracy = 0;

            if (modem.getGPS(&status, &gps_latitude, &gps_longitude, &gps_speed, &gps_altitude, &gps_vsat, &gps_usat,
                             &gps_accuracy)) {
                gpsLatitude = gps_latitude;
                gpsLongitude = gps_longitude;
                gpsAccuracy = gps_accuracy;
            } else {
                return false;
            }
#endif
        return true;
    }
};
