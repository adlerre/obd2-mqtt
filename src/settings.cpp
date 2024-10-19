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
#include "settings.h"

#include <FS.h>
#include <ArduinoJson.h>

SettingsClass::SettingsClass() = default;

void SettingsClass::readJson(JsonDocument &doc) {
    general.measurementSystem = doc["general"]["measurementSystem"] | 0;

    strlcpy(wifi.ssid, doc["wifi"]["ssid"] | "", sizeof(wifi.ssid));
    strlcpy(wifi.password, doc["wifi"]["password"] | "", sizeof(wifi.password));

    strlcpy(mobile.pin, doc["mobile"]["pin"] | "", sizeof(mobile.pin));
    strlcpy(mobile.apn, doc["mobile"]["apn"] | "", sizeof(mobile.apn));
    strlcpy(mobile.username, doc["mobile"]["username"] | "", sizeof(mobile.username));
    strlcpy(mobile.password, doc["mobile"]["password"] | "", sizeof(mobile.password));

    strlcpy(obd2.name, doc["obd2"]["name"] | "", sizeof(obd2.name));
    strlcpy(obd2.mac, doc["obd2"]["mac"] | "", sizeof(obd2.mac));
    obd2.checkPIDSupport = doc["obd2"]["checkPIDSupport"] | false;
    obd2.protocol = doc["obd2"]["protocol"] | '0';

    strlcpy(mqtt.hostname, doc["mqtt"]["hostname"] | "", sizeof(mqtt.hostname));
    mqtt.port = doc["mqtt"]["port"] | 1883;
    strlcpy(mqtt.username, doc["mqtt"]["username"] | "", sizeof(mqtt.username));
    strlcpy(mqtt.password, doc["mqtt"]["password"] | "", sizeof(mqtt.password));
    mqtt.dataInterval = doc["mqtt"]["dataInterval"] | 1;
    mqtt.diagnosticInterval = doc["mqtt"]["diagnosticInterval"] | 30;
    mqtt.discoveryInterval = doc["mqtt"]["discoveryInterval"] | 300;
    mqtt.locationInterval = doc["mqtt"]["locationInterval"] | 30;
}

void SettingsClass::writeJson(JsonDocument &doc) {
    doc["general"]["measurementSystem"] = general.measurementSystem;

    doc["wifi"]["ssid"] = wifi.ssid;
    doc["wifi"]["password"] = wifi.password;

    doc["mobile"]["pin"] = mobile.pin;
    doc["mobile"]["apn"] = mobile.apn;
    doc["mobile"]["username"] = mobile.username;
    doc["mobile"]["password"] = mobile.password;

    doc["obd2"]["name"] = obd2.name;
    doc["obd2"]["mac"] = obd2.mac;
    doc["obd2"]["checkPIDSupport"] = obd2.checkPIDSupport;
    doc["obd2"]["protocol"] = obd2.protocol;

    doc["mqtt"]["hostname"] = mqtt.hostname;
    doc["mqtt"]["port"] = mqtt.port;
    doc["mqtt"]["username"] = mqtt.username;
    doc["mqtt"]["password"] = mqtt.password;
    doc["mqtt"]["dataInterval"] = mqtt.dataInterval;
    doc["mqtt"]["diagnosticInterval"] = mqtt.diagnosticInterval;
    doc["mqtt"]["discoveryInterval"] = mqtt.discoveryInterval;
    doc["mqtt"]["locationInterval"] = mqtt.locationInterval;
}

bool SettingsClass::readSettings(fs::FS &fs) {
    bool success = false;

    File file = fs.open(SETTINGS_FILE, FILE_READ);
    if (file && !file.isDirectory()) {
        JsonDocument doc;
        if (!deserializeJson(doc, file)) {
            readJson(doc);
            success = true;
        }
        file.close();
    }

    return success;
}

bool SettingsClass::writeSettings(fs::FS &fs) {
    bool success = false;

    File file = fs.open(SETTINGS_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file settings.json for writing.");
        return false;
    }

    JsonDocument doc;
    writeJson(doc);
    success = serializeJson(doc, file);

    file.close();

    return success;
}

std::string SettingsClass::buildJson() {
    std::string payload;

    JsonDocument doc;
    writeJson(doc);
    serializeJson(doc, payload);

    return payload;
}

bool SettingsClass::parseJson(std::string json) {
    bool success = false;
    JsonDocument doc;
    if (!deserializeJson(doc, json)) {
        readJson(doc);
        success = true;
    }

    return success;
}

