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

#include <ELMduino.h>
#include <map>
#include <OBDState.h>
#include <vector>

class OBDStates {
    ELM327 *elm327;
    std::vector<OBDState *> states{};

    bool checkPidSupport = false;

    std::function<double(const char *)> varResolveFunction = nullptr;

    std::map<const char *, const std::function<double(double)>> customFunctions{};

    static bool compareStates(const OBDState *a, const OBDState *b);

    void setCustomFunctions(const std::map<const char *, const std::function<double(double)>> &funcs);

    template<typename T>
    T getStateValue(const char *name, T empty);

    template<typename T>
    T getStateValue(const std::string &name, T empty);

    template<typename T>
    void setStateValue(const char *name, T value);

    template<typename T>
    void setStateValue(const std::string &name, T value);

public:
    explicit OBDStates(ELM327 *elm327);

    void setCheckPidSupport(bool enable);

    void setVariableResolveFunction(const std::function<double(const char *)> &func);

    void addCustomFunction(const char *name, const std::function<double(double)> &func);

    void clearStates();

    void getStates(const std::function<bool(OBDState *)> &pred, std::vector<OBDState *> &states);

    template<typename T>
    T *getStateByName(const char *name);

    OBDState *getStateByName(const char *name);

    double getStateValue(const char *name);

    bool getStateValue(const char *name, bool defaultValue);

    void setStateValue(const char *name, bool value);

    float getStateValue(const char *name, float defaultValue);

    void setStateValue(const char *name, float value);

    int getStateValue(const char *name, int defaultValue);

    void setStateValue(const char *name, int value);

    void addState(OBDState *state);

    void listStates() const;

    double avgLastUpdate(const std::function<bool(OBDState *)> &pred);

    OBDState *nextState();
};
