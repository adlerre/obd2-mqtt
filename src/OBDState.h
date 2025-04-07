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
#include <map>
#include <ArduinoJson.h>

namespace obd {
    typedef enum {
        READ,
        CALC,
    } OBDStateType;
}

class OBDState {
protected:
    ELM327 *elm327 = nullptr;

    obd::OBDStateType type = obd::READ;

    char name[33] = "\0";
    char description[129] = "\0";
    char icon[33] = "\0";
    char unit[9] = "\0";
    char deviceClass[33] = "\0";
    bool measurement = true;
    bool diagnostic = false;

    uint8_t service = 0;
    uint16_t pid = 0;
    uint16_t header = 0;
    uint8_t numResponses = 0;
    uint8_t numExpectedBytes = 0;
    double scaleFactor = 1;
    char scaleFactorExpression[257] = "\0";
    float bias = 0;

    char calcExpression[257] = "\0";

    bool init = false;
    bool checkPidSupport = false;
    bool setHeader = false;
    bool supported = true;
    bool enabled = true;
    bool visible = true;
    bool processing = false;

    long updateInterval = 1000;

    long previousUpdate = 0;

    long lastUpdate = 0;

    int8_t updateStatus = 0;

    void setPreviousUpdate(long timestamp);

    void setLastUpdate(long timestamp);

public:
    void *operator new(size_t size);

    void operator delete(void *ptr);

    OBDState(obd::OBDStateType type, const char *name, const char *description, const char *icon,
             const char *unit = "", const char *deviceClass = "", bool measurement = true, bool diagnostic = false);

    virtual ~OBDState() = default;

    obd::OBDStateType getType() const;

    virtual const char *valueType() const;

    void setELM327(ELM327 *elm327);

    const char *getName() const;

    const char *getDescription() const;

    const char *getIcon() const;

    const char *getUnit() const;

    const char *getDeviceClass() const;

    bool isMeasurement() const;

    bool isDiagnostic() const;

    virtual void setCalcExpression(const char *expression);

    virtual bool hasCalcExpression() const;

    uint32_t supportedPIDs(const uint8_t &service, const uint16_t &pid) const;

    bool isPIDSupported(const uint8_t &service, const uint16_t &pid) const;

    void setPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                        const uint8_t &numResponses, const uint8_t &numExpectedBytes, const double &scaleFactor = 1,
                        const float &bias = 0);

    void setPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                        const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                        const char *scaleFactorExpression = nullptr,
                        const float &bias = 0);

    virtual OBDState *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                      const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                      const double &scaleFactor = 1,
                                      const float &bias = 0);

    virtual OBDState *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                      const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                      const char *scaleFactorExpression = nullptr,
                                      const float &bias = 0);

    bool isInit() const;

    void setCheckPidSupport(bool enable);

    bool isSupported() const;

    bool isEnabled() const;

    void setEnabled(bool enable);

    virtual OBDState *withEnabled(bool enable);

    bool isVisible() const;

    void setVisible(bool visible);

    virtual OBDState *withVisible(bool visible);

    bool isProcessing() const;

    void setUpdateInterval(long interval);

    virtual OBDState *withUpdateInterval(long interval);

    long getUpdateInterval() const;

    long getPreviousUpdate() const;

    long getLastUpdate() const;

    virtual void readValue();

    virtual void calcValue(const std::function<double(const char *)> &func,
                           const std::map<const char *, const std::function<double(double)>> &funcs = {});

    virtual void toJSON(JsonDocument &doc);
};

template<typename T>
class TypedOBDState : public OBDState {
protected:
    T oldValue;

    T value;

    char readFunctionName[33] = "\0";

    std::function<T()> readFunction = nullptr;

    std::function<void(TypedOBDState *)> postProcessFunction = nullptr;

    char valueFormat[17] = "%d";

    char valueFormatExpression[257] = "\0";

    char valueFormatFunctionName[33] = "\0";

    std::function<char *(T)> valueFormatFunction = nullptr;

public:
    TypedOBDState(obd::OBDStateType type, const char *name, const char *description, const char *icon,
                  const char *unit = "", const char *deviceClass = "", bool measurement = true,
                  bool diagnostic = false);

    const char *valueType() const override;

