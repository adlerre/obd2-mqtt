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

#include "OBDState.h"

#include <ExprParser.h>

void *OBDState::operator new(const size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

void OBDState::operator delete(void *ptr) {
    heap_caps_free(ptr);
}

OBDState::OBDState(const obd::OBDStateType type, const char *name, const char *description, const char *icon,
                   const char *unit, const char *deviceClass, const bool measurement, const bool diagnostic) {
    this->type = type;
    strlcpy(this->name, name, sizeof(this->name));
    strlcpy(this->description, description, sizeof(this->description));
    strlcpy(this->icon, icon, sizeof(this->icon));
    strlcpy(this->unit, unit, sizeof(this->unit));
    strlcpy(this->deviceClass, deviceClass, sizeof(this->deviceClass));
    this->measurement = measurement;
    this->diagnostic = diagnostic;
    this->updateInterval = 100;
}

obd::OBDStateType OBDState::getType() const {
    return this->type;
}

const char *OBDState::valueType() const {
    return "base";
}

void OBDState::setELM327(ELM327 *elm327) {
    this->elm327 = elm327;
}

const char *OBDState::getName() const {
    return this->name;
}

const char *OBDState::getDescription() const {
    return this->description;
}

const char *OBDState::getIcon() const {
    return this->icon;
}

const char *OBDState::getUnit() const {
    return this->unit;
}

const char *OBDState::getDeviceClass() const {
    return this->deviceClass;
}

bool OBDState::isMeasurement() const {
    return this->measurement;
}

bool OBDState::isDiagnostic() const {
    return this->diagnostic;
}

void OBDState::setCalcExpression(const char *expression) {
    this->type = obd::CALC;
    strlcpy(this->calcExpression, expression, sizeof(this->calcExpression));
}

bool OBDState::hasCalcExpression() const {
    return strlen(this->calcExpression) != 0;
}

uint32_t OBDState::supportedPIDs(const uint8_t &service, const uint16_t &pid) const {
    const uint8_t pidInterval = (pid / PID_INTERVAL_OFFSET) * PID_INTERVAL_OFFSET;
    return static_cast<uint32_t>(elm327->processPID(service, pidInterval, 1, 4));
}

bool OBDState::isPIDSupported(const uint8_t &service, const uint16_t &pid) const {
    if (service >= 0x01 && service <= 0x0A) {
        const uint32_t response = supportedPIDs(service, pid);
        if (elm327->nb_rx_state == ELM_SUCCESS) {
            return ((response >> (32 - pid)) & 0x1);
        }

        return false;
    }

    return true;
}

void OBDState::setPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                              const uint8_t &numResponses,
                              const uint8_t &numExpectedBytes, const double &scaleFactor, const float &bias) {
    this->type = obd::READ;
    this->service = service;
    this->pid = pid;
    this->header = header;
    this->numResponses = numResponses;
    this->numExpectedBytes = numExpectedBytes;
    this->scaleFactor = scaleFactor;
    this->bias = bias;
    this->updateInterval = 100;
}

void OBDState::setPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                              const uint8_t &numResponses,
                              const uint8_t &numExpectedBytes, const char *scaleFactorExpression, const float &bias) {
    double scaleFactor = 1;
    if (scaleFactorExpression != nullptr && strlen(scaleFactorExpression) > 0) {
        ExprParser parser;
        scaleFactor = parser.evalExp(scaleFactorExpression);
        if (strlen(parser.errormsg) > 0) {
            Serial.print("Error: ");
            Serial.print(this->name);
            Serial.print(" ");
            Serial.println(scaleFactorExpression);
            Serial.print(" ");
            Serial.println(parser.errormsg);
        }
    }
    strlcpy(this->scaleFactorExpression, scaleFactorExpression, sizeof(this->scaleFactorExpression));

    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
}

OBDState *OBDState::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                    const uint8_t &numResponses,
                                    const uint8_t &numExpectedBytes, const double &scaleFactor, const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDState *OBDState::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                    const uint8_t &numResponses,
                                    const uint8_t &numExpectedBytes, const char *scaleFactorExpression,
                                    const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

bool OBDState::isInit() const {
    return this->init;
}

void OBDState::setCheckPidSupport(const bool enable) {
    this->checkPidSupport = enable;
}

bool OBDState::isSupported() const {
    return this->supported;
}

bool OBDState::isEnabled() const {
    return this->enabled;
}

void OBDState::setEnabled(bool enable) {
    this->enabled = enable;
}

OBDState *OBDState::withEnabled(const bool enable) {
    this->setEnabled(enable);
    return this;
}

