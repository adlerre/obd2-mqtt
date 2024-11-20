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
#include <functional>

typedef enum {
    READ,
    CALC,
} OBDStateType;

class OBDState {
protected:
    ELM327 *elm327 = nullptr;

    OBDStateType type = READ;

    const char *name{};
    const char *description{};
    const char *icon{};
    const char *unit{};
    const char *deviceClass{};
    bool measurement = true;
    bool diagnostic = false;

    uint8_t service = 0;
    uint16_t pid = 0;
    uint8_t numResponses = 0;
    uint8_t numExpectedBytes = 0;
    double scaleFactor = 1;
    float bias = 0;

    bool init = false;
    bool checkPidSupport = false;
    bool supported = true;
    bool enabled = true;
    bool processing = false;

    long updateInterval = 1000;

    long previousUpdate = 0;

    long lastUpdate = 0;

    void setPreviousUpdate(long timestamp);

    void setLastUpdate(long timestamp);

public:
    OBDState(OBDStateType type, const char *name, const char *description, const char *icon,
             const char *unit = "", const char *deviceClass = "", bool measurement = true, bool diagnostic = false);

    virtual ~OBDState() = default;

    OBDStateType getType() const;

    virtual char *valueType();

    void setELM327(ELM327 *elm327);

    const char *getName() const;

    const char *getDescription() const;

    const char *getIcon() const;

    const char *getUnit() const;

    const char *getDeviceClass() const;

    bool isMeasurement() const;

    bool isDiagnostic() const;

    void setPIDSettings(const uint8_t &service, const uint16_t &pid,
                        const uint8_t &numResponses, const uint8_t &numExpectedBytes, const double &scaleFactor = 1,
                        const float &bias = 0);

    virtual OBDState *withPIDSettings(const uint8_t &service, const uint16_t &pid,
                                      const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                      const double &scaleFactor = 1,
                                      const float &bias = 0);

    bool isInit() const;

    void setCheckPidSupport(bool enable);

    bool isSupported() const;

    bool isEnabled() const;

    void setEnabled(bool enable);

    virtual OBDState *withEnabled(bool enable);

    bool isProcessing() const;

    void setUpdateInterval(long interval);

    virtual OBDState *withUpdateInterval(long interval);

    long getUpdateInterval() const;

    long getPreviousUpdate() const;

    long getLastUpdate() const;

    virtual void readValue();
};

template<typename T>
class TypedOBDState : public OBDState {
protected:
    T oldValue;

    T value;

    std::function<T()> readFunction = nullptr;

    std::function<void(TypedOBDState *)> postProcessFunction = nullptr;

    const char *valueFormat = "%d";

    std::function<char *(T)> valueFormatFunction = nullptr;

public:
    TypedOBDState(OBDStateType type, const char *name, const char *description, const char *icon,
                  const char *unit = "", const char *deviceClass = "", bool measurement = true,
                  bool diagnostic = false);

    char *valueType() override;

    TypedOBDState *withPIDSettings(const uint8_t &service, const uint16_t &pid,
                                   const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                   const double &scaleFactor = 1,
                                   const float &bias = 0) override;

    TypedOBDState *withEnabled(bool enable) override;

    virtual T getOldValue();

    virtual void setOldValue(T value);

    virtual T getValue();

    virtual void setValue(T value);

    TypedOBDState *withUpdateInterval(long interval) override;

    void setReadFunc(const std::function<T()> &func);

    virtual TypedOBDState *withReadFunc(const std::function<T()> &func);

    void readValue() override;

    virtual void setPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction);

    virtual TypedOBDState *withPostProcessFunc(std::function<void(TypedOBDState *)> postProcessFunction);

    virtual void setValueFormat(const char *format);

    virtual TypedOBDState *withValueFormat(const char *format);

    virtual void setValueFormatFunc(const std::function<char *(T)> &valueFormatFunction);

    virtual TypedOBDState *withValueFormatFunc(const std::function<char *(T)> &valueFormatFunction);

    /**
     * Formats value with set valueFormat property or with valueFormatFunction.
     * HINT: you must use free() to deallocate memory after usage
     *
     * @return the formated value
     */
    virtual char *formatValue();
};

class OBDStateBool final : public TypedOBDState<bool> {
public:
    OBDStateBool(OBDStateType type, const char *name, const char *description, const char *icon,
                 const char *unit = "", const char *deviceClass = "", bool measurement = true, bool diagnostic = false);

    char *valueType() override;

    OBDStateBool *withPIDSettings(const uint8_t &service, const uint16_t &pid,
                                  const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                  const double &scaleFactor = 1,
                                  const float &bias = 0) override;

    OBDStateBool *withEnabled(bool enable) override;

    OBDStateBool *withUpdateInterval(long interval) override;

    OBDStateBool *withReadFunc(const std::function<bool()> &func) override;

    OBDStateBool *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction);

    OBDStateBool *withValueFormat(const char *format) override;

    OBDStateBool *withValueFormatFunc(const std::function<char *(bool)> &valueFormatFunction);
};

class OBDStateFloat final : public TypedOBDState<float> {
public:
    OBDStateFloat(OBDStateType type, const char *name, const char *description, const char *icon,
                  const char *unit = "", const char *deviceClass = "", bool measurement = true,
                  bool diagnostic = false);

    char *valueType() override;

    OBDStateFloat *withPIDSettings(const uint8_t &service, const uint16_t &pid,
                                   const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                   const double &scaleFactor = 1,
                                   const float &bias = 0) override;

    OBDStateFloat *withEnabled(bool enable) override;

    OBDStateFloat *withUpdateInterval(long interval) override;

    OBDStateFloat *withReadFunc(const std::function<float()> &func) override;

    OBDStateFloat *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction);

    OBDStateFloat *withValueFormat(const char *format) override;

    OBDStateFloat *withValueFormatFunc(const std::function<char *(float)> &valueFormatFunction);
};

class OBDStateInt final : public TypedOBDState<int> {
public:
    OBDStateInt(OBDStateType type, const char *name, const char *description, const char *icon,
                const char *unit = "", const char *deviceClass = "", bool measurement = true, bool diagnostic = false);

    char *valueType() override;

    OBDStateInt *withPIDSettings(const uint8_t &service, const uint16_t &pid,
                                 const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                 const double &scaleFactor = 1,
                                 const float &bias = 0) override;

    OBDStateInt *withEnabled(bool enable) override;

    OBDStateInt *withUpdateInterval(long interval) override;

    OBDStateInt *withReadFunc(const std::function<int()> &func) override;

    OBDStateInt *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction);

    OBDStateInt *withValueFormat(const char *format) override;

    OBDStateInt *withValueFormatFunc(const std::function<char *(int)> &valueFormatFunction);
};