    TypedOBDState *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                   const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                   const double &scaleFactor = 1,
                                   const float &bias = 0) override;

    TypedOBDState *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                   const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                   const char *scaleFactorExpression = nullptr,
                                   const float &bias = 0) override;

    TypedOBDState *withEnabled(bool enable) override;

    TypedOBDState *withVisible(bool visible) override;

    virtual T getOldValue();

    virtual void setOldValue(T value);

    virtual T getValue();

    virtual void setValue(T value);

    TypedOBDState *withUpdateInterval(long interval) override;

    void setReadFuncName(const char *funcName);

    virtual TypedOBDState *withReadFuncName(const char *funcName);

    void setReadFunc(const std::function<T()> &func);

    virtual TypedOBDState *withReadFunc(const std::function<T()> &func);

    void readValue() override;

    virtual TypedOBDState *withCalcExpression(const char *expression);

    void calcValue(const std::function<double(const char *)> &func,
                   const std::map<const char *, const std::function<double(double)>> &funcs) override;

    virtual void setPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction);

    virtual TypedOBDState *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction);

    virtual void setValueFormat(const char *format);

    virtual TypedOBDState *withValueFormat(const char *format);

    virtual void setValueFormatExpression(const char *expression);

    virtual TypedOBDState *withValueFormatExpression(const char *expression);

    virtual void setValueFormatFuncName(const char *funcName);

    virtual TypedOBDState *withValueFormatFuncName(const char *funcName);

    virtual void setValueFormatFunc(const std::function<char *(T)> &valueFormatFunction);

    virtual TypedOBDState *withValueFormatFunc(const std::function<char *(T)> &valueFormatFunction);

    /**
     * Formats value with set valueFormat property or with valueFormatFunction.
     * HINT: you must use free() to deallocate memory after usage
     *
     * @return the formated value
     */
    virtual char *formatValue();

    void toJSON(JsonDocument &doc) override;
};

class OBDStateBool final : public TypedOBDState<bool> {
public:
    OBDStateBool(obd::OBDStateType type, const char *name, const char *description, const char *icon,
                 const char *unit = "", const char *deviceClass = "", bool measurement = true, bool diagnostic = false);

    const char *valueType() const override;

    OBDStateBool *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                  const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                  const double &scaleFactor = 1,
                                  const float &bias = 0) override;

    OBDStateBool *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                  const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                  const char *scaleFactorExpression = nullptr,
                                  const float &bias = 0) override;

    OBDStateBool *withEnabled(bool enable) override;

    OBDStateBool *withVisible(bool visible) override;

    OBDStateBool *withUpdateInterval(long interval) override;

    OBDStateBool *withReadFuncName(const char *funcName) override;

    OBDStateBool *withReadFunc(const std::function<bool()> &func) override;

    OBDStateBool *withCalcExpression(const char *expression) override;

    OBDStateBool *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) override;

    OBDStateBool *withValueFormat(const char *format) override;

    OBDStateBool *withValueFormatExpression(const char *expression) override;

    OBDStateBool *withValueFormatFuncName(const char *funcName) override;

    OBDStateBool *withValueFormatFunc(const std::function<char *(bool)> &valueFormatFunction) override;
};

class OBDStateFloat final : public TypedOBDState<float> {
public:
    OBDStateFloat(obd::OBDStateType type, const char *name, const char *description, const char *icon,
                  const char *unit = "", const char *deviceClass = "", bool measurement = true,
                  bool diagnostic = false);

    const char *valueType() const override;

    OBDStateFloat *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                   const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                   const double &scaleFactor = 1,
                                   const float &bias = 0) override;

    OBDStateFloat *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                   const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                   const char *scaleFactorExpression = nullptr,
                                   const float &bias = 0) override;

    OBDStateFloat *withEnabled(bool enable) override;

    OBDStateFloat *withVisible(bool visible) override;

    OBDStateFloat *withUpdateInterval(long interval) override;

    OBDStateFloat *withReadFuncName(const char *funcName) override;

    OBDStateFloat *withReadFunc(const std::function<float()> &func) override;

    OBDStateFloat *withCalcExpression(const char *expression) override;

    OBDStateFloat *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) override;

    OBDStateFloat *withValueFormat(const char *format) override;

    OBDStateFloat *withValueFormatExpression(const char *expression) override;

    OBDStateFloat *withValueFormatFuncName(const char *funcName) override;

    OBDStateFloat *withValueFormatFunc(const std::function<char *(float)> &valueFormatFunction) override;
};

class OBDStateInt final : public TypedOBDState<int> {
public:
    OBDStateInt(obd::OBDStateType type, const char *name, const char *description, const char *icon,
                const char *unit = "", const char *deviceClass = "", bool measurement = true, bool diagnostic = false);

    const char *valueType() const override;

    OBDStateInt *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                 const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                 const double &scaleFactor = 1,
                                 const float &bias = 0) override;

    OBDStateInt *withPIDSettings(const uint8_t &service, const uint16_t &pid, const uint16_t &header,
                                 const uint8_t &numResponses, const uint8_t &numExpectedBytes,
                                 const char *scaleFactorExpression = nullptr,
                                 const float &bias = 0) override;

    OBDStateInt *withEnabled(bool enable) override;

    OBDStateInt *withVisible(bool visible) override;

    OBDStateInt *withUpdateInterval(long interval) override;

    OBDStateInt *withReadFuncName(const char *funcName) override;

    OBDStateInt *withReadFunc(const std::function<int()> &func) override;

    OBDStateInt *withCalcExpression(const char *expression) override;

    OBDStateInt *withPostProcessFunc(const std::function<void(TypedOBDState *)> &postProcessFunction) override;

    OBDStateInt *withValueFormat(const char *format) override;

    OBDStateInt *withValueFormatExpression(const char *expression) override;

    OBDStateInt *withValueFormatFuncName(const char *funcName) override;

    OBDStateInt *withValueFormatFunc(const std::function<char *(int)> &valueFormatFunction) override;
};