bool OBDState::isVisible() const {
    return this->visible;
}

void OBDState::setVisible(const bool visible) {
    this->visible = visible;
}

OBDState *OBDState::withVisible(const bool visible) {
    this->setVisible(visible);
    return this;
}

bool OBDState::isProcessing() const {
    return this->processing;
}

void OBDState::setUpdateInterval(const long interval) {
    this->updateInterval = interval;
}

long OBDState::getUpdateInterval() const {
    return this->updateInterval;
}

OBDState *OBDState::withUpdateInterval(const long interval) {
    this->updateInterval = interval;
    return this;
}

void OBDState::setPreviousUpdate(const long timestamp) {
    this->previousUpdate = timestamp;
}

long OBDState::getPreviousUpdate() const {
    return this->previousUpdate;
}

void OBDState::setLastUpdate(const long timestamp) {
    this->lastUpdate = timestamp;
}

long OBDState::getLastUpdate() const {
    return this->lastUpdate;
}

void OBDState::readValue() {
}

void OBDState::calcValue(const std::function<double(const char *)> &func,
                         const std::map<const char *, const std::function<double(double)>> &funcs) {
}

void OBDState::toJSON(JsonDocument &doc) {
    doc["type"] = this->getType();
    doc["valueType"] = this->valueType();
    doc["enabled"] = this->isEnabled();
    doc["visible"] = this->isVisible();
    doc["interval"] = this->getUpdateInterval();

    doc["name"] = this->getName();
    doc["description"] = this->getDescription();
    doc["icon"] = this->getIcon();
    doc["unit"] = this->getUnit();
    doc["deviceClass"] = this->getDeviceClass();
    doc["measurement"] = this->isMeasurement();
    doc["diagnostic"] = this->isDiagnostic();

    if (this->type == obd::CALC && strlen(this->calcExpression) != 0) {
        doc["expr"] = this->calcExpression;
    }
}

template<typename T>
TypedOBDState<T>::TypedOBDState(obd::OBDStateType type, const char *name, const char *description,
                                const char *icon, const char *unit, const char *deviceClass,
                                const bool measurement, const bool diagnostic): OBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
}

