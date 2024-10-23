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

#include "MQTTWebSocketStreamClient.h"

MQTTWebSocketStreamClient::MQTTWebSocketStreamClient(MQTTWebSocketClient &webSocketClient, const char *path,
                                                     const char *protocol) {
    this->webSocketClient = &webSocketClient;
    this->path = path;
    this->protocol = protocol;
}

int MQTTWebSocketStreamClient::connect(const IPAddress ip, uint16_t port) {
    webSocketClient->begin(path, this->protocol);
    return 1;
}

int MQTTWebSocketStreamClient::connect(const char *host, uint16_t port) {
    webSocketClient->begin(path, this->protocol);
    return 1;
}

size_t MQTTWebSocketStreamClient::write(uint8_t b) {
    if (!connected())
        return -1;
    return write(&b, 1);
}

size_t MQTTWebSocketStreamClient::write(const uint8_t *buf, size_t size) {
    if (!connected())
        return -1;
    webSocketClient->beginMessage(TYPE_BINARY);
    webSocketClient->write(buf, size);
    webSocketClient->endMessage();
    return size;
}

int MQTTWebSocketStreamClient::available() {
    if (!connected())
        return 0;
    if (webSocketClient->available() == 0)
        webSocketClient->parseMessage();
    return webSocketClient->available();
}

int MQTTWebSocketStreamClient::read() {
    if (!connected() || !available())
        return -1;
    return webSocketClient->read();
}

int MQTTWebSocketStreamClient::read(uint8_t *buf, size_t size) {
    if (!connected() || !available())
        return -1;
    return webSocketClient->read(buf, size);
}

int MQTTWebSocketStreamClient::peek() {
    if (!connected() || !available())
        return -1;
    return webSocketClient->peek();
}

void MQTTWebSocketStreamClient::flush() {
    if (!connected())
        return;
    webSocketClient->flush();
}

void MQTTWebSocketStreamClient::stop() {
    if (!connected())
        return;
    webSocketClient->stop();
}

uint8_t MQTTWebSocketStreamClient::connected() {
    return webSocketClient->connected();
}

MQTTWebSocketStreamClient::operator bool() {
    return webSocketClient != nullptr;
}
