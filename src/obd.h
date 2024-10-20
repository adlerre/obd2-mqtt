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
#include <atomic>
#include <BluetoothSerial.h>

#include "ELMduino.h"

// set default adapter name
#ifndef OBD_ADP_NAME
#define OBD_ADP_NAME        "OBDII"
#endif

#define BT_DISCOVER_TIME    10000

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

#define KPH_TO_MPH          1.60934f
#define LITER_TO_GALLON     3.7854f

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
    BAT_VOLTAGE,
    PEDAL_POS,
    MILSTATUS
} obd_pid_states;

typedef enum {
    METRIC,
    IMPERIAL
} measurementSystem;

class OBDClass {
    BluetoothSerial serialBt;
    ELM327 elm327;

    bool initDone = false;
    bool stopConnect = false;

    String devName;
    String devMac;
    char protocol;
    bool checkPidSupport = false;

    obd_pid_states obd_state = ENG_LOAD;

    // cached PIDs
    uint32_t supportedPids_1_20{0};
    bool supportedPids_1_20_cached{false};
    uint32_t supportedPids_21_40{0};
    bool supportedPids_21_40_cached{false};
    uint32_t supportedPids_41_60{0};
    bool supportedPids_41_60_cached{false};
    uint32_t supportedPids_61_80{0};
    bool supportedPids_61_80_cached{false};

    int load{0};
    int throttle{0};
    float rpm{0};
    float coolantTemp{0};
    float oilTemp{0};
    float ambientAirTemp{0};
    int kph{0};
    float fuelLevel{0};
    float fuelRate{0};
    uint8_t fuelType{0};
    bool fuelTypeRead{false};
    float mafRate{0};
    float batVoltage{0};
    float intakeAirTemp{0};
    uint8_t manifoldPressure{0};
    float timingAdvance{0};
    float pedalPosition{0};
    uint32_t monitorStatus{0};
    bool milState{false};

    unsigned long lastReadSpeed{0};
    unsigned long runStartTime{0};
    float curConsumption{0};
    float consumption{0};
    float consumptionPer100{0};
    float distanceDriven{0};
    float avgSpeed{0};
    int topSpeed{0};

    std::string connectedBTAddress;
    std::string VIN;

    std::function<void(BTScanResults *scanResult)> devDiscoveredCallback = nullptr;

    BTScanResults *discoverBtDevices();

    static void BTEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

    /**
     * Set value and next state.
     *
     * @param var the variable to set
     * @param nextState the next state
     * @param value the value to set
     * @return <code>true</code> on success
     */
    template<typename T>
    bool setStateValue(T &var, obd_pid_states nextState, T value);
protected:
public:
    OBDClass();

    void begin(const String &devName, const String &devMac, char protocol = AUTOMATIC, bool checkPidSupport = false);

    void end();

    void connect(bool reconnect = false);

    void loop();

    void onDevicesDiscovered(const std::function<void(BTScanResults *scanResult)> &callable);

    /**
     * Checks if PID is supported.
     *
     * @param pid the PID
     */
    bool isPidSupported(uint8_t pid);

    uint32_t getSupportedPids1To20() const;

    uint32_t getSupportedPids21To40() const;

    uint32_t getSupportedPids41To60() const;

    uint32_t getSupportedPids61To80() const;

    std::string vin() const;

    std::string getConnectedBTAddress() const;

    int getLoad() const;

    int getThrottle() const;

    float getRPM() const;

    float getCoolantTemp() const;

    float getOilTemp() const;

    float getAmbientAirTemp() const;

    int getSpeed(measurementSystem system = METRIC) const;

    float getFuelLevel() const;

    float getFuelRate(measurementSystem system = METRIC) const;

    uint8_t getFuelType() const;

    bool getFuelTypeRead() const;

    float getMafRate() const;

    float getBatVoltage() const;

    float getIntakeAirTemp() const;

    uint8_t getManifoldPressure() const;

    float getTimingAdvance() const;

    float getPedalPosition() const;

    uint32_t getMonitorStatus() const;

    bool getMilState() const;

    unsigned long getLastReadSpeed() const;

    unsigned long getRunStartTime() const;

    float getCurConsumption(measurementSystem system = METRIC) const;

    float getConsumption(measurementSystem system = METRIC) const;

    float getConsumptionForMeasurement(measurementSystem system = METRIC) const;

    float getDistanceDriven(measurementSystem system = METRIC) const;

    float getAvgSpeed(measurementSystem system = METRIC) const;

    int getTopSpeed(measurementSystem system = METRIC) const;

    /**
     * Calculate the current consumption from MAF Rate.
     *
     * @param fuelType the fuel type
     * @param kph the km/h
     * @param mafRate the MAF rate
     * @return the consumption
     */
    static float calcCurrentConsumption(int fuelType, int kph, float mafRate);

    /**
     * Calculate the consumption from MAF Rate.
     *
     * @param fuelType the fuel type
     * @param kph the km/h
     * @param mafRate the MAF rate
     * @return the consumption
     */
    static float calcConsumption(int fuelType, int kph, float mafRate);

    /**
     * Calculate distance from given km/h and time.
     *
     * @param kph the km/h
     * @param time the time in seconds
     * @return the distance
     */
    static float calcDistance(int kph, float time);
};

extern OBDClass OBD;