int SettingsClass::getMeasurementSystem() const {
    return general.measurementSystem;
}

void SettingsClass::setMeasurementSystem(int measurementSystem) {
    general.measurementSystem = measurementSystem;
}

String SettingsClass::getWiFiAPSSID(const String &alternate) const {
    if (strlen(wifi.ssid) == 0) {
        return alternate;
    }
    return wifi.ssid;
}

void SettingsClass::setWiFiAPSSID(const char *ssid) {
    strlcpy(wifi.ssid, ssid, sizeof(wifi.ssid));
}

String SettingsClass::getWiFiAPPassword() const {
    return wifi.password;
}

void SettingsClass::setWiFiAPPassword(const char *password) {
    strlcpy(wifi.password, password, sizeof(wifi.password));
}

String SettingsClass::getSimPin() const {
    return mobile.pin;
}

void SettingsClass::setSimPin(const char *simPin) {
    strlcpy(mobile.pin, simPin, sizeof(mobile.pin));
}

String SettingsClass::getMobileAPN() const {
    return mobile.apn;
}

void SettingsClass::setMobileAPN(const char *apn) {
    strlcpy(mobile.apn, apn, sizeof(mobile.apn));
}

String SettingsClass::getMobileUsername() const {
    return mobile.username;
}

void SettingsClass::setMobileUsername(const char *username) {
    strlcpy(mobile.username, username, sizeof(mobile.username));
}

String SettingsClass::getMobilePassword() const {
    return mobile.password;
}

void SettingsClass::setMobilePassword(const char *password) {
    strlcpy(mobile.password, password, sizeof(mobile.password));
}

String SettingsClass::getOBD2Name(const String &alternate) const {
    if (strlen(obd2.name) == 0) {
        return alternate;
    }
    return obd2.name;
}

void SettingsClass::setOBD2Name(const char *name) {
    strlcpy(obd2.name, name, sizeof(obd2.name));
}

String SettingsClass::getOBD2MAC() const {
    return obd2.mac;
}

void SettingsClass::setOBD2MAC(const char *mac) {
    strlcpy(obd2.mac, mac, sizeof(obd2.mac));
}

bool SettingsClass::getOBD2CheckPIDSupport() const {
    return obd2.checkPIDSupport;
}

void SettingsClass::setOBD2CheckPIDSupport(bool checkPIDSupport) {
    obd2.checkPIDSupport = checkPIDSupport;
}

char SettingsClass::getOBD2Protocol() const {
    return obd2.protocol;
}

void SettingsClass::setOBD2Protocol(char protocol) {
    obd2.protocol = protocol;
}

String SettingsClass::getMQTTHostname() const {
    return mqtt.hostname;
}

void SettingsClass::setMQTTHostname(const char *hostname) {
    strlcpy(mqtt.hostname, hostname, sizeof(mqtt.hostname));
}

unsigned int SettingsClass::getMQTTPort() const {
    return mqtt.port;
}

void SettingsClass::setMQTTPort(unsigned int port) {
    mqtt.port = port;
}

String SettingsClass::getMQTTUsername() const {
    return mqtt.username;
}

void SettingsClass::setMQTTUsername(const char *username) {
    strlcpy(mqtt.username, username, sizeof(mqtt.username));
}

String SettingsClass::getMQTTPassword() const {
    return mqtt.password;
}

void SettingsClass::setMQTTPassword(const char *password) {
    strlcpy(mqtt.password, password, sizeof(mqtt.password));
}

unsigned int SettingsClass::getMQTTDataInterval() const {
    return mqtt.dataInterval;
}

void SettingsClass::setMQTTDataInterval(unsigned int dataInterval) {
    mqtt.dataInterval = dataInterval;
}

unsigned int SettingsClass::getMQTTDiagnosticInterval() const {
    return mqtt.diagnosticInterval;
}

void SettingsClass::setMQTTDiagnosticInterval(unsigned int diagnosticInterval) {
    mqtt.diagnosticInterval = diagnosticInterval;
}

unsigned int SettingsClass::getMQTTDiscoveryInterval() const {
    return mqtt.discoveryInterval;
}

void SettingsClass::setMQTTDiscoveryInterval(unsigned int discoveryInterval) {
    mqtt.discoveryInterval = discoveryInterval;
}

unsigned int SettingsClass::getMQTTLocationInterval() const {
    return mqtt.locationInterval;
}

void SettingsClass::setMQTTLocationInterval(unsigned int locationInterval) {
    mqtt.locationInterval = locationInterval;
}

SettingsClass Settings;