template<typename T>
const char *TypedOBDState<T>::valueType() const {
    return "generic";
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                                    const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                                    const double &scaleFactor, const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                                    const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                                    const char *scaleFactorExpression,
                                                    const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactorExpression, bias);
    return this;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withEnabled(const bool enable) {
    this->setEnabled(enable);
    return this;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withVisible(const bool visible) {
    this->setVisible(visible);
    return this;
}

template<typename T>
T TypedOBDState<T>::getOldValue() {
    return this->oldValue;
}

template<typename T>
void TypedOBDState<T>::setOldValue(T value) {
    this->oldValue = value;
}

template<typename T>
T TypedOBDState<T>::getValue() {
    return this->value;
}

template<typename T>
void TypedOBDState<T>::setValue(T value) {
    this->value = value;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withUpdateInterval(long interval) {
    this->setUpdateInterval(interval);
    return this;
}

template<typename T>
void TypedOBDState<T>::setReadFuncName(const char *funcName) {
    strlcpy(this->readFunctionName, funcName, sizeof(this->readFunctionName));
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withReadFuncName(const char *funcName) {
    this->setReadFuncName(funcName);
    return this;
}

template<typename T>
void TypedOBDState<T>::setReadFunc(const std::function<T()> &func) {
    this->type = obd::READ;
    this->readFunction = func;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withReadFunc(const std::function<T()> &func) {
    this->setReadFunc(func);
    return this;
}

template<typename T>
void TypedOBDState<T>::readValue() {
    if (elm327 != nullptr && elm327->elm_port && this->type == obd::READ) {
        if (!this->init && this->readFunction == nullptr) {
            this->supported = this->checkPidSupport && isPIDSupported(this->service, this->pid) || true;
            this->init = this->checkPidSupport && elm327->nb_rx_state == ELM_SUCCESS || true;
        } else if (this->readFunction != nullptr) {
            this->init = true;
        }

        if (this->init && this->supported) {
            if (!this->processing) {
                if (this->header > 0 && !this->setHeader) {
                    this->setHeader = false;
                    char command[20] = {'\0'};
                    char h[10] = {'\0'};
                    snprintf(h, sizeof(h), "%X", header);
                    snprintf(command, sizeof(command), SET_HEADER, h);
                    if (elm327->sendCommand_Blocking(command) == ELM_SUCCESS) {
                        if (strstr(elm327->payload, RESPONSE_OK) != nullptr) {
                            this->setHeader = true;
                        }
                    }
                }

                this->oldValue = this->value;
                this->previousUpdate = this->lastUpdate;
                this->processing = true;
            }

            T value = static_cast<T>(this->readFunction != nullptr
                                         ? this->readFunction()
                                         : elm327->processPID(this->service, this->pid, this->numResponses,
                                                              this->numExpectedBytes,
                                                              this->scaleFactor, this->bias));

            if (elm327->nb_rx_state == ELM_SUCCESS) {
                this->value = value;

                if (this->postProcessFunction != nullptr) {
                    this->postProcessFunction(this);
                }

                this->lastUpdate = millis();
                this->processing = false;
                this->updateStatus = elm327->nb_rx_state;
            } else if (elm327->nb_rx_state == ELM_NO_DATA) {
                this->value = 0;
                this->lastUpdate = millis();
                this->processing = false;
                this->updateStatus = elm327->nb_rx_state;
            } else if (elm327->nb_rx_state != ELM_GETTING_MSG) {
                this->processing = false;
                this->updateStatus = elm327->nb_rx_state;
            }

            if (this->header > 0 && this->setHeader && !this->processing) {
                if (elm327->sendCommand_Blocking(SET_ALL_TO_DEFAULTS) == ELM_SUCCESS) {
                    if (strstr(elm327->payload, RESPONSE_OK) != nullptr) {
                        this->setHeader = false;
                    }
                }
            }
        }
    }
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withCalcExpression(const char *expression) {
    this->setCalcExpression(expression);
    return this;
}

template<typename T>
void TypedOBDState<T>::calcValue(const std::function<double(const char *)> &func,
                                 const std::map<const char *, const std::function<double(double)>> &funcs) {
    if (this->type == obd::CALC && strlen(this->calcExpression) != 0) {
        if (!this->processing) {
            this->oldValue = this->value;
            this->previousUpdate = this->lastUpdate;
            this->processing = true;
        }

        ExprParser parser;
        parser.setCustomFunctions(funcs);
        parser.setVariableResolveFunction(func);
        this->value = static_cast<T>(parser.evalExp(const_cast<char *>(this->calcExpression)));
        if (strlen(parser.errormsg) > 0) {
            Serial.println();
            Serial.print(this->name);
            Serial.print(" (");
            Serial.print(this->calcExpression);
            Serial.print(") : ");
            Serial.println(parser.errormsg);
        }
        this->lastUpdate = millis();
        this->processing = false;
    }
}

template<typename T>
void TypedOBDState<T>::setPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) {
    this->postProcessFunction = postProcessFunction;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<
    T>::withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) {
    this->postProcessFunction = postProcessFunction;
    return this;
}

template<typename T>
void TypedOBDState<T>::setValueFormat(const char *format) {
    strlcpy(this->valueFormat, format, sizeof(this->valueFormat));
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withValueFormat(const char *format) {
    this->setValueFormat(format);
    return this;
}

template<typename T>
void TypedOBDState<T>::setValueFormatExpression(const char *expression) {
    strlcpy(this->valueFormatExpression, expression, sizeof(this->valueFormatExpression));
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withValueFormatExpression(const char *expression) {
    this->setValueFormatExpression(expression);
    return this;
}

template<typename T>
void TypedOBDState<T>::setValueFormatFuncName(const char *funcName) {
    strlcpy(this->valueFormatFunctionName, funcName, sizeof(this->valueFormatFunctionName));
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withValueFormatFuncName(const char *funcName) {
    this->setValueFormatFuncName(funcName);
    return this;
}

template<typename T>
void TypedOBDState<T>::setValueFormatFunc(const std::function<char *(T)> &valueFormatFunction) {
    this->valueFormatFunction = valueFormatFunction;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withValueFormatFunc(const std::function<char *(T)> &valueFormatFunction) {
    this->setValueFormatFunc(valueFormatFunction);
    return this;
}

template<typename T>
char *TypedOBDState<T>::formatValue() {
    if (this->valueFormatFunction != nullptr) {
        return this->valueFormatFunction(this->getValue());
    }

    char str[50];
    if (strlen(this->valueFormatExpression) != 0) {
        ExprParser parser;
        parser.setVariableResolveFunction([&](const char *varName)-> double {
            if (varName != nullptr) {
                if (varName[0] == '$') {
                    varName++;
                }
                if (strcmp(varName, "value") == 0) {
                    return this->getValue();
                }
            }
            return 0.0;
        });
        double val = parser.evalExp(const_cast<char *>(this->valueFormatExpression));
        sprintf(str, this->valueFormat, static_cast<T>(!isinf(val) ? val : 0));
    } else {
        sprintf(str, this->valueFormat, this->getValue());
    }

    return strdup(str);
}

template<typename T>
void TypedOBDState<T>::toJSON(JsonDocument &doc) {
    OBDState::toJSON(doc);

    if (this->type == obd::READ) {
        if (this->readFunction != nullptr && strlen(this->readFunctionName) != 0) {
            doc["readFunc"] = this->readFunctionName;
        } else {
            doc["pid"]["service"] = this->service;
            doc["pid"]["pid"] = this->pid;
            doc["pid"]["header"] = this->header;
            doc["pid"]["numResponses"] = this->numResponses;
            doc["pid"]["numExpectedBytes"] = this->numExpectedBytes;
            if (this->scaleFactorExpression != nullptr) {
                doc["pid"]["scaleFactor"] = this->scaleFactorExpression;
            }
            if (this->bias != 0) {
                doc["pid"]["bias"] = this->bias;
            }
        }
    }

    doc["value"]["format"] = this->valueFormat;
    if (this->valueFormatFunction != nullptr && strlen(this->valueFormatFunctionName) != 0) {
        doc["value"]["func"] = this->valueFormatFunctionName;
    } else if (strlen(this->valueFormatExpression) != 0) {
        doc["value"]["expression"] = this->valueFormatExpression;
    }
}

OBDStateBool::OBDStateBool(obd::OBDStateType type, const char *name, const char *description,
                           const char *icon, const char *unit, const char *deviceClass,
                           const bool measurement, const bool diagnostic): TypedOBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
    this->oldValue = false;
    this->value = false;
    this->TypedOBDState::setValueFormatFunc([](const bool val) {
        return strdup(val ? "on" : "off");
    });
}

const char *OBDStateBool::valueType() const {
    return "bool";
}

OBDStateBool *OBDStateBool::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                            const uint8_t &numResponses,
                                            const uint8_t &numExpectedBytes, const double &scaleFactor,
                                            const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDStateBool *OBDStateBool::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                            const uint8_t &numResponses,
                                            const uint8_t &numExpectedBytes, const char *scaleFactorExpression,
                                            const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactorExpression, bias);
    return this;
}

OBDStateBool *OBDStateBool::withEnabled(const bool enable) {
    this->setEnabled(enable);
    return this;
}

OBDStateBool *OBDStateBool::withVisible(const bool visible) {
    this->setVisible(visible);
    return this;
}

OBDStateBool *OBDStateBool::withUpdateInterval(const long interval) {
    this->setUpdateInterval(interval);
    return this;
}

OBDStateBool *OBDStateBool::withReadFuncName(const char *funcName) {
    this->setReadFuncName(funcName);
    return this;
}

OBDStateBool *OBDStateBool::withReadFunc(const std::function<bool()> &func) {
    this->setReadFunc(func);
    return this;
}

OBDStateBool *OBDStateBool::withCalcExpression(const char *expression) {
    TypedOBDState::setCalcExpression(expression);
    return this;
}

OBDStateBool *OBDStateBool::withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) {
    TypedOBDState::setPostProcessFunc(postProcessFunction);
    return this;
}

OBDStateBool *OBDStateBool::withValueFormat(const char *format) {
    TypedOBDState::setValueFormat(format);
    return this;
}

OBDStateBool *OBDStateBool::withValueFormatExpression(const char *expression) {
    TypedOBDState::setValueFormatExpression(expression);
    return this;
}

OBDStateBool *OBDStateBool::withValueFormatFuncName(const char *funcName) {
    TypedOBDState::setValueFormatFuncName(funcName);
    return this;
}

OBDStateBool *OBDStateBool::withValueFormatFunc(const std::function<char *(bool)> &valueFormatFunction) {
    TypedOBDState::setValueFormatFunc(valueFormatFunction);
    return this;
}

OBDStateFloat::OBDStateFloat(obd::OBDStateType type, const char *name, const char *description,
                             const char *icon, const char *unit, const char *deviceClass,
                             const bool measurement, const bool diagnostic): TypedOBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
    this->oldValue = 0.0;
    this->value = 0.0;
    this->TypedOBDState::setValueFormat("%4.2f");
}

const char *OBDStateFloat::valueType() const {
    return "float";
}

OBDStateFloat *OBDStateFloat::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                              const uint8_t &numResponses,
                                              const uint8_t &numExpectedBytes, const double &scaleFactor,
                                              const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDStateFloat *OBDStateFloat::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                              const uint8_t &numResponses,
                                              const uint8_t &numExpectedBytes, const char *scaleFactorExpression,
                                              const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactorExpression, bias);
    return this;
}

OBDStateFloat *OBDStateFloat::withEnabled(const bool enable) {
    this->setEnabled(enable);
    return this;
}

OBDStateFloat *OBDStateFloat::withVisible(const bool visible) {
    this->setVisible(visible);
    return this;
}

OBDStateFloat *OBDStateFloat::withUpdateInterval(const long interval) {
    this->setUpdateInterval(interval);
    return this;
}

OBDStateFloat *OBDStateFloat::withReadFuncName(const char *funcName) {
    this->setReadFuncName(funcName);
    return this;
}

OBDStateFloat *OBDStateFloat::withReadFunc(const std::function<float()> &func) {
    this->setReadFunc(func);
    return this;
}

OBDStateFloat *OBDStateFloat::withCalcExpression(const char *expression) {
    TypedOBDState::setCalcExpression(expression);
    return this;
}

OBDStateFloat *OBDStateFloat::withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) {
    TypedOBDState::setPostProcessFunc(postProcessFunction);
    return this;
}

