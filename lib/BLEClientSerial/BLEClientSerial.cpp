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

class AdvertisedDeviceCallbacks final : public BLEAdvertisedDeviceCallbacks {
    BLEUUID serviceUUID;
    BLEAdvertisedDeviceCb callback = nullptr;

public:
    AdvertisedDeviceCallbacks(const BLEUUID &serviceFilter) {
        serviceUUID = serviceFilter;
    }

    explicit AdvertisedDeviceCallbacks(const BLEUUID &serviceFilter, const BLEAdvertisedDeviceCb &cb) {
        serviceUUID = serviceFilter;
        callback = cb;
    }

    /**
     * Called for each advertising BLE server.
     */
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        // filter for required service
        if (advertisedDevice.getServiceUUID().equals(serviceUUID) &&
            scanResults.add(advertisedDevice) && callback) {
            callback(&advertisedDevice);
        }
    }
};

class ClientCallbacks final : public BLEClientCallbacks {
    BLEConnectCb cCb;
    BLEDisconnectCb disCb;

public:
    ClientCallbacks();

    ClientCallbacks(const BLEConnectCb &connectCb, const BLEDisconnectCb &disconnectCb) {
        cCb = connectCb;
        disCb = disconnectCb;
    }

    void onConnect(BLEClient *pClient) override {
        if (cCb != nullptr) {
            cCb();
        }
    }

    void onDisconnect(BLEClient *pClient) override {
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
    return true;
}

void BLEClientSerial::notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length,
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
    this->serviceUUID = BLEUUID(serviceUUID);
    this->rxUUID = BLEUUID(rxUUID);
    this->txUUID = BLEUUID(txUUID);
    BLEDevice::init(local_name.c_str());
    return true;
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

bool BLEClientSerial::setPin(int pin) {
    auto *pSecurity = new BLESecurity();
    pSecurity->setKeySize();
    pSecurity->setStaticPIN(pin);
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    return true;
}

void BLEClientSerial::registerSecurityCallbacks(BLESecurityCallbacks *cb) {
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(cb);
}

bool BLEClientSerial::connect(const String &remoteName) {
    BLEAdvertisedDevice *device = nullptr;
    BLEScanResultsSet *bleDeviceList = discover();

    if (bleDeviceList != nullptr && bleDeviceList->getCount() > 0) {
        for (int i = 0; i < bleDeviceList->getCount(); i++) {
            BLEAdvertisedDevice *dev = bleDeviceList->getDevice(i);
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

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(pConnectCb, pDisconnectCb));
    if (pClient->connect(device)) {
        const std::map<std::string, BLERemoteService *> *pRemoteServices = pClient->getServices();
        if (pRemoteServices == nullptr) {
            log_e("No services");
            return false;
        }

        BLERemoteService *pService = pClient->getService(serviceUUID);
        if (pService) {
            pRxCharacteristic = pService->getCharacteristic(rxUUID);
            pTxCharacteristic = pService->getCharacteristic(txUUID);

            // Check and setup Rx notification
            if (pRxCharacteristic->canNotify()) {
                pRxCharacteristic->registerForNotify([&](
                                                 BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                                 uint8_t *pData,
                                                 size_t length,
                                                 bool isNotify) {
                                                         notifyCallback(pBLERemoteCharacteristic, pData, length,
                                                                        isNotify);
                                                     }, true);
            }

            return true;
        }

        log_e("can find service %s", serviceUUID_FFF0.toString());
    }

    return false;
}

bool BLEClientSerial::connect(uint8_t remoteAddress[]) {
    const BLEAddress addr = BLEAddress(remoteAddress);

    disconnect();

    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(pConnectCb, pDisconnectCb));

    if (pClient->connect(addr)) {
        const std::map<std::string, BLERemoteService *> *pRemoteServices = pClient->getServices();
        if (pRemoteServices == nullptr) {
            log_e("No services");
            return false;
        }

        BLERemoteService *pService = pClient->getService(serviceUUID);
        if (pService) {
            pRxCharacteristic = pService->getCharacteristic(rxUUID);
            pTxCharacteristic = pService->getCharacteristic(txUUID);

            // Check and setup Rx notification
            if (pRxCharacteristic->canNotify()) {
                pRxCharacteristic->registerForNotify([&](
                                                 BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                                 uint8_t *pData,
                                                 size_t length,
                                                 bool isNotify) {
                                                         notifyCallback(pBLERemoteCharacteristic, pData, length,
                                                                        isNotify);
                                                     }, true);
            }

            return true;
        }

        log_e("can find service %s", serviceUUID_FFF0.toString());
    }

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
    pTxCharacteristic->writeValue(c, true);
    delay(10);
    return 1;
}

size_t BLEClientSerial::write(const uint8_t *buffer, size_t size) {
    for (int i = 0; i < size; i++) {
        pTxCharacteristic->writeValue(buffer[i], false);
    }
    return size;
}

void BLEClientSerial::flush() {
    buffer.clear();
}

void BLEClientSerial::end() {
    disconnect();
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

    if (discoverAsync([](BLEAdvertisedDevice *pDevice) {
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

    BLEScan *pBLEScan = BLEDevice::getScan();

    if (pBLEScan != nullptr) {
        pBLEScan->stop();


        pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks(serviceUUID, cb));
        pBLEScan->setInterval(INQ_TIME);
        pBLEScan->setWindow(449);
        pBLEScan->setActiveScan(true);
        pBLEScan->start(timeout / 1000, false);

        return true;
    }

    return false;
}

void BLEClientSerial::discoverAsyncStop() {
    if (BLEDevice::getScan() != nullptr) {
        BLEDevice::getScan()->stop();
        BLEDevice::getScan()->setActiveScan(false);
    }
}

void BLEClientSerial::discoverClear() {
    scanResults.clear();
}

BLEScanResultsSet *BLEClientSerial::getScanResults() {
    return &scanResults;
}
