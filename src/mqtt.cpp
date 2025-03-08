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
#include <ArduinoJson.h>
#include <helper.h>
#include <MQTTWebSocketStreamClient.h>

std::string MQTT::createNodeId(const std::string &topic) {
    auto splitPos = topic.find_last_of('/');
    return stripChars((splitPos == std::string::npos) ? topic : topic.substr(splitPos + 1));
}

std::string MQTT::createFieldTopic(const std::string &field) const {
    return stripChars((!identifier.empty() ? identifier : maintopic) + "_" + field);
}

MQTT::MQTT(): client(nullptr), wsClient(nullptr), wsStreamClient(nullptr) {
    mqtt.setKeepAlive(60);
    mqtt.setBufferSize(MQTT_MAX_PACKET_SIZE);
}

void MQTT::setClient(Client *client) {
    this->client = client;
}

bool MQTT::connect(const char *clientId, const char *broker, const unsigned int port, const char *username,
                   const char *password, mqttProtocol protocol, int conTimeout) {
    if (protocol == USE_MQTT) {
        mqtt.setClient(*client);
        mqtt.setServer(broker, port);
    } else {
        wsClient = new MQTTWebSocketClient(*client, broker, port);
        wsStreamClient = new MQTTWebSocketStreamClient(*wsClient, "/");
        mqtt.setClient(*wsStreamClient);
    }

    int numFailed = 0;
    while (!mqtt.connected() && numFailed < MQTT_CON_RETRIES) {
        Serial.printf("The client %s connects to the MQTT broker...", clientId);
        std::string lwtTopic = maintopic + "/" + createFieldTopic(LWT_TOPIC);
        if (mqtt.connect(clientId, username, password, lwtTopic.c_str(), 0, false, LWT_DISCONNECTED, true)) {
            Serial.println("...connected.");
            ++this->numReconnects;
            return true;
        }

        Serial.printf("...failed with state %d\n", mqtt.state());
        delay(conTimeout / MQTT_CON_RETRIES);
        ++numFailed;
    }

    return false;
}

std::string MQTT::getMainTopic() {
    return maintopic;
}

void MQTT::setMainTopic(const std::string &topic) {
    maintopic = topic;
}

std::string MQTT::getIdentifier() {
    return identifier;
}

void MQTT::setIdentifier(const std::string &identifier) {
    this->identifier = identifier;
}

std::string MQTT::getIdentifierName() {
    return identifierName;
}

void MQTT::setIdentifierName(const std::string &identifierName) {
    this->identifierName = identifierName;
}

int MQTT::reconnectAttemps() const {
    return this->numReconnects;
}

bool MQTT::connected() {
    return mqtt.connected();
}

void MQTT::disconnect() {
    mqtt.disconnect();
}

void MQTT::loop() {
    mqtt.loop();
}

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
        String version = String(BUILD_GIT_BRANCH);
        if (!version.startsWith("v")) {
            version += " (" + String(BUILD_GIT_COMMIT_HASH) + ")";
        }
        config["device"]["sw_version"] = version;
    }

    serializeJson(config, payload);

    return publish(topicFull, payload, true, MQTT_MAX_PACKET_SIZE / 2);
}

bool MQTT::sendTopicUpdate(const std::string &field, const std::string &payload, bool isAttribJson) {
    std::string node_id = createNodeId(maintopic);
    std::string topicFull = node_id + "/" + createFieldTopic(field);

    if (isAttribJson) {
        topicFull += "/attributes";
    }

    return publish(topicFull, payload, true, MQTT_MAX_PACKET_SIZE / 2);
}
