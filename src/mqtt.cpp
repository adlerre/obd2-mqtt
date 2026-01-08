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

#include <utility>

MQTTSubscription::MQTTSubscription(const std::string &field, const std::string &topic) {
    this->field = field;
    this->topic = topic;
}

std::string MQTTSubscription::getField() const {
    return field;
}

std::string MQTTSubscription::getTopic() const {
    return topic;
}

void MQTTSubscription::setCallback(std::function<void(const char *)> callback) {
    this->callback = callback;
}

void MQTTSubscription::fireCallback(const char *msg) const {
    if (this->callback != nullptr) {
        this->callback(msg);
    }
}

void MQTT::callback(const char *topic, const byte *payload, const unsigned int length) {
    char msg[length + 1];
    for (int i = 0; i < length; i++) {
        msg[i] = static_cast<char>(payload[i]);
    }
    msg[length] = '\0';

    for (auto &sub: subscriptions) {
        if (sub->getTopic() == topic) {
            sub->fireCallback(msg);
        }
    }
}

std::string MQTT::createNodeId(const std::string &topic) {
    auto splitPos = topic.find_last_of('/');
    return stripChars((splitPos == std::string::npos) ? topic : topic.substr(splitPos + 1));
}

std::string MQTT::createFieldTopic(const std::string &field) const {
    return stripChars((!identifier.empty() ? identifier : maintopic) + "_" + field);
}

bool MQTT::hasSubscription(const std::string &field) const {
    for (auto &sub: subscriptions) {
        if (sub->getField() == field) {
            return true;
        }
    }
    return false;
}

void MQTT::addSubscription(const std::string &field, const std::string &topic) {
    if (!hasSubscription(field)) {
        subscriptions.push_back(new MQTTSubscription(field, topic));
        mqtt.subscribe(topic.c_str());
    }
}

MQTT::MQTT() : client(nullptr), wsClient(nullptr), wsStreamClient(nullptr) {
    mqtt.setKeepAlive(MQTT_KEEPALIVE);
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

    mqtt.setCallback([&](char *topic, byte *payload, unsigned int length) {
        this->callback(topic, payload, length);
    });

    int numFailed = 0;
    while (!mqtt.connected() && numFailed < MQTT_CON_RETRIES) {
        Serial.printf("Client %s connects to the MQTT broker...", clientId);
        std::string lwtTopic = maintopic + "/" + createFieldTopic(LWT_TOPIC);
        if (mqtt.connect(clientId, username, password, lwtTopic.c_str(), 0, false, LWT_DISCONNECTED, true)) {
            Serial.println("...connected.");
            ++this->numReconnects;

            for (auto &sub: subscriptions) {
                mqtt.subscribe(sub->getTopic().c_str());
            }

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

bool MQTT::sendTopicConfig(const std::string &field,
                           const std::string &name,
                           const std::string &icon, const std::string &unit, const std::string &deviceClass,
                           const std::string &stateClass, const std::string &entityCategory,
                           const std::string &topicType, const std::string &sourceType, bool allowOffline,
                           const std::string &valueTemplate) {
    // Abbreviations - https://github.com/home-assistant/core/blob/dev/homeassistant/components/mqtt/abbreviations.py
    std::string payload;

    std::string configTopic = createFieldTopic(field);
    std::string node_id = createNodeId(maintopic);
    std::string topicFull = "homeassistant/" + topicType + "/" + node_id + "/" + configTopic + "/config";

    JsonDocument config;

    config["~"] = maintopic;
    config["uniq_id"] = configTopic;
    config["obj_id"] = configTopic;
    config["name"] = name;

    if (!icon.empty()) {
        config["icon"] = "mdi:" + icon;
    }

    if (topicType != TT_BUTTON && topicType != TT_D_TRACKER) {
        config["stat_t"] = "~/" + configTopic;
    }

    if (topicType == TT_B_SENSOR) {
        config["val_tpl"] = "{{ 'OFF' if 'off' in value else 'ON'}}";
    } else if (topicType == TT_BUTTON) {
        std::string cmdTopic = node_id + "/" + configTopic + "/ctrl";
        config["cmd_t"] = cmdTopic;
        addSubscription(field, cmdTopic);
    } else if (topicType == TT_D_TRACKER) {
        config["json_attr_t"] = "~/" + configTopic + "/attributes";
    }

    if (!sourceType.empty()) {
        config["src_type"] = sourceType;
    }

    if (!unit.empty()) {
        config["unit_of_meas"] = unit;
    }

    if (!deviceClass.empty()) {
        config["dev_cla"] = deviceClass;
    }

    if (!stateClass.empty()) {
        config["stat_cla"] = stateClass;
    }

    if (!entityCategory.empty()) {
        config["ent_cat"] = entityCategory;
    }

    if (!allowOffline) {
        config["avty_t"] = "~/" + createFieldTopic(LWT_TOPIC);
        config["pl_avail"] = LWT_CONNECTED;
        config["pl_not_avail"] = LWT_DISCONNECTED;
    }

    if (!valueTemplate.empty()) {
        config["val_tpl"] = valueTemplate;
    }

    config["dev"]["ids"].add(!identifier.empty() ? identifier : maintopic);
    config["dev"]["name"] = !identifierName.empty()
                                ? identifierName
                                : !identifier.empty()
                                      ? identifier
                                      : maintopic;;
    config["dev"]["mdl"] = "OBD2 to MQTT";
    config["dev"]["mf"] = "René Adler";
    config["dev"]["sw"] = getVersion();

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

void MQTT::subscribe(const std::string &field, const std::function<void(const char *)> &callback) {
    for (auto &sub: subscriptions) {
        if (sub->getField() == field) {
            sub->setCallback(callback);
        }
    }
}
