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

#include "MQTTWebSocketClient.h"
#include <Client.h>

class MQTTWebSocketStreamClient : public Client {
    MQTTWebSocketClient *webSocketClient;
    const char *path;
    const char *protocol;

public:
    MQTTWebSocketStreamClient(MQTTWebSocketClient &webSocketClient, const char *path, const char *protocol = "mqtt");

    int connect(IPAddress ip, uint16_t port) override;

    int connect(const char *host, uint16_t port) override;

    size_t write(uint8_t b) override;

    size_t write(const uint8_t *buf, size_t size) override;

    int available() override;

    int read() override;

    int read(uint8_t *buf, size_t size) override;

    int peek() override;

    void flush() override;

    void stop() override;

    uint8_t connected() override;

    explicit operator bool() override;
};
