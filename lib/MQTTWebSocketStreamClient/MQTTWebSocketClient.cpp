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

#include "MQTTWebSocketClient.h"
#include "b64.h"

MQTTWebSocketClient::MQTTWebSocketClient(Client &client, const char *host, uint16_t port): WebSocketClient(
    client, host, port) {
    connectionKeepAlive();
}

int MQTTWebSocketClient::connect(const IPAddress &ip, uint16_t port) {
    connectionKeepAlive();
    return WebSocketClient::connect(ip, port);
}

int MQTTWebSocketClient::begin(const char *aPath, const char *protocol) {
    // start the GET request
    beginRequest();
    int status = get(aPath);

    if (status == 0) {
        uint8_t randomKey[16];
        char base64RandomKey[25];

        // create a random key for the connection upgrade
        for (int i = 0; i < static_cast<int>(sizeof(randomKey)); i++) {
            randomKey[i] = random(0x01, 0xff);
        }
        memset(base64RandomKey, 0x00, sizeof(base64RandomKey));
        b64_encode(randomKey, sizeof(randomKey), reinterpret_cast<unsigned char *>(base64RandomKey),
                   sizeof(base64RandomKey));

        // start the connection upgrade sequence
        sendHeader("Upgrade", "websocket");
        sendHeader("Connection", "Upgrade");
        sendHeader("Sec-WebSocket-Key", base64RandomKey);
        sendHeader("Sec-WebSocket-Version", "13");
        if (protocol) {
            // is required by Mosquitto broker
            sendHeader("Sec-WebSocket-Protocol", protocol);
        }
        endRequest();

        status = responseStatusCode();
        if (status > 0) {
            skipResponseHeaders();
        }
    }

    // iRxSize = 0;

    // status code of 101 means success
    return (status == 101) ? 0 : status;
}

int MQTTWebSocketClient::begin(const String &aPath, const char *protocol) {
    return begin(aPath.c_str(), protocol);
}

bool MQTTWebSocketClient::flush(unsigned int maxWaitMs) {
    WebSocketClient::flush();
    return true;
}

bool MQTTWebSocketClient::stop(unsigned int maxWaitMs) {
    WebSocketClient::stop();
    return true;
}
