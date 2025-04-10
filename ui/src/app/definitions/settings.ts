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

export interface GeneralSettings {
    sleepTimeout?: number;
    sleepDuration?: number;
}

export interface WiFiSettings {
    ssid?: string;
    password?: string;
}

export enum NetworkMode {
    AUTO = 2,
    GSM = 13,
    WCDMA = 14,
    LTE = 38,
}

export interface MobileSettings {
    networkMode?: NetworkMode;
    pin?: string;
    apn: string;
    username?: string;
    password?: string;
}

export enum OBD2Protocol {
    AUTOMATIC = "0",
    SAE_J1850_PWM_41_KBAUD = "1",
    SAE_J1850_PWM_10_KBAUD = "2",
    ISO_9141_5_BAUD_INIT = "3",
    ISO_14230_5_BAUD_INIT = "4",
    ISO_14230_FAST_INIT = "5",
    ISO_15765_11_BIT_500_KBAUD = "6",
    ISO_15765_29_BIT_500_KBAUD = "7",
    ISO_15765_11_BIT_250_KBAUD = "8",
    ISO_15765_29_BIT_250_KBAUD = "9",
    SAE_J1939_29_BIT_250_KBAUD = "A"
}

export interface OBD2Settings {
    name?: string;
    mac?: string;
    checkPIDSupport?: boolean;
    debug?: boolean;
    specifyNumResponses?: boolean;
    protocol?: OBD2Protocol;
}

export enum MQTTProtocol {
    MQTT = 0,
    WEBSOCKET = 1,
}

export interface MQTTSettings {
    protocol?: MQTTProtocol;
    hostname: string;
    port?: number;
    secure?: boolean;
    username?: string;
    password?: string;
    allowOffline?: boolean;
    dataInterval?: number;
    diagnosticInterval?: number;
    discoveryInterval?: number;
    locationInterval?: number;
}

export interface Settings {
    general?: GeneralSettings;
    wifi?: WiFiSettings;
    mobile?: MobileSettings;
    obd2?: OBD2Settings;
    mqtt?: MQTTSettings;
}

export const dataIntervals = [1, 3, 5];
export const diagnosticIntervals = [30, 60, 120];
export const discoveryIntervals = [300, 1800, 3600];
export const locationIntervals = [10, 30, 60];