OBDStateFloat *OBDStateFloat::withValueFormat(const char *format) {
    TypedOBDState::setValueFormat(format);
    return this;
}

OBDStateFloat *OBDStateFloat::withValueFormatExpression(const char *expression) {
    TypedOBDState::setValueFormatExpression(expression);
    return this;
}

OBDStateFloat *OBDStateFloat::withValueFormatFuncName(const char *funcName) {
    TypedOBDState::setValueFormatFuncName(funcName);
    return this;
}

OBDStateFloat *OBDStateFloat::withValueFormatFunc(const std::function<char *(float)> &valueFormatFunction) {
    TypedOBDState::setValueFormatFunc(valueFormatFunction);
    return this;
}

OBDStateInt::OBDStateInt(obd::OBDStateType type, const char *name, const char *description,
                         const char *icon, const char *unit, const char *deviceClass,
                         const bool measurement, const bool diagnostic): TypedOBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
    this->oldValue = 0;
    this->value = 0;
    this->TypedOBDState::setValueFormat("%d");
}

const char *OBDStateInt::valueType() const {
    return "int";
}

OBDStateInt *OBDStateInt::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                          const uint8_t &numResponses,
                                          const uint8_t &numExpectedBytes, const double &scaleFactor,
                                          const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDStateInt *OBDStateInt::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                          const uint8_t &numResponses,
                                          const uint8_t &numExpectedBytes, const char *scaleFactorExpression,
                                          const float &bias) {
    this->setPIDSettings(service, pid, header, numResponses, numExpectedBytes, scaleFactorExpression, bias);
    return this;
}

