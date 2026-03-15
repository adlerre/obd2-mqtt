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

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
#include "device_sim800.h"
#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
#include "device_simA76xx.h"
#elif defined(LILYGO_SIM7000G) or defined(LILYGO_SIM7070G)
#include "device_sim7xxx.h"
#elif defined(WS_A7670E) or defined(WS_A7670E_R2)
#include "device_ws.h"
#endif

#if defined(BOARD_BAT_ADC_PIN) or defined(MAX17048_I2C_ADDRESS)
#define DEVICE_HAS_BATTERY      true

#if defined(BOARD_BAT_ADC_PIN)
#define DEVICE_BATTERY_VOLTAGE  true
#define DEVICE_BATTERY_LEVEL    false

#include <numeric>
#define DEVICE_CAN_DEEP_SLEEP   true
#include "driver/rtc_io.h"
#include "driver/adc.h"

#include "esp32/ulp.h"
#include "soc/soc.h"
#define ULP_START_OFFSET 32

#elif defined(MAX17048_I2C_ADDRESS)
#define DEVICE_BATTERY_VOLTAGE  false
#define DEVICE_BATTERY_LEVEL    true
#define DEVICE_CAN_DEEP_SLEEP   false

#include <Wire.h>
#endif

#else
#define DEVICE_HAS_BATTERY      false
#define DEVICE_CAN_DEEP_SLEEP   false
#endif

#if defined(LILYGO_GPS_SHIELD)
#define BOARD_GPS_TX_PIN                    21
#define BOARD_GPS_RX_PIN                    22
#endif

#define SQ_NOT_KNOWN    99

class GSM {
    std::string ipAddress;
    unsigned int reconnectAttempts;
    int networkMode;

    TinyGsmClient *client = nullptr;
    TinyGsmClientSecure *secureClient = nullptr;

    /**
     * Returns the ADC index for given GPIO.
     *
     * @param pin the GPIO num
     * @return <code>-1</code> if nothing found or the ADC index
     */
    static int adcPeriphNum(gpio_num_t pin);

    /**
     * Returns the ADC channel for given GPIO.
     *
     * @param pin the GPIO num
     * @return <code>-1</code> if nothing found or the ADC channel
     */
    static int adcChannelNum(gpio_num_t pin);

    /**
     * Prepares ULP for deep sleep.
     */
    static void ulpPrepareSleep();

    /**
     * Hard reset modem. Seems to crash after long runs.
     */
    void resetModem();

public:
    Stream &stream;
    TinyGsm modem;

    /**
     * Convert signal quality value to RSSI
     *
     * @param signalQuality the signal quality
     * @return the signal quality as RSSI value
     *
     * @see https://m2msupport.net/m2msupport/atcsq-signal-quality/
     */
    static int convertSQToRSSI(int signalQuality);

    /**
     * Inits the ULP code
     *
     * @param threshold this value must differ from first ULP reading, default 10mV
     * @param highThreshold if above this value, initial measurement is done again, default 3000mV
     * @param wakeupPeriod the number of seconds to restart ULP program
     *
     * @note You must init the ULP within setup() routine to get ADC reading working.
     */
    static void ulpInit(unsigned int threshold = 10, unsigned int highThreshold = 3000, int wakeupPeriod = -1);

    explicit GSM(Stream &stream);

    /**
     * Returns the underlying TinyGSM*Client
     *
     * @param useSecure <code>true</code> if a secured connection should be used
     * @return the client pointer
     */
    Client *getClient(bool useSecure = false);

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
     * Set network mode.
     *
     * @param mode the network mode, default <code>2</code> means AUTO
     */
    void setNetworkMode(int mode = 2);

    /**
     * Connects to the configured APN.
     */
    void connectToNetwork();

    /**
     * Power off modem.
     */
    void powerOff();

    /**
     * Checks if network is connected else wise a reconnect is done.
     *
     * @return <code>true</code> if network is active or <code>false</code> on a failure
     */
    bool checkNetwork(bool resetConnection = false);

    /**
     * Returns is network is connected.
     *
     * @return <code>true</code> if connected
     */
    bool isNetworkConnected();

    // bool updateLocaleTime();

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

    /**
     * Init battery.
     */
    static void initBattery();

    /**
     * Returns if battery is supported.
     *
     * @return <code>true</code> if supported
     */
    static bool hasBattery();

    /**
     * Returns if battery is in use.
     *
     * @return <code>true</code> if used
     */
    static bool isBatteryUsed();

    /**
     * Returns the battery type.
     *
     * @return -1 N/A, 0 voltage, 1 level
     */
    static int getBatteryType();

    /**
     * Reads the battery voltage.
     *
     * @return the battery voltage in mV
     */
    static unsigned int getBatteryVoltage();

    /**
     * Reads the battery level.
     *
     * @return the battery level in %
     */
    static float getBatteryLevel();

    /**
     * Returns if deep sleep is supported/implemented for this board.
     *
     * @return <code>true</code> if can
     */
    static bool canDeepSleep();

    /**
     * Enter deep sleep.
     *
     * @param ms the number of milliseconds to wait before wakeup
     */
    static void deepSleep(uint32_t ms);
};
