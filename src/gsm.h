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
#include <Arduino.h>

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true

#include <TinyGsmClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif

#define SQ_NOT_KNOWN    99

class GSM {
    std::string ipAddress;
    unsigned int reconnectAttempts;

    /**
     * Hard reset modem. Seems to crash after long runs.
     */
    void resetModem();

public:
    Stream &stream;
    TinyGsm modem;
    TinyGsmClient client;

    /**
     * Convert signal quality value to RSSI
     *
     * @param signalQuality the signal quality
     * @return the signal quality as RSSI value
     *
     * @see https://m2msupport.net/m2msupport/atcsq-signal-quality/
     */
    static int convertSQToRSSI(int signalQuality);

    explicit GSM(Stream &stream);

    /**
     * Returns the ip address.
     *
     * @return the ip address
     */
    std::string getIpAddress();

    /**
     * Returns if GPRS/LTE used.
     *
     * @return <code>true</code> if used
     */
    static bool isUseGPRS();

    /**
     * Connects to the configured APN.
     */
    void connectToNetwork();

    /**
     * Checks if network is connected else wise a reconnect is done.
     *
     * @return <code>true</code> if network is active or <code>false</code> on a failure
     */
    bool checkNetwork();

    /**
     * Returns is network is connected.
     *
     * @return <code>true</code> if connected
     */
    bool isNetworkConnected();

    /**
     * Returns the signal quality.
     *
     * @return the signal quality
     */
    short int getSignalQuality();

    /**
     * Returns if GSM location is supported.
     *
     * @return <code>true</code> if supported
     */
    static bool hasGSMLocation();

    /**
     * Returns if GPS location is supported.
     *
     * @return <code>true</code> if supported
     */
    static bool hasGPSLocation();

    /**
     * Enable GPS, if is supported.
     */
    void enableGPS();

    /**
     * Checks if GPS is enabled.
     *
     * @return <code>true</code> if is enabled or <code>false</code> on a failure
     */
    bool checkGPS();

    /**
     * Reads the GSM location.
     *
     * @param gsmLatitude the latitude
     * @param gsmLongitude the longitude
     * @param gsmAccuracy the accuracy
     */
    bool readGSMLocation(float &gsmLatitude, float &gsmLongitude, float &gsmAccuracy);

    /**
     * Reads the GPS location.
     *
     * @param gpsLatitude the latitude
     * @param gpsLongitude the longitude
     * @param gpsAccuracy the accuracy
     */
    bool readGPSLocation(float &gpsLatitude, float &gpsLongitude, float &gpsAccuracy);
};