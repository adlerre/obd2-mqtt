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
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

class OTAClass {
    boolean autoReboot = false;
    boolean otaStarted = false;
    unsigned long otaProgressMillis = 0;
    std::function<void()> successCallback = nullptr;

    void onOTAStart();

    void onOTAProgress(size_t current, size_t final);

    void onOTAEnd(bool success);

public:
    OTAClass();

    void begin(AsyncWebServer &server);

    void setAutoReboot(bool autoReboot);

    bool isStarted() const;

    void onSuccess(const std::function<void()> &callable);
};

extern OTAClass OTA;
