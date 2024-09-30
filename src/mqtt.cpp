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
#include "mqtt.h"

std::string MQTT::createNodeId(const std::string &topic) {
    auto splitPos = topic.find_last_of('/');
    return stripChars((splitPos == std::string::npos) ? topic : topic.substr(splitPos + 1));
}

std::string MQTT::createFieldTopic(const std::string &field) const {
    return stripChars((!identifier.empty() ? identifier : maintopic) + "_" + field);
}

std::string MQTT::stripChars(const std::string &str) {
    std::regex reg("[^a-zA-Z0-9_-]");
    return std::regex_replace(str, reg, "");
}

/**
 * Constructor of MQTT Helper
 *
 * @param client the PubSubClient
 * @param broker the broker hostname
 * @param port the broker port, defaults to 1883
 */
MQTT::MQTT(const PubSubClient &client, const char *broker, const int port) {
    mqtt = client;
    mqtt.setKeepAlive(60);
    mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);
    mqtt.setServer(broker, port);
}

/**
* Connect to broker
*
* @param clientId the client id
* @param username the username
* @param password the password
*/
void MQTT::connect(const char *clientId, const char *username, const char *password) {
    while (!mqtt.connected()) {
        Serial.printf("The client %s connects to the MQTT broker...", clientId);
        std::string lwtTopic = maintopic + "/" + createFieldTopic(LWT_TOPIC);
        if (mqtt.connect(clientId, username, password, lwtTopic.c_str(), 0, false, LWT_DISCONNECTED, true)) {
            Serial.println("...connected.");
            ++numReconnects;
            return;
        }

        Serial.printf("...failed with state %d\n", mqtt.state());
        delay(2000);
    }
}

/**
* Returns the defined main topic.
*
* @return the main topic
*/
std::string MQTT::getMainTopic() {
    return maintopic;
}

/**
* Set main topic.
*
* @param topic the main topic to set
*/
void MQTT::setMainTopic(const std::string &topic) {
    maintopic = topic;
}

/**
* Returns the defined identifier.
*
* @return the identifier
*/
std::string MQTT::getIdentifier() {
    return identifier;
}

/**
* Set the identifier.
*
* @param identifier the identifier
*/
void MQTT::setIdentifier(const std::string &identifier) {
    this->identifier = identifier;
}

/**
* Returns the identifier name.
*
* @return the identifier name
*/
std::string MQTT::getIdentifierName() {
    return identifierName;
}

/**
* Set the identifier name.
*
* @param identifierName the identifier name
*/
void MQTT::setIdentifierName(const std::string &identifierName) {
    this->identifierName = identifierName;
}

/**
* Returns the number of reconnect attemps.
*
* @return how many reconnects done
*/
int MQTT::reconnectAttemps() const {
    return numReconnects;
}

/**
* Connects to MQTT client.
*
* @return <code>true</code> if connected
*/
bool MQTT::connected() {
    return mqtt.connected();
}

/**
* Disconnects from MQTT client.
*/
void MQTT::disconnect() {
    mqtt.disconnect();
}

/**
* Run the main loop from MQTT client.
*/
void MQTT::loop() {
    mqtt.loop();
}

