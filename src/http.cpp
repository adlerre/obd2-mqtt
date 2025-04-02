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
#include "http.h"
#include <LittleFS.h>
#include <obd.h>

#if OTA_ENABLED
#include <ota.h>
#include <settings.h>
#endif

HTTPServer::HTTPServer(const int port): server(port) {
}

void HTTPServer::init(fs::FS &fs) {
    server.serveStatic("/", fs, "/public/").setDefaultFile("index.html");
    server.onNotFound([](AsyncWebServerRequest *request) {
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += request->url();
        message += "\nMethod: ";
        message += (request->method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += request->args();
        message += "\n";
        for (uint8_t i = 0; i < request->args(); i++) {
            message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
        }
        request->send(404, "text/plain", message);
    });

    server.on(
        "/api/reboot",
        HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (request->hasParam("reboot")) {
                request->send(200);
                Serial.println("Rebooting...");
                delay(2000);
                ESP.restart();
            }
            request->send(406);
        }
    );

    server.on("/api/ota", HTTP_GET, [](AsyncWebServerRequest *request) {
#if OTA_ENABLED
        request->send(200, "text/plain", "enabled");
#else
        request->send(501, "text/plain", "disabled");
#endif
    });

#if OTA_ENABLED
    OTA.begin(server);
    OTA.setAutoReboot(false);
    OTA.onStart([]() {
        Serial.println("Start update...");
    });
    OTA.onProgress([&](size_t current, size_t final) {
        if (millis() > otaProgressMillis) {
            otaProgressMillis = millis() + 1000;
            Serial.printf("...progress current: %u bytes, Final: %u bytes\n", current, final);
        }
    });
    OTA.onEnd([&fs](bool success) {
        if (success) {
            Serial.println("...write settings");
            Settings.writeSettings(fs);
            Serial.println("...done");
            Serial.println("...write states");
            OBD.writeStates(fs);
            Serial.println("...done");
        } else {
            Serial.println("...failed");
        }
    });
#endif
}

void HTTPServer::begin(fs::FS &fs) {
    this->init(fs);
    server.begin();
    Serial.println("HTTP server started");
}

void HTTPServer::end() {
    server.end();
    Serial.println("HTTP server stopped");
}

AsyncCallbackWebHandler &HTTPServer::on(const char *uri,
                                        const WebRequestMethodComposite method,
                                        const ArRequestHandlerFunction &onRequest,
                                        const ArUploadHandlerFunction &onUpload,
                                        const ArBodyHandlerFunction &onBody) {
    return server.on(uri, method, onRequest, onUpload, onBody);
}
