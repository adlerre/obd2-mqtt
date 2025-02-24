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
#include <ArduinoJson.h>
#include <FS.h>

/**
 * @see https://arduinojson.org/v7/how-to/add-support-for-char/
 */
namespace ArduinoJson {
    template<>
    struct Converter<char> {
        static void toJson(char c, JsonVariant var) {
            char buf[] = {c, 0}; // create a string of length 1
            var.set(buf);
        }

        static char fromJson(JsonVariantConst src) {
            auto p = src.as<const char *>();
            return p ? p[0] : 0; // returns the first character or 0
        }

        static bool checkJson(JsonVariantConst src) {
            auto p = src.as<const char *>();
            return p && p[0] && !p[1]; // must be a string of length 1
        }
    };
}

#define SETTINGS_FILE "/settings.json"

struct GeneralSettings {
    int sleepTimeout;
    int sleepDuration;
};

struct WiFiSettings {
    char ssid[65];
    char password[33];
};

struct MobileSettings {
    int networkMode;
    char pin[5];
    char apn[65];
    char username[33];
    char password[33];
};

struct OBD2Settings {
    char name[65];
    char mac[19];
    char pin[7];
    bool checkPIDSupport;
    char protocol;
};

struct MQTTSettings {
    int protocol;
    char hostname[65];
    unsigned int port;
    bool secure;
    char username[33];
    char password[33];
    unsigned int dataInterval;
    unsigned int diagnosticInterval;
    unsigned int discoveryInterval;
    unsigned int locationInterval;
};

class SettingsClass {
    GeneralSettings general{};
    WiFiSettings wifi{};
    MobileSettings mobile{};
    OBD2Settings obd2{};
    MQTTSettings mqtt{};

    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

public:
    SettingsClass();

    bool readSettings(fs::FS &fs);

    bool writeSettings(fs::FS &fs);

    std::string buildJson();

    bool parseJson(std::string json);

    int getSleepTimeout() const;

    void setSleepTimeout(int timeout);

    int getSleepDuration() const;

    void setSleepDuration(int time);

    String getWiFiAPSSID(const String &alternate = String()) const;

    void setWiFiAPSSID(const char *ssid);

    String getWiFiAPPassword() const;

    void setWiFiAPPassword(const char *password);

    int getMobileNetworkMode() const;

    void setMobileNetworkMode(int networkMode);

    String getSimPin() const;

    void setSimPin(const char *simPin);

    String getMobileAPN() const;

    void setMobileAPN(const char *apn);

    String getMobileUsername() const;

    void setMobileUsername(const char *username);

    String getMobilePassword() const;

    void setMobilePassword(const char *password);

    String getOBD2Name(const String &alternate = String()) const;

    void setOBD2Name(const char *name);

    String getOBD2MAC() const;

    void setOBD2MAC(const char *mac);

    String getOBD2Pin() const;

    void setOBD2Pin(const char *pin);

    bool getOBD2CheckPIDSupport() const;

    void setOBD2CheckPIDSupport(bool checkPIDSupport);

    char getOBD2Protocol() const;

    void setOBD2Protocol(char protocol);

    int getMQTTProtocol() const;

    void setMQTTProtocol(int protocol);

    String getMQTTHostname() const;

    void setMQTTHostname(const char *hostname);

    unsigned int getMQTTPort() const;

    void setMQTTPort(unsigned int port);

    bool getMQTTSecure() const;

    void setMQTTSecure(bool secure);

    String getMQTTUsername() const;

    void setMQTTUsername(const char *username);

    String getMQTTPassword() const;

    void setMQTTPassword(const char *password);

    unsigned int getMQTTDataInterval() const;

    void setMQTTDataInterval(unsigned int dataInterval);

    unsigned int getMQTTDiagnosticInterval() const;

    void setMQTTDiagnosticInterval(unsigned int diagnosticInterval);

    unsigned int getMQTTDiscoveryInterval() const;

    void setMQTTDiscoveryInterval(unsigned int discoveryInterval);

    unsigned int getMQTTLocationInterval() const;

    void setMQTTLocationInterval(unsigned int locationInterval);
};

extern SettingsClass Settings;
