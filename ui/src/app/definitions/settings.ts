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
export class WiFiSettings {
    ssid?: string;
    password?: string;
}

export class MobileSettings {
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

export class OBD2Settings {
    name?: string;
    mac?: string;
    checkPIDSupport?: boolean;
    protocol?: OBD2Protocol;
}

export class MQTTSettings {
    hostname: string;
    port?: number;
    username?: string;
    password?: string;
}

export interface Settings {
    wifi: WiFiSettings;
    mobile: MobileSettings;
    obd2: OBD2Settings;
    mqtt: MQTTSettings;
}
