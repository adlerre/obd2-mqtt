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

OBDState::OBDState(const OBDStateType type, const char *name, const char *description, const char *icon,
                   const char *unit, const char *deviceClass, const bool measurement, const bool diagnostic) {
    this->type = type;
    this->name = name;
    this->description = description;
    this->icon = icon;
    this->unit = unit;
    this->deviceClass = deviceClass;
    this->measurement = measurement;
    this->diagnostic = diagnostic;
    this->updateInterval = 100;
}

OBDStateType OBDState::getType() const {
    return this->type;
}

char *OBDState::valueType() {
    return const_cast<char *>("base");
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
    this->type = CALC;
    this->calcExpression = expression;
}

bool OBDState::hasCalcExpression() const {
    return this->calcExpression != nullptr;
}

uint32_t OBDState::supportedPIDs(const uint8_t &service, const uint16_t &pid) const {
    const uint8_t pidInterval = (pid / PID_INTERVAL_OFFSET) * PID_INTERVAL_OFFSET;
    return static_cast<uint32_t>(elm327->processPID(service, pidInterval, 1, 4));
}

bool OBDState::isPIDSupported(const uint8_t &service, const uint16_t &pid) const {
    const uint8_t pidInterval = (pid / PID_INTERVAL_OFFSET) * PID_INTERVAL_OFFSET;
    const uint32_t response = supportedPIDs(service, pidInterval);
    if (elm327->nb_rx_state == ELM_SUCCESS) {
        return ((response >> (32 - pid)) & 0x1);
    }
    return false;
}

void OBDState::setPIDSettings(const uint8_t &service, const uint16_t &pid, const uint8_t &numResponses,
                              const uint8_t &numExpectedBytes, const double &scaleFactor, const float &bias) {
    this->type = READ;
    this->service = service;
    this->pid = pid;
    this->numResponses = numResponses;
    this->numExpectedBytes = numExpectedBytes;
    this->scaleFactor = scaleFactor;
    this->bias = bias;
    this->updateInterval = 100;
}

