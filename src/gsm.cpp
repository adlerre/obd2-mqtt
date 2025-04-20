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
#elif defined(WS_A7670E)
#include "device_ws.h"
#endif

#if defined(LILYGO_GPS_SHIELD)
#define BOARD_GPS_TX_PIN                    21
#define BOARD_GPS_RX_PIN                    22

#ifndef SerialGPS
#define SerialGPS Serial2
#endif

#include <TinyGPS++.h>

TinyGPSPlus gps;
#endif

#ifdef BOARD_BAT_ADC_PIN
#include <numeric>
#if defined(LILYGO_T_A7670) || defined(LILYGO_T_A7608X)
#include "driver/rtc_io.h"
#include "driver/adc.h"

#include "esp32/ulp.h"
#include "soc/soc.h"
#define ULP_START_OFFSET 32
#endif
#endif

#include "soc/adc_periph.h"

int GSM::convertSQToRSSI(int signalQuality) {
    if (signalQuality > 0 && signalQuality <= 32) {
        return (111 - signalQuality * 2 - 2) * -1;
    }

    return signalQuality;
}

int GSM::adcPeriphNum(const gpio_num_t pin) {
    for (int periph = 0; periph < SOC_ADC_PERIPH_NUM; ++periph) {
        for (int channel = 0; channel < SOC_ADC_MAX_CHANNEL_NUM; ++channel) {
            if (adc_channel_io_map[periph][channel] == pin) return periph;
        }
    }
    return -1;
}

int GSM::adcChannelNum(const gpio_num_t pin) {
    for (int periph = 0; periph < SOC_ADC_PERIPH_NUM; ++periph) {
        for (int channel = 0; channel < SOC_ADC_MAX_CHANNEL_NUM; ++channel) {
            if (adc_channel_io_map[periph][channel] == pin) return channel;
        }
    }
    return -1;
}

