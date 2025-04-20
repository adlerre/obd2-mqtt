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

class GeneralSettings {
    struct {
        int sleepTimeout;
        int sleepDuration;
    } general{};

    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

    friend class SettingsClass;

public:
    int getSleepTimeout() const;

    void setSleepTimeout(int timeout);

    int getSleepDuration() const;

    void setSleepDuration(int time);
};

class WiFiSettings {
    struct {
        char ssid[65];
        char password[33];
    } wifi{};

    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

    friend class SettingsClass;

public:
    String getAPSSID(const String &alternate = String()) const;

    void setAPSSID(const char *ssid);

    String getAPPassword() const;

    void setAPPassword(const char *password);
};

class MobileSettings {
    struct {
        int networkMode;
        char pin[5];
        char apn[65];
        char username[33];
        char password[33];
    } mobile{};

    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

    friend class SettingsClass;

public:
    int getNetworkMode() const;

    void setNetworkMode(int networkMode);

    String getPin() const;

    void setPin(const char *simPin);

    String getAPN() const;

    void setAPN(const char *apn);

    String getUsername() const;

    void setUsername(const char *username);

    String getPassword() const;

    void setPassword(const char *password);
};

class OBD2Settings {
    struct {
        char name[65];
        char mac[19];
        bool checkPIDSupport;
        bool debug;
        bool specifyNumResponses;
        char protocol;
    } obd2{};

    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

    friend class SettingsClass;

public:
    String getName(const String &alternate = String()) const;

    void setName(const char *name);

    String getMAC() const;

    void setMAC(const char *mac);

    bool getCheckPIDSupport() const;

    void setCheckPIDSupport(bool checkPIDSupport);

    bool getDebug() const;

    void setDebug(bool debug);

    bool getSpecifyNumResponses() const;

    void setSpecifyNumResponses(bool specifyNumResponses);

    char getProtocol() const;

    void setProtocol(char protocol);
};

class MQTTSettings {
    struct {
        int protocol;
        char hostname[65];
        unsigned int port;
        bool secure;
        char username[33];
        char password[33];
        bool allowOffline;
        unsigned int dataInterval;
        unsigned int diagnosticInterval;
        unsigned int discoveryInterval;
        unsigned int locationInterval;
    } mqtt{};

    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

    friend class SettingsClass;

public:
    int getProtocol() const;

    void setProtocol(int protocol);

    String getHostname() const;

    void setHostname(const char *hostname);

    unsigned int getPort() const;

    void setPort(unsigned int port);

    bool getSecure() const;

    void setSecure(bool secure);

    String getUsername() const;

    void setUsername(const char *username);

    String getPassword() const;

    void setPassword(const char *password);

    bool getAllowOffline() const;

    void setAllowOffline(bool allowOffline);

    unsigned int getDataInterval() const;

    void setDataInterval(unsigned int dataInterval);

    unsigned int getDiagnosticInterval() const;

    void setDiagnosticInterval(unsigned int diagnosticInterval);

    unsigned int getDiscoveryInterval() const;

    void setDiscoveryInterval(unsigned int discoveryInterval);

    unsigned int getLocationInterval() const;

    void setLocationInterval(unsigned int locationInterval);
};

class SettingsClass {
    void readJson(JsonDocument &doc);

    void writeJson(JsonDocument &doc);

public:
    SettingsClass();

    GeneralSettings General;

    WiFiSettings WiFi;

    MobileSettings Mobile;

    OBD2Settings OBD2;

    MQTTSettings MQTT;

    bool readSettings(fs::FS &fs);

    bool writeSettings(fs::FS &fs);

    std::string buildJson();

    bool parseJson(std::string json);
};

extern SettingsClass Settings;
