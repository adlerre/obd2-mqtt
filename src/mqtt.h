/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, in a file called gpl.txt or license.txt.
 * If not, write to the Free Software Foundation Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */
#pragma once

#include <PubSubClient.h>
#include <regex>
#include <string>
#include <MQTTWebSocketClient.h>
#include <MQTTWebSocketStreamClient.h>

#ifndef BUILD_GIT_BRANCH
#define BUILD_GIT_BRANCH ""
#endif
#ifndef BUILD_GIT_COMMIT_HASH
#define BUILD_GIT_COMMIT_HASH ""
#endif

#define LWT_TOPIC           "connection"
#define LWT_CONNECTED       "connected"
#define LWT_DISCONNECTED    "connection lost"

#define MQTT_CLIENT_ID      "obd2mqtt"

#define MQTT_CON_RETRIES    10

typedef enum {
    USE_MQTT = 0,
    USE_WS = 1
} mqttProtocol;

class MQTT {
    Client *client;
    PubSubClient mqtt;
    MQTTWebSocketClient *wsClient;
    MQTTWebSocketStreamClient *wsStreamClient;

    int numReconnects = -1;
    std::string maintopic = "obd2mqtt";
    std::string identifier;
    std::string identifierName;

    static std::string createNodeId(const std::string &topic);

    std::string createFieldTopic(const std::string &field) const;

public:
    /**
     * Constructor of MQTT Helper
     */
    MQTT();

    /**
     * Set client (Wi-Fi or TinyGSM)
     *
     * @param client the client
     */
    void setClient(Client *client);

    /**
     * Connect to broker
     *
     * @param clientId the client id
     * @param broker the broker hostname
     * @param port the broker port
     * @param username the username
     * @param password the password
     * @param protocol the protocol
     * @param conTimeout the connection timeout
     *
     * @return <code>true</code> on success
     */
    bool connect(const char *clientId, const char *broker, unsigned int port, const char *username,
                 const char *password, mqttProtocol protocol = USE_MQTT, int conTimeout = 20000);

    /**
     * Returns the defined main topic.
     *
     * @return the main topic
     */
    std::string getMainTopic();

    /**
     * Set main topic.
     *
     * @param topic the main topic to set
     */
    void setMainTopic(const std::string &topic);

    /**
     * Returns the defined identifier.
     *
     * @return the identifier
     */
    std::string getIdentifier();

    /**
     * Set the identifier.
     *
     * @param identifier the identifier
     */
    void setIdentifier(const std::string &identifier);

    /**
     * Returns the identifier name.
     *
     * @return the identifier name
     */
    std::string getIdentifierName();

    /**
     * Set the identifier name.
     *
     * @param identifierName the identifier name
     */
    void setIdentifierName(const std::string &identifierName);

    /**
     * Returns the number of reconnect attemps.
     *
     * @return how many reconnects done
     */
    int reconnectAttemps() const;

    /**
     * Connects to MQTT client.
     *
     * @return <code>true</code> if connected
     */
    bool connected();

    /**
     * Disconnects from MQTT client.
     */
    void disconnect();

    /**
     * Run the main loop from MQTT client.
     */
    void loop();

    /**
     * Public payload to broker.
     *
     * @param topic the topic
     * @param payload the payload
     * @param retained should retain
     * @param maxSize max size of payload length, is split if exceeded
     * @return <code>true</code> on success
     */
    bool publish(const std::string &topic, const std::string &payload, bool retained = true,
                 int maxSize = MQTT_MAX_PACKET_SIZE);

    /**
     * Send topic config.
     *
     * @param group the group
     * @param field the field name
     * @param name the name or description of field
     * @param icon the icon
     * @param unit the unit
     * @param deviceClass the device class
     * @param stateClass the state class
     * @param entityCategory the entity category
     * @param topicType the topic type e.g. sensor or other
     * @param sourceType the source type e.g. gps
     * @param allowOffline <code>true</code> if topic should not remove
     * @return <code>true</code> on success
     *
     * @see
     *   https://www.home-assistant.io/integrations/mqtt/#discovery-examples
     *   https://developers.home-assistant.io/docs/core/entity/sensor/
     *   https://www.home-assistant.io/integrations/device_tracker.mqtt/
     *   icons -> https://mdisearch.com
     */
    bool sendTopicConfig(const std::string &group, const std::string &field,
                         const std::string &name,
                         const std::string &icon, const std::string &unit, const std::string &deviceClass,
                         const std::string &stateClass, const std::string &entityCategory,
                         const std::string &topicType = "sensor", const std::string &sourceType = "",
                         bool allowOffline = false);

    /**
     * Send topic update payload.
     *
     * @param field the field name
     * @param payload the payload
     * @param isAttribJson <code>true</code> if json attribute
     */
    bool sendTopicUpdate(const std::string &field, const std::string &payload, bool isAttribJson = false);
};
