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
#include "gsm.h"

#include <settings.h>

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
#include "device_sim800.h"
#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
#include "device_simA76xx.h"
#endif

int GSM::convertSQToRSSI(int signalQuality) {
    if (signalQuality > 0 && signalQuality <= 32) {
        return (111 - signalQuality * 2 - 2) * -1;
    }

    return signalQuality;
}

GSM::GSM(Stream &stream) : stream(stream), modem(TinyGsm(stream)), client(TinyGsmClient(modem)) {
    ipAddress = "";
    reconnectAttempts = 0;
}

void GSM::resetModem() {
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

std::string GSM::getIpAddress() {
    return ipAddress;
}

bool GSM::isUseGPRS() {
#if TINY_GSM_USE_GPRS
    return true;
#else
        return false;
#endif
}

void GSM::connectToNetwork() {
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

#if defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
    modem.setNetworkMode(MODEM_NETWORK_AUTO);
#endif

    if (isUseGPRS()) {
        // Unlock your SIM card with a PIN if needed
        if (!Settings.getSimPin().isEmpty() && modem.getSimStatus() != 3) {
            modem.simUnlock(Settings.getSimPin().c_str());
        }
    }

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
    // The XBee must run the gprsConnect function BEFORE waiting for network!
    Serial.print("Waiting for GPRS connect...");
    modem.gprsConnect(Settings.getMobileAPN().c_str(), Settings.getMobileUsername().c_str(),
        Settings.getMobilePassword().c_str());
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
        if (Settings.getMobileAPN().isEmpty()) {
            Serial.println("No APN was configured");
            return;
        }

        // GPRS connection parameters are usually set after network registration
        Serial.printf("Connecting to %s...", Settings.getMobileAPN().c_str());
        if (!modem.gprsConnect(Settings.getMobileAPN().c_str(), Settings.getMobileUsername().c_str(),
                               Settings.getMobilePassword().c_str())) {
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

bool GSM::checkNetwork() {
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
                Serial.printf("Connecting to %s...", Settings.getMobileAPN().c_str());
                if (!modem.gprsConnect(Settings.getMobileAPN().c_str(), Settings.getMobileUsername().c_str(),
                                       Settings.getMobilePassword().c_str())) {
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

bool GSM::isNetworkConnected() {
    return modem.isNetworkConnected();
}

short int GSM::getSignalQuality() {
    if (isUseGPRS()) {
        return modem.getSignalQuality();
    }
    return 0;
}

bool GSM::hasGSMLocation() {
#if defined TINY_GSM_MODEM_HAS_GSM_LOCATION
    return true;
#else
        return false;
#endif
}

bool GSM::hasGPSLocation() {
#if defined TINY_GSM_MODEM_HAS_GPS
    return true;
#else
    return false;
#endif
}

void GSM::enableGPS() {
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

bool GSM::checkGPS() {
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

bool GSM::readGSMLocation(float &gsmLatitude, float &gsmLongitude, float &gsmAccuracy) {
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
    } else {
        return false;
    }
#endif
    return true;
}

bool GSM::readGPSLocation(float &gpsLatitude, float &gpsLongitude, float &gpsAccuracy) {
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
