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

#include "OBDStates.h"
#include <Arduino.h>
#include <algorithm>
#include <numeric>

OBDStates::OBDStates(ELM327 *elm327) {
    this->elm327 = elm327;
}

void OBDStates::setCheckPidSupport(const bool enable) {
    this->checkPidSupport = enable;
    for (auto &state: states) {
        state->setCheckPidSupport(checkPidSupport);
    }
}

void OBDStates::setVariableResolveFunction(const std::function<double(const char *)> &func) {
    this->varResolveFunction = func;
}

void OBDStates::addCustomFunction(const char *name, const std::function<double(double)> &func) {
    customFunctions.insert({name, func});
}

void OBDStates::setCustomFunctions(const std::map<const char *, const std::function<double(double)>> &funcs) {
    customFunctions.insert(begin(funcs), end(funcs));
}

void OBDStates::clearStates() {
    if (!states.empty()) {
        for (unsigned i = 0; i < states.size(); ++i) {
            OBDState *state = states[i];
            free(state);
        }
        states.clear();
    }
}

void OBDStates::getStates(const std::function<bool(OBDState *)> &pred, std::vector<OBDState *> &states) {
    std::copy_if(
        this->states.begin(),
        this->states.end(),
        std::back_inserter(states),
        pred
    );
}

bool OBDStates::compareStates(const OBDState *a, const OBDState *b) {
    return a->isProcessing() && !b->isProcessing()
           || (a->getLastUpdate() + a->getUpdateInterval()) < (b->getLastUpdate() + b->getUpdateInterval());
}

template<typename T>
T *OBDStates::getStateByName(const char *name) {
    for (auto &state: states) {
        if (strcmp(state->getName(), name) == 0) {
            return static_cast<T *>(state);
        }
    }
    return nullptr;
}

template<typename T>
T OBDStates::getStateValue(const char *name, T empty) {
    auto *state = getStateByName<TypedOBDState<T> >(name);
    if (state != nullptr) {
        return state->getValue();
    }

    return empty;
}

template<typename T>
T OBDStates::getStateValue(const std::string &name, T empty) {
    return getStateValue<T>(name.c_str(), empty);
}

template<typename T>
void OBDStates::setStateValue(const char *name, T value) {
    auto *state = getStateByName<TypedOBDState<T> >(name);
    if (state != nullptr) {
        state->setValue(value);
    }
}

template<typename T>
void OBDStates::setStateValue(const std::string &name, T value) {
    setStateValue(name.c_str(), value);
}

OBDState *OBDStates::getStateByName(const char *name) {
    return getStateByName<OBDState>(name);
}

double OBDStates::getStateValue(const char *name) {
    auto *state = getStateByName(name);
    if (state != nullptr) {
        if (state->valueType() == "int") {
            auto *is = reinterpret_cast<OBDStateInt *>(state);
            return is->getValue();
        }
        if (state->valueType() == "float") {
            auto *is = reinterpret_cast<OBDStateFloat *>(state);
            return is->getValue();
        }
        if (state->valueType() == "bool") {
            auto *is = reinterpret_cast<OBDStateBool *>(state);
            return is->getValue();
        }
    }
    return 0.0;
}

bool OBDStates::getStateValue(const char *name, bool defaultValue) {
    return getStateValue<bool>(name, defaultValue);
}

void OBDStates::setStateValue(const char *name, bool value) {
    setStateValue<bool>(name, value);
}

float OBDStates::getStateValue(const char *name, float defaultValue) {
    return getStateValue<float>(name, defaultValue);
}

void OBDStates::setStateValue(const char *name, const float value) {
    setStateValue<float>(name, value);
}

int OBDStates::getStateValue(const char *name, int defaultValue) {
    return getStateValue<int>(name, defaultValue);
}

void OBDStates::setStateValue(const char *name, const int value) {
    setStateValue<int>(name, value);
}

void OBDStates::addState(OBDState *state) {
    if (getStateByName<OBDState>(state->getName()) == nullptr) {
        state->setELM327(elm327);
        state->setCheckPidSupport(checkPidSupport);
        states.push_back(state);
    }
}

void OBDStates::listStates() const {
    for (auto &state: states) {
        Serial.printf("%s: %d %d\n", state->getName(), state->getType(), state->isEnabled());
    }
}

double OBDStates::avgLastUpdate(const std::function<bool(OBDState *)> &pred) {
    std::vector<OBDState *> readStates{};
    getStates(pred, readStates);

    std::vector<uint32_t> data;
    for (auto &state: readStates) {
        data.push_back(millis() - state->getLastUpdate());
    }
    int sum = std::accumulate(data.begin(), data.end(), 0);
    return static_cast<double>(sum) / data.size();
}

OBDState *OBDStates::nextState() {
    if (!states.empty() && elm327 != nullptr && elm327->elm_port) {
        std::vector<OBDState *> readStates{};
        getStates([](const OBDState *state) {
            return state->isEnabled() &&
                   (state->getType() ==  obd::READ || state->getType() ==  obd::CALC && state->hasCalcExpression()) &&
                   (state->getUpdateInterval() != -1 || state->getUpdateInterval() == -1 && state->getLastUpdate() ==
                    0);
        }, readStates);
        sort(readStates.begin(), readStates.end(), compareStates);

        OBDState &state = *readStates.at(0);
        if (state.getUpdateInterval() == -1 || state.getLastUpdate() + state.getUpdateInterval() < millis()) {
            // int aFreeInternalHeapSizeBefore = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

            if (state.getType() ==  obd::READ) {
                state.readValue();
            } else if (state.getType() ==  obd::CALC) {
                state.calcValue(varResolveFunction, customFunctions);
            }

            // int aFreeInternalHeapSizeAfter = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            // int aMinFreeInternalHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
            // Serial.print("heap: ");
            // Serial.print(aFreeInternalHeapSizeBefore);
            // Serial.print(", after ");
            // Serial.print(aFreeInternalHeapSizeAfter);
            // Serial.print(", delta ");
            // Serial.print(aFreeInternalHeapSizeBefore - aFreeInternalHeapSizeAfter);
            // Serial.print(", lowest free ");
            // Serial.println(aMinFreeInternalHeapSize);

            return &state;
        }
    }

    return nullptr;
}