OBDState *OBDState::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint8_t &numResponses,
                                    const uint8_t &numExpectedBytes, const double &scaleFactor, const float &bias) {
    this->setPIDSettings(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

bool OBDState::isInit() const {
    return this->init;
}

void OBDState::setCheckPidSupport(bool enable) {
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

OBDState *OBDState::withEnabled(bool enable) {
    this->setEnabled(enable);
    return this;
}

bool OBDState::isVisible() const {
    return this->visible;
}

void OBDState::setVisible(bool visible) {
    this->visible = visible;
}

OBDState *OBDState::withVisible(bool visible) {
    this->setVisible(visible);
    return this;
}

bool OBDState::isProcessing() const {
    return this->processing;
}

void OBDState::setUpdateInterval(long interval) {
    updateInterval = interval;
}

long OBDState::getUpdateInterval() const {
    return updateInterval;
}

OBDState *OBDState::withUpdateInterval(long interval) {
    this->updateInterval = interval;
    return this;
}

void OBDState::setPreviousUpdate(long timestamp) {
    this->previousUpdate = timestamp;
}

long OBDState::getPreviousUpdate() const {
    return previousUpdate;
}

void OBDState::setLastUpdate(long timestamp) {
    this->lastUpdate = timestamp;
}

long OBDState::getLastUpdate() const {
    return lastUpdate;
}

void OBDState::readValue() {
}

void OBDState::calcValue(const std::function<double(const char *)> &func,
                         const std::map<const char *, const std::function<double(double)>> &funcs) {
}

template<typename T>
TypedOBDState<T>::TypedOBDState(OBDStateType type, const char *name, const char *description,
                                const char *icon, const char *unit, const char *deviceClass,
                                const bool measurement, const bool diagnostic): OBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
}

template<typename T>
char *TypedOBDState<T>::valueType() {
    return const_cast<char *>("generic");
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withPIDSettings(const uint8_t &service, const uint16_t &pid,
                                                    const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                                    const double &scaleFactor, const float &bias) {
    this->setPIDSettings(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withEnabled(bool enable) {
    this->setEnabled(enable);
    return this;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withVisible(bool visible) {
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
void TypedOBDState<T>::setReadFunc(const std::function<T()> &func) {
    this->type = READ;
    this->readFunction = func;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withReadFunc(const std::function<T()> &func) {
    this->setReadFunc(func);
    return this;
}

template<typename T>
void TypedOBDState<T>::readValue() {
    if (elm327 != nullptr && elm327->elm_port && this->type == READ) {
        if (!this->init && this->readFunction == nullptr) {
            this->supported = this->checkPidSupport && isPIDSupported(this->service, this->pid) || true;
            this->init = this->checkPidSupport && elm327->nb_rx_state == ELM_SUCCESS || true;
        } else if (this->readFunction != nullptr) {
            this->init = true;
        }

        if (this->init && this->supported) {
            if (!this->processing) {
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
            } else if (elm327->nb_rx_state == ELM_NO_DATA) {
                this->value = 0;
                this->lastUpdate = millis();
                this->processing = false;
            } else if (elm327->nb_rx_state != ELM_GETTING_MSG) {
                this->lastUpdate = millis();
                this->processing = false;
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
    if (this->type == CALC && this->calcExpression != nullptr) {
        if (!this->processing) {
            this->oldValue = this->value;
            this->previousUpdate = this->lastUpdate;
            this->processing = true;
        }

        ExprParser parser;
        parser.setCustomFunctions(funcs);
        parser.setVariableResolveFunction(func);
        this->value = static_cast<T>(parser.evalExp(this->calcExpression));
        if (strlen(parser.errormsg) > 0) {
            Serial.println();
            Serial.print(this->calcExpression);
            Serial.print(": ");
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
    this->valueFormat = format;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withValueFormat(const char *format) {
    this->setValueFormat(format);
    return this;
}

template<typename T>
void TypedOBDState<T>::setValueFormatExpression(const char *expression) {
    this->valueFormatExpression = expression;
}

template<typename T>
TypedOBDState<T> *TypedOBDState<T>::withValueFormatExpression(const char *expression) {
    this->setValueFormatExpression(expression);
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
    if (this->valueFormatExpression != nullptr) {
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
        double val = parser.evalExp(this->valueFormatExpression);
        sprintf(str, this->valueFormat, static_cast<T>(!isinf(val) ? val : 0));
    } else {
        sprintf(str, this->valueFormat, this->getValue());
    }

    return strdup(str);
}

OBDStateBool::OBDStateBool(OBDStateType type, const char *name, const char *description,
                           const char *icon, const char *unit, const char *deviceClass,
                           const bool measurement, const bool diagnostic): TypedOBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
    this->oldValue = false;
    this->value = false;
    this->TypedOBDState::setValueFormatFunc([](const bool val) {
        return strdup(val ? "on" : "off");
    });
}

char *OBDStateBool::valueType() {
    return const_cast<char *>("bool");
}

OBDStateBool *OBDStateBool::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint8_t &numResponses,
                                            const uint8_t &numExpectedBytes, const double &scaleFactor,
                                            const float &bias) {
    this->setPIDSettings(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDStateBool *OBDStateBool::withEnabled(bool enable) {
    this->setEnabled(enable);
    return this;
}

OBDStateBool * OBDStateBool::withVisible(bool visible) {
    this->setVisible(visible);
    return this;
}

OBDStateBool *OBDStateBool::withUpdateInterval(long interval) {
    this->setUpdateInterval(interval);
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

OBDStateBool *OBDStateBool::withValueFormatFunc(const std::function<char *(bool)> &valueFormatFunction) {
    TypedOBDState::setValueFormatFunc(valueFormatFunction);
    return this;
}

OBDStateFloat::OBDStateFloat(OBDStateType type, const char *name, const char *description,
                             const char *icon, const char *unit, const char *deviceClass,
                             const bool measurement, const bool diagnostic): TypedOBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
    this->oldValue = 0.0;
    this->value = 0.0;
    this->TypedOBDState::setValueFormat("%4.2f");
}

char *OBDStateFloat::valueType() {
    return const_cast<char *>("float");
}

OBDStateFloat *OBDStateFloat::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint8_t &numResponses,
                                              const uint8_t &numExpectedBytes, const double &scaleFactor,
                                              const float &bias) {
    this->setPIDSettings(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDStateFloat *OBDStateFloat::withEnabled(bool enable) {
    this->setEnabled(enable);
    return this;
}

OBDStateFloat * OBDStateFloat::withVisible(bool visible) {
    this->setVisible(visible);
    return this;
}

OBDStateFloat *OBDStateFloat::withUpdateInterval(long interval) {
    this->setUpdateInterval(interval);
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

OBDStateFloat *OBDStateFloat::withValueFormatFunc(const std::function<char *(float)> &valueFormatFunction) {
    TypedOBDState::setValueFormatFunc(valueFormatFunction);
    return this;
}

OBDStateInt::OBDStateInt(OBDStateType type, const char *name, const char *description,
                         const char *icon, const char *unit, const char *deviceClass,
                         const bool measurement, const bool diagnostic): TypedOBDState(
    type, name, description, icon, unit, deviceClass, measurement, diagnostic) {
    this->oldValue = 0;
    this->value = 0;
    this->TypedOBDState::setValueFormat("%d");
}

char *OBDStateInt::valueType() {
    return const_cast<char *>("int");
}

OBDStateInt *OBDStateInt::withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint8_t &numResponses,
                                          const uint8_t &numExpectedBytes, const double &scaleFactor,
                                          const float &bias) {
    this->setPIDSettings(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    return this;
}

OBDStateInt *OBDStateInt::withEnabled(bool enable) {
    this->setEnabled(enable);
    return this;
}

OBDStateInt * OBDStateInt::withVisible(bool visible) {
    this->setVisible(visible);
    return this;
}

OBDStateInt *OBDStateInt::withUpdateInterval(long interval) {
    this->setUpdateInterval(interval);
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

OBDStateInt *OBDStateInt::withValueFormatFunc(const std::function<char *(int)> &valueFormatFunction) {
    TypedOBDState::setValueFormatFunc(valueFormatFunction);
    return this;
}