/**
* Public payload to broker.
*
* @param topic the topic
* @param payload the payload
* @param retained should retain
* @param maxSize max size of payload length, is split if exceeded
* @return <code>true</code> on success
*/
bool MQTT::publish(const std::string &topic, const std::string &payload, bool retained, int maxSize) {
    if (!mqtt.connected()) {
        return false;
    }

    if (payload.length() > maxSize) {
        if (mqtt.beginPublish(topic.c_str(), payload.length(), retained)) {
            size_t pos = 0;
            while (pos < payload.length()) {
                size_t length = maxSize;
                if (length > payload.length() - pos) {
                    length = payload.length() - pos;
                }

                std::string tmp = payload.substr(pos, length);

                unsigned char buffer[tmp.length()];
                std::copy(tmp.begin(), tmp.end(), buffer);

                size_t written = mqtt.write(buffer, length);
                if (written != length) {
                    return false;
                }
                pos += length;
            }
            return mqtt.endPublish();
        }

        return false;
    }

    return mqtt.publish(topic.c_str(), payload.c_str(), retained);
}

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
* @param allowOffline <code>true</code> if topic should not removed
* @return <code>true</code> on success
*
* @see
*   https://www.home-assistant.io/integrations/mqtt/#discovery-examples
*   https://developers.home-assistant.io/docs/core/entity/sensor/
*   https://www.home-assistant.io/integrations/device_tracker.mqtt/
*   icons -> https://mdisearch.com
*/
bool MQTT::sendTopicConfig(const std::string &group, const std::string &field,
                           const std::string &name,
                           const std::string &icon, const std::string &unit, const std::string &deviceClass,
                           const std::string &stateClass, const std::string &entityCategory,
                           const std::string &topicType, const std::string &sourceType, bool allowOffline) {
    std::string payload;

    std::string configTopic = createFieldTopic(field);
    std::string node_id = createNodeId(maintopic);
    std::string topicFull = "homeassistant/" + topicType + "/" + node_id + "/" + configTopic + "/config";

    JsonDocument config;

    config["~"] = maintopic;
    config["unique_id"] = configTopic;
    config["object_id"] = configTopic;
    config["name"] = name;

    if (!icon.empty()) {
        config["icon"] = "mdi:" + icon;
    }

    if (topicType != "device_tracker") {
        if (!group.empty()) {
            config["state_topic"] = "~/" + group + "/" + configTopic;
        } else {
            config["state_topic"] = "~/" + configTopic;
        }
    }

    if (topicType == "binary_sensor") {
        config["value_template"] = "{{ 'OFF' if 'off' in value else 'ON'}}";
    } else if (topicType == "device_tracker") {
        if (!group.empty()) {
            config["json_attributes_topic"] = "~/" + group + "/" + configTopic + "/attributes";
        } else {
            config["json_attributes_topic"] = "~/" + configTopic + "/attributes";
        }
    }

    if (!sourceType.empty()) {
        config["source_type"] = sourceType;
    }

    if (!unit.empty()) {
        config["unit_of_measurement"] = unit;
    }

    if (!deviceClass.empty()) {
        config["device_class"] = deviceClass;
    }

    if (!stateClass.empty()) {
        config["state_class"] = stateClass;
    }

    if (!entityCategory.empty()) {
        config["entity_category"] = entityCategory;
    }

    if (!allowOffline) {
        config["availability_topic"] = "~/" + createFieldTopic(LWT_TOPIC);
        config["payload_available"] = LWT_CONNECTED;
        config["payload_not_available"] = LWT_DISCONNECTED;
    }

    config["device"]["identifiers"].add(!identifier.empty() ? identifier : maintopic);
    config["device"]["name"] = !identifierName.empty()
                                   ? identifierName
                                   : !identifier.empty()
                                         ? identifier
                                         : maintopic;;
    config["device"]["model"] = "OBD2 to MQTT";
    config["device"]["manufacturer"] = "René Adler";

    if (BUILD_GIT_BRANCH != "" && BUILD_GIT_COMMIT_HASH != "") {
        config["device"]["sw_version"] = String(BUILD_GIT_BRANCH) + " (" + String(BUILD_GIT_COMMIT_HASH) + ")";
    }

    serializeJson(config, payload);

    return publish(topicFull, payload, true, MQTT_MAX_PACKET_SIZE / 2);
}

/**
* Send topic update payload.
*
* @param field the field name
* @param payload the payload
* @param isAttribJson <code>true</code> if json attribute
*/
bool MQTT::sendTopicUpdate(const std::string &field, const std::string &payload, bool isAttribJson) {
    std::string node_id = createNodeId(maintopic);
    std::string topicFull = node_id + "/" + createFieldTopic(field);

    if (isAttribJson) {
        topicFull += "/attributes";
    }

    return publish(topicFull, payload, true, MQTT_MAX_PACKET_SIZE / 2);
}
