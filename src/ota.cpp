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

#include <StreamString.h>

OTAClass::OTAClass() = default;

void OTAClass::begin(AsyncWebServer &server) {
    server.on("/api/ota/start", HTTP_GET, [&](AsyncWebServerRequest *request) {
        OTA_Mode mode = OTA_MODE_FIRMWARE;

        if (request->hasParam("mode")) {
            String argValue = request->getParam("mode")->value();
            if (argValue == "fs") {
                mode = OTA_MODE_FILESYSTEM;
            } else {
                mode = OTA_MODE_FIRMWARE;
            }
        }

        if (preUpdateCallback != nullptr) preUpdateCallback();

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, mode == OTA_MODE_FILESYSTEM ? U_SPIFFS : U_FLASH)) {
            StreamString str;
            Update.printError(str);
            updateError = str.c_str();
            updateError.concat("\n");
        }

        return request->send(Update.hasError() ? 400 : 200, "text/plain",
                             Update.hasError() ? updateError.c_str() : "OK");
    });

    server.on("/api/ota/upload", HTTP_POST, [&](AsyncWebServerRequest *request) {
                  if (postUpdateCallback != nullptr) postUpdateCallback(!Update.hasError());

                  AsyncWebServerResponse *response = request->beginResponse(
                      Update.hasError() ? 400 : 200, "text/plain",
                      Update.hasError() ? updateError.c_str() : "OK");
                  response->addHeader("Connection", "close");
                  response->addHeader("Access-Control-Allow-Origin", "*");
                  request->send(response);
              }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len,
                     bool final) {
                  if (!index) {
                      currentProgressSize = 0;
                  }

                  if (len) {
                      if (Update.write(data, len) != len) {
                          return request->send(400, "text/plain", "Failed to write chunked data to free space");
                      }
                      currentProgressSize += len;
                      if (progressUpdateCallback != nullptr)
                          progressUpdateCallback(currentProgressSize, request->contentLength());
                  }

                  if (final) {
                      if (!Update.end(true)) {
                          StreamString str;
                          Update.printError(str);
                          updateError = str.c_str();
                          updateError.concat("\n");
                      }
                  }
              });
}

void OTAClass::setAutoReboot(bool autoReboot) {
    this->autoReboot = autoReboot;
}

void OTAClass::onStart(const std::function<void()> &callable) {
    preUpdateCallback = callable;
}

void OTAClass::onProgress(const std::function<void(int, int)> &callable) {
    progressUpdateCallback = callable;
}

void OTAClass::onEnd(const std::function<void(bool)> &callable) {
    postUpdateCallback = callable;
}

OTAClass OTA;
