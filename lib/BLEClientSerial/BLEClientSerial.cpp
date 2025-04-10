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

#include "BLEClientSerial.h"

static BLEScanResultsSet scanResults;

static void printFriendlyResponse(const uint8_t *pData, size_t length) {
    log_d("");
    for (int i = 0; i < length; i++) {
        char recChar = static_cast<char>(pData[i]);
        if (recChar == '\f')
            log_d("\\f");
        else if (recChar == '\n')
            log_d("\\n");
        else if (recChar == '\r')
            log_d("\\r");
        else if (recChar == '\t')
            log_d("\\t");
        else if (recChar == '\v')
            log_d("\\v");
        else if (recChar == ' ')
            // convert spaces to underscore, easier to see in debug output
            log_d("_");
        else
            // display regular printable
            log_d("%c", recChar);
    }
    log_d("");
}

class AdvertisedDeviceCallbacks final : public NimBLEScanCallbacks {
    NimBLEUUID serviceUUID;
    BLEAdvertisedDeviceCb callback = nullptr;

public:
    AdvertisedDeviceCallbacks(const NimBLEUUID &serviceFilter) {
        serviceUUID = serviceFilter;
    }

    explicit AdvertisedDeviceCallbacks(const NimBLEUUID &serviceFilter, const BLEAdvertisedDeviceCb &cb) {
        serviceUUID = serviceFilter;
        callback = cb;
    }

    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
        // filter for required service
        if (advertisedDevice != nullptr && advertisedDevice->isAdvertisingService(serviceUUID) &&
            scanResults.add(*advertisedDevice) && callback) {
            callback(advertisedDevice);
        }
    }
};

class ClientCallbacks final : public NimBLEClientCallbacks {
    BLEConnectCb cCb;
    BLEDisconnectCb disCb;

public:
    ClientCallbacks();

    ClientCallbacks(const BLEConnectCb &connectCb, const BLEDisconnectCb &disconnectCb) {
        cCb = connectCb;
        disCb = disconnectCb;
    }

    void onConnect(NimBLEClient *pClient) override {
        if (cCb != nullptr) {
            cCb();
        }
    }

    void onDisconnect(NimBLEClient *pClient, int reason) override {
        if (disCb != nullptr) {
            disCb();
        }
    }
};

// Constructor
BLEClientSerial::BLEClientSerial() = default;

// Destructor
BLEClientSerial::~BLEClientSerial() = default;

BLEClientSerial::operator bool() const {
    return init;
}

void BLEClientSerial::notifyCallback(NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                                     size_t length,
                                     bool isNotify) {
    if (pDataCb) {
        pDataCb(pBLERemoteCharacteristic, pData, length);
    }

    log_d("ELM RESPONSE > ");
    printFriendlyResponse(pData, length);

    std::string receivedData(reinterpret_cast<char *>(pData), length);

    if (buffer.size() < length || buffer.substr(buffer.size() - length) != receivedData) {
        buffer += receivedData;
    }

    log_v("buffer after append: ");
    log_v("%s", buffer.c_str());
}

// Begin bluetooth serial
bool BLEClientSerial::begin(const String &localName, const std::string &serviceUUID, const std::string &rxUUID,
                            const std::string &txUUID) {
    local_name = localName;
    this->serviceUUID = NimBLEUUID(serviceUUID);
    this->rxUUID = NimBLEUUID(rxUUID);
    this->txUUID = NimBLEUUID(txUUID);
    NimBLEDevice::init(local_name.c_str());

    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
    NimBLEDevice::setSecurityAuth(false, false, true);

    init = true;
    return init;
}

int BLEClientSerial::available() {
    // reply with data available
    return static_cast<int>(buffer.length());
}

int BLEClientSerial::peek() {
    // return first character available
    // but don't remove it from the buffer
    if ((!buffer.empty())) {
        uint8_t c = buffer[0];
        return c;
    }

    return -1;
}

bool BLEClientSerial::connect(const String &remoteName) {
    NimBLEAdvertisedDevice *device = nullptr;
    BLEScanResultsSet *bleDeviceList = discover();

    if (bleDeviceList != nullptr && bleDeviceList->getCount() > 0) {
        for (int i = 0; i < bleDeviceList->getCount(); i++) {
            NimBLEAdvertisedDevice *dev = bleDeviceList->getDevice(i);
            if (strcmp(dev->getName().c_str(), remoteName.c_str()) == 0) {
                device = dev;
            }
        }
    } else {
        return false;
    }

    if (device == nullptr) {
        return false;
    }

    disconnect();

    return connect(device->getAddress());
}