OBDStateInt *OBDStateInt::withEnabled(const bool enable) {
    this->setEnabled(enable);
    return this;
}

OBDStateInt *OBDStateInt::withVisible(const bool visible) {
    this->setVisible(visible);
    return this;
}

OBDStateInt *OBDStateInt::withUpdateInterval(const long interval) {
    this->setUpdateInterval(interval);
    return this;
}

OBDStateInt *OBDStateInt::withReadFuncName(const char *funcName) {
    this->setReadFuncName(funcName);
    return this;
}

OBDStateInt *OBDStateInt::withReadFunc(const std::function<int()> &func) {
    this->setReadFunc(func);
    return this;
}

OBDStateInt *OBDStateInt::withCalcExpression(const char *expression) {
    TypedOBDState::setCalcExpression(expression);
    return this;
}

OBDStateInt *OBDStateInt::withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) {
    TypedOBDState::setPostProcessFunc(postProcessFunction);
    return this;
}

OBDStateInt *OBDStateInt::withValueFormat(const char *format) {
    TypedOBDState::setValueFormat(format);
    return this;
}

OBDStateInt *OBDStateInt::withValueFormatExpression(const char *expression) {
    TypedOBDState::setValueFormatExpression(expression);
    return this;
}

OBDStateInt *OBDStateInt::withValueFormatFuncName(const char *funcName) {
    TypedOBDState::setValueFormatFuncName(funcName);
    return this;
}

OBDStateInt *OBDStateInt::withValueFormatFunc(const std::function<char *(int)> &valueFormatFunction) {
    TypedOBDState::setValueFormatFunc(valueFormatFunction);
    return this;
}
