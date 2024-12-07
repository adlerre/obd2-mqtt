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
#include <ESPAsyncWebServer.h>

#ifndef OTA_ENABLED
#define OTA_ENABLED false
#endif

class HTTPServer {
    AsyncWebServer server;
    unsigned long otaProgressMillis = 0;

    void init(fs::FS &fs);

public:
    HTTPServer(int port = 80);

    void begin(fs::FS &fs);

    void end();

    AsyncCallbackWebHandler &on(const char *uri,
                                WebRequestMethodComposite method,
                                const ArRequestHandlerFunction &onRequest,
                                const ArUploadHandlerFunction &onUpload = nullptr,
                                const ArBodyHandlerFunction &onBody = nullptr);
};
