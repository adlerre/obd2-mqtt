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

OBDStates::OBDStates(ELM327 *elm327) {
    this->elm327 = elm327;
}

void OBDStates::setCheckPidSupport(bool enable) {
    this->checkPidSupport = enable;
    for (auto &state: states) {
        state->setCheckPidSupport(checkPidSupport);
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
        if (state->getName() == name) {
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

OBDState *OBDStates::nextState() {
    if (!states.empty() && elm327 != nullptr && elm327->elm_port) {
        std::vector<OBDState *> readStates{};
        getStates([](const OBDState *state) {
            return state->getType() == READ && state->isEnabled() &&
                   (state->getUpdateInterval() != -1 || state->getUpdateInterval() == -1 && state->getLastUpdate() ==
                    0);
        }, readStates);
        sort(readStates.begin(), readStates.end(), compareStates);

        OBDState &state = *readStates.at(0);
        if (state.getUpdateInterval() == -1 || state.getLastUpdate() + state.getUpdateInterval() < millis()) {
            state.readValue();
            return &state;
        }
    }

    return nullptr;
}
