/*
* This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, in a file called gpl.txt or license.txt.
 * If not, write to the Free Software Foundation Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef _BLE_CLIENT_SERIAL_H_
#define _BLE_CLIENT_SERIAL_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "Arduino.h"
#include "Stream.h"
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "BLEScanResultsSet.h"

typedef std::function<void(NimBLERemoteCharacteristic *pBLERemoteCharacteristic, const uint8_t *buffer, size_t size)>
BLESerialDataCb;
typedef std::function<void()> BLEConnectCb;
typedef std::function<void()> BLEDisconnectCb;
typedef std::function<void(const NimBLEAdvertisedDevice *pAdvertisedDevice)> BLEAdvertisedDeviceCb;

/**
 * BLE Serial Client
 * Was adopted from <a href="https://github.com/vdvornichenko/obd-ble-serial">vdvornichenko/obd-ble-serial</a>
 */
class BLEClientSerial : public Stream {
    bool init = false;
    String local_name;

    std::string buffer;

    NimBLEUUID serviceUUID;
    NimBLEUUID rxUUID;
    NimBLEUUID txUUID;

    NimBLEClient *pClient = nullptr;
    NimBLERemoteCharacteristic *pTxCharacteristic = nullptr;
    NimBLERemoteCharacteristic *pRxCharacteristic = nullptr;

    BLESerialDataCb pDataCb;
    BLEConnectCb pConnectCb;
    BLEDisconnectCb pDisconnectCb;

    void notifyCallback(NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
                        bool isNotify);

    friend class AdvertisedDeviceCallbacks;
    friend class ClientCallbacks;

public:
    BLEClientSerial();

    ~BLEClientSerial();

    explicit operator bool() const;

    bool begin(const String &localName = String(), const std::string &serviceUUID = "FFF0",
               const std::string &rxUUID = "FFF1",
               const std::string &txUUID = "FFF2");

    bool begin(unsigned long baud) {
        //compatibility
        return begin();
    }

    bool connect(const String &remoteName);

    bool connect(const NimBLEAddress &remoteAddress);

    bool connect(const uint8_t remoteAddress[]) {
        return connect(NimBLEAddress(remoteAddress, 0));
    }

    bool connected() const;

    bool disconnect();

    bool isClosed() const;

    int available();

    int peek();

    int read();

    size_t write(uint8_t c);

    size_t write(const uint8_t *buffer, size_t size);

    void flush();

    void end();

    void onData(const BLESerialDataCb &cb);

    void onConnect(const BLEConnectCb &cb);

    void onDisconnect(const BLEDisconnectCb &cb);

    BLEScanResultsSet *discover(int timeout = 30000);

    bool discoverAsync(const BLEAdvertisedDeviceCb &cb, int timeout = 30000);

    void discoverAsyncStop();

    void discoverClear();

    BLEScanResultsSet *getScanResults();

    const int INQ_TIME = 100;
};

#endif
#endif