void GSM::ulpInit(unsigned int threshold, unsigned int highThreshold, int wakeupPeriod) {
#if defined(BOARD_BAT_ADC_PIN) && defined(LILYGO_T_A7670) || defined(LILYGO_T_A7608X)
    unsigned int idx = adcPeriphNum(static_cast<gpio_num_t>(BOARD_BAT_ADC_PIN));

    if (idx == 0) {
        unsigned int channel = adcChannelNum(static_cast<gpio_num_t>(BOARD_BAT_ADC_PIN));

        // ULP program
        // @see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ulp_macros.html
        const ulp_insn_t program[] = {
            I_MOVI(R0, 0),
            I_MOVI(R1, 0),
            I_MOVI(R2, 0),
            I_MOVI(R3, 0),

            M_LABEL(1),
            /* */I_ADDI(R0, R0, 1), // increment cycle counter (reg. R0)
            /* */I_ADC(R3, idx, channel), // read ADC value to reg. R3
            /* */I_ADDR(R1, R1, R3), // add ADC value from reg R3 to reg. R1
            /* */I_DELAY(40000), // delay 5 ms
            M_BL(1, 4), // if cycle counter is less than 4, go to LABEL 1

            I_RSHI(R3, R1, 2), // divide accumulated ADC value in reg. R1 by 4 and save it to reg. R1
            I_MOVR(R1, R3),
            I_MOVR(R0, R1),
            I_MOVI(R3, 0),
            M_BGE(1, highThreshold),
            I_MOVI(R0, 1), // set address offset
            I_ST(R1, R0, 0), // RTC_SLOW_MEM [R0 + 0] = R1;

            M_LABEL(2),
            /* */I_STAGE_RST(),
            /* */M_LABEL(3),
            /*  */I_MOVI(R0, 200), // R0 = n * 1000 / 5, where n is the number of seconds to delay, 200 = 1 s
            /*  */M_LABEL(4),
            /*   */ // since ULP runs at 8 MHz
            /*   */ // 40000 cycles correspond to 5 ms (max possible delay is 65535 cycles or 8.19 ms)
            /*   */I_DELAY(40000),
            /*   */I_SUBI(R0, R0, 1), // R0 --;
            /*  */M_BGE(4, 1), // } while (R0 >= 1); ... jump to label 3 if R0 > 0
            /*  */I_STAGE_INC(1),
            /* */M_BSLT(3, 5), // if stage counter is less than 5, jump to label 3

            /* */I_MOVI(R0, 0), // reset R0
            /* */I_MOVI(R2, 0), // reset R2
            /* */I_MOVI(R3, 0), // reset R3

            /* */M_LABEL(5),
            /*  */I_ADDI(R0, R0, 1), // increment cycle counter (reg. R0)
            /*  */I_ADC(R3, idx, channel), // read ADC value to reg. R3
            /*  */I_ADDR(R2, R2, R3), // add ADC value from reg R3 to reg. R2
            /*  */I_DELAY(40000),
            /* */M_BL(5, 4), // if cycle counter is less than 4, go to LABEL 4

            /* */I_RSHI(R3, R2, 2), // divide accumulated ADC value in reg. R2 by 4 and save it to reg. R2
            /* */I_MOVR(R2, R3),
            /* */I_MOVI(R0, 2), // set address offset
            /* */I_ST(R2, R0, 0), // RTC_SLOW_MEM [R0 + 0] = R2;

            /* */I_SUBR(R3, R2, R1), // R3 = R2 - R1
            /* */I_MOVI(R0, 0), // set address offset
            /* */I_ST(R3, R0, 0), // RTC_SLOW_MEM [R0 + 0] = R0;
            /* */I_MOVR(R0, R3),
            /* */M_BXF(2), // on ALU overflow jump to label 2
            /* */M_BGE(6, threshold), // R0 < threshold (mV) ... jump to label 5
            M_BX(2), // while (1)

            M_LABEL(6),
            /* */I_MOVI(R0, 0),
            /* */ // check is allowed to wake up
            /* */I_RD_REG(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP_S, RTC_CNTL_RDY_FOR_WAKEUP_S),
            /* */M_BGE(7, 1), // if allowed to wake up jump to label 6
            M_BX(6), // else jump to label 5

            M_LABEL(7),
            /* */I_WAKE(), // WAKEUP ESP32

            I_END(), // END ULP timer
            I_HALT() // STOP ULP PROCESS
        };

        size_t ulpSize = sizeof(program) / sizeof(ulp_insn_t);

        const esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        switch (wakeup_reason) {
            case ESP_SLEEP_WAKEUP_EXT0:
                log_d("Wakeup caused by external signal using RTC_IO");
                break;
            case ESP_SLEEP_WAKEUP_EXT1:
                log_d("Wakeup caused by external signal using RTC_CNTL");
                break;
            case ESP_SLEEP_WAKEUP_TIMER:
                log_d("Wakeup caused by timer");
                break;
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                log_d("Wakeup caused by touchpad");
                break;
            case ESP_SLEEP_WAKEUP_ULP:
                log_d("Wakeup caused by ULP program");
                log_d("threshold result = %d\n", RTC_SLOW_MEM[0] & 0xFFF);
                break;
            default:
                log_d("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
                break;
        }

        // Clear rtc memory
        memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);

        // Load ULP program
        ESP_ERROR_CHECK(ulp_process_macros_and_load(ULP_START_OFFSET, program, &ulpSize));
        if (wakeupPeriod > 0) {
            ESP_ERROR_CHECK(ulp_set_wakeup_period(0, wakeupPeriod * 1000 * 1000));
        }

        // Run ULP program
        ESP_ERROR_CHECK(ulp_run(ULP_START_OFFSET));
    }
#endif
}

void GSM::ulpPrepareSleep() {
#if defined(BOARD_BAT_ADC_PIN) && defined(LILYGO_T_A7670) || defined(LILYGO_T_A7608X)
    unsigned int idx = adcPeriphNum(static_cast<gpio_num_t>(BOARD_BAT_ADC_PIN));

    if (idx == 0) {
        unsigned int channel = adcChannelNum(static_cast<gpio_num_t>(BOARD_BAT_ADC_PIN));

        // configure channel for ulp usage
        adc1_config_channel_atten(static_cast<adc1_channel_t>(channel), ADC_ATTEN_DB_12);
        adc1_config_width(ADC_WIDTH_BIT_12);
        // @FIXME read adc value before ULP enable, fixes reading issues in deep sleep
        adc1_get_raw(static_cast<adc1_channel_t>(channel));
        adc1_ulp_enable();

        ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    }
#endif
}

GSM::GSM(Stream &stream) : stream(stream), modem(TinyGsm(stream)) {
    ipAddress = "";
    reconnectAttempts = 0;
#if defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
    networkMode = MODEM_NETWORK_AUTO;
#endif
}