bool BLEClientSerial::connect(const NimBLEAddress &remoteAddress) {
    disconnect();

    if (NimBLEDevice::getCreatedClientCount()) {
        pClient = NimBLEDevice::getClientByPeerAddress(remoteAddress);
        if (pClient) {
            if (!pClient->connect(remoteAddress, false)) {
                return false;
            }
        } else {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    if (!pClient) {
        if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
            log_e("Max clients reached - no more connections available.");
            return false;
        }

        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks(pConnectCb, pDisconnectCb), true);
        pClient->setConnectionParams(12, 12, 0, 150);
        pClient->setConnectTimeout(5 * 1000);

        if (!pClient->connect(remoteAddress)) {
            NimBLEDevice::deleteClient(pClient);
            log_e("Couldn't connect client.");
            return false;
        }
    }

    if (!pClient->isConnected()) {
        if (!pClient->connect(remoteAddress)) {
            log_e("Failed to connect");
            return false;
        }
    }

    log_d("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(),
          pClient->getRssi());

    NimBLERemoteService *pService = pClient->getService(serviceUUID);
    if (pService) {
        pRxCharacteristic = pService->getCharacteristic(rxUUID);
        pTxCharacteristic = pService->getCharacteristic(txUUID);

        if (!pRxCharacteristic) {
            log_e("Characteristic not found %s", rxUUID.toString().c_str());
            return false;
        }

        if (!pTxCharacteristic) {
            log_e("Characteristic not found %s", txUUID.toString().c_str());
            return false;
        }

        if (pRxCharacteristic->canNotify()) {
            pRxCharacteristic->subscribe(true, [&](
                                     NimBLERemoteCharacteristic *pBLERemoteCharacteristic,
                                     uint8_t *pData,
                                     size_t length,
                                     bool isNotify) {
                                             notifyCallback(pBLERemoteCharacteristic, pData, length,
                                                            isNotify);
                                         });
        }

        return true;
    }

    log_e("Couldn't find service %s", serviceUUID.toString());

    return false;
}

bool BLEClientSerial::connected() const {
    return pClient != nullptr && pClient->isConnected();
}

bool BLEClientSerial::disconnect() {
    if (pClient != nullptr) {
        if (pClient->isConnected()) {
            pClient->disconnect();
        }
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;
        return true;
    }

    return false;
}

bool BLEClientSerial::isClosed() const {
    return pClient == nullptr || !pClient->isConnected();
}

int BLEClientSerial::read(void) {
    // read a character
    if (!buffer.empty()) {
        uint8_t c = buffer[0];
        buffer.erase(0, 1); // remove it from the buffer
        return c;
    }
    return -1;
}

size_t BLEClientSerial::write(uint8_t c) {
    if (pTxCharacteristic) {
        return pTxCharacteristic->writeValue(c, true);
    }

    return 0;
}

size_t BLEClientSerial::write(const uint8_t *buffer, size_t size) {
    size_t ws = 0;
    if (pTxCharacteristic) {
        for (int i = 0; i < size; i++) {
            ws += pTxCharacteristic->writeValue(buffer[i], false);
        }
        return ws;
    }

    return 0;
}

void BLEClientSerial::flush() {
    buffer.clear();
}

void BLEClientSerial::end() {
    disconnect();
    init = false;
}

void BLEClientSerial::onData(const BLESerialDataCb &cb) {
    pDataCb = cb;
}

void BLEClientSerial::onConnect(const BLEConnectCb &cb) {
    pConnectCb = cb;
}

void BLEClientSerial::onDisconnect(const BLEDisconnectCb &cb) {
    pDisconnectCb = cb;
}

BLEScanResultsSet *BLEClientSerial::discover(int timeout) {
    BLEScanResultsSet *bleDeviceList = getScanResults();

    if (discoverAsync([](const NimBLEAdvertisedDevice *pDevice) {
        log_d("found %s - %s %d\n", pDevice->getAddress().toString().c_str(), pDevice->getName().c_str(),
              pDevice->getRSSI());
    }, timeout)) {
        delay(timeout);
        discoverAsyncStop();

        if (bleDeviceList->getCount() > 0) {
            return bleDeviceList;
        }
    }

    return nullptr;
}

bool BLEClientSerial::discoverAsync(const BLEAdvertisedDeviceCb &cb, int timeout) {
    disconnect();
    discoverClear();

    NimBLEScan *pBLEScan = NimBLEDevice::getScan();

    if (pBLEScan != nullptr) {
        pBLEScan->stop();

        pBLEScan->setScanCallbacks(new AdvertisedDeviceCallbacks(serviceUUID, cb));
        pBLEScan->setInterval(INQ_TIME);
        pBLEScan->setWindow(INQ_TIME);
        pBLEScan->setMaxResults(0);
        pBLEScan->setActiveScan(true);
        return pBLEScan->start(timeout, false);
    }

    return false;
}

void BLEClientSerial::discoverAsyncStop() {
    if (NimBLEDevice::getScan() != nullptr) {
        NimBLEDevice::getScan()->stop();
        NimBLEDevice::getScan()->setActiveScan(false);
    }
}

void BLEClientSerial::discoverClear() {
    scanResults.clear();
}

BLEScanResultsSet *BLEClientSerial::getScanResults() {
    return &scanResults;
}
