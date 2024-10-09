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
#include "ota.h"

#include <LittleFS.h>
#include <settings.h>

OTAClass::OTAClass() = default;

void OTAClass::begin(AsyncWebServer &server) {
    ElegantOTA.begin(&server);
    ElegantOTA.onStart(std::bind(&OTAClass::onOTAStart, this));
    ElegantOTA.onProgress(std::bind(&OTAClass::onOTAProgress, this, std::placeholders::_1, std::placeholders::_2));
    ElegantOTA.onEnd(std::bind(&OTAClass::onOTAEnd, this, std::placeholders::_1));
}

void OTAClass::setAutoReboot(bool autoReboot) {
    this->autoReboot = autoReboot;
}

bool OTAClass::isStarted() const {
    return otaStarted;
}

void OTAClass::onOTAStart() {
    Serial.println("OTA update started!");
    otaStarted = true;
}

void OTAClass::onOTAProgress(size_t current, size_t final) {
    if (millis() > otaProgressMillis) {
        otaProgressMillis = millis() + 1000;
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void OTAClass::onOTAEnd(bool success) {
    if (success) {
        Serial.println("OTA update finished successfully!");
        successCallback();
    } else {
        Serial.println("There was an error during OTA update!");
    }
    otaStarted = false;
}

void OTAClass::onSuccess(const std::function<void()> &callable) {
    successCallback = callable;
}

OTAClass OTA;