Client *GSM::getClient(const bool useSecure) {
    if (useSecure) {
        if (client != nullptr) {
            free(client);
        }
        if (secureClient == nullptr) {
#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
            Serial.println("WARNING: SIM800 devices supports only SSL 2/3 and TLS 1.0");
#endif
            secureClient = new TinyGsmClientSecure(modem);
        }

        return secureClient;
    }

    if (secureClient != nullptr) {
        free(secureClient);
    }
    if (client == nullptr) {
        client = new TinyGsmClient(modem);
    }
    return client;
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

void GSM::setNetworkMode(int mode) {
    networkMode = mode;
}

void GSM::connectToNetwork() {
    Serial.println("Start modem...");

#if defined(SIM800L_IP5306_VERSION_20190610) or defined(SIM800L_AXP192_VERSION_20200327) or defined(SIM800C_AXP192_VERSION_20200609) or defined(SIM800L_IP5306_VERSION_20200811)
    setupModem();

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

#elif defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X) or defined(WS_A7670E)
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
        Serial.println("SIM Card insert?");
        delay(10000L);
        ESP.restart();
        return;
    }
    // modem.restart();

    String name = modem.getModemName();
    Serial.printf("Modem Name: %s\n", name.c_str());

    String modemInfo = modem.getModemInfo();
    Serial.printf("Modem Info: %s\n", modemInfo.c_str());

#if defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
    modem.setNetworkMode(static_cast<NetworkMode>(networkMode));
#endif

    if (isUseGPRS()) {
        // Unlock your SIM card with a PIN if needed
        if (!Settings.Mobile.getPin().isEmpty() && modem.getSimStatus() != 3) {
            modem.simUnlock(Settings.Mobile.getPin().c_str());
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
        if (Settings.Mobile.getAPN().isEmpty()) {
            Serial.println("No APN was configured");
            return;
        }

        // GPRS connection parameters are usually set after network registration
        Serial.printf("Connecting to %s...", Settings.Mobile.getAPN().c_str());
        if (!modem.gprsConnect(Settings.Mobile.getAPN().c_str(), Settings.Mobile.getUsername().c_str(),
                               Settings.Mobile.getPassword().c_str())) {
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

void GSM::powerOff() {
    modem.poweroff();

    // check if not response
    while (modem.testAT()) {
        delay(500);
    }
}

bool GSM::checkNetwork(bool resetConnection) {
    // Make sure we're still registered on the network
    if (!modem.isNetworkConnected() || resetConnection) {
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

#if defined(LILYGO_T_A7670) or defined(LILYGO_T_CALL_A7670_V1_0) or defined(LILYGO_T_CALL_A7670_V1_1) or defined(LILYGO_T_A7608X)
        modem.setNetworkMode(static_cast<NetworkMode>(networkMode));
#endif

        if (isUseGPRS()) {
            if (!Settings.Mobile.getPin().isEmpty() && modem.getSimStatus() != 3) {
                modem.simUnlock(Settings.Mobile.getPin().c_str());
            }
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
                Serial.printf("Connecting to %s...", Settings.Mobile.getAPN().c_str());
                if (!modem.gprsConnect(Settings.Mobile.getAPN().c_str(), Settings.Mobile.getUsername().c_str(),
                                       Settings.Mobile.getPassword().c_str())) {
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
#if defined(TINY_GSM_MODEM_HAS_GPS) || defined(LILYGO_GPS_SHIELD)
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
#elif defined LILYGO_GPS_SHIELD
    SerialGPS.begin(9600, SERIAL_8N1, BOARD_GPS_RX_PIN, BOARD_GPS_TX_PIN);
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
#elif defined(LILYGO_GPS_SHIELD)
    while (SerialGPS.available()) {
        int c = SerialGPS.read();
        if (gps.encode(c)) {
            if (gps.location.isValid()) {
                gpsLatitude = gps.location.lat();
                gpsLongitude = gps.location.lng();
                gpsAccuracy = 5;
            } else {
                return false;
            }
        }
    }
#endif
    return true;
}

bool GSM::hasBattery() {
#ifdef BOARD_BAT_ADC_PIN
    return true;
#else
    return false;
#endif
}

bool GSM::isBatteryUsed() {
#ifdef BOARD_BAT_ADC_PIN
    pinMode(BOARD_BAT_ADC_PIN, INPUT);
    return digitalRead(BOARD_BAT_ADC_PIN) == HIGH;
#else
    return false;
#endif
}

unsigned int GSM::getBatteryVoltage() {
#ifdef BOARD_BAT_ADC_PIN
    return analogReadMilliVolts(BOARD_BAT_ADC_PIN) * 2;
#else
    return 0;
#endif
}

void GSM::deepSleep(uint32_t ms) {
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    ulpPrepareSleep();
    esp_deep_sleep_start();
}
