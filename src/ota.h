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
#include <Update.h>

enum OTA_Mode {
    OTA_MODE_FIRMWARE = 0,
    OTA_MODE_FILESYSTEM = 1
};

class OTAClass {
    boolean autoReboot = false;

    int currentProgressSize;
    String updateError;

    std::function<void()> preUpdateCallback = nullptr;
    std::function<void(int, int)> progressUpdateCallback = nullptr;
    std::function<void(bool)> postUpdateCallback = nullptr;

public:
    OTAClass();

    void begin(AsyncWebServer &server);

    void setAutoReboot(bool autoReboot);

    void onStart(const std::function<void()> &callable);

    void onProgress(const std::function<void(int, int)> &callable);

    void onEnd(const std::function<void(bool)> &callable);
};

extern OTAClass OTA;
