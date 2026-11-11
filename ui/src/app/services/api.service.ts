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

import { Injectable } from "@angular/core";
import {
    Configuration,
    DiscoveredDevices,
    DTCs,
    ModemInfo,
    OBDState,
    OTAMode,
    Settings,
    WifiInfo
} from "../definitions";

function httpError(url: string, status: number, statusText: string, error: string) {
    return Object.assign(
        new Error(`Http failure response for ${url}: ${status} ${statusText}`),
        {status, error},
    );
}

async function http(url: string, method = "GET", body?: unknown): Promise<string> {
    const res = await fetch(url, {
        method,
        body: body === undefined ? undefined : JSON.stringify(body),
        headers: body === undefined ? undefined : {"Content-Type": "application/json"},
    });
    const text = await res.text();
    if (!res.ok) {
        throw httpError(url, res.status, res.statusText, text);
    }
    return text;
}

async function json<T>(url: string, method?: string, body?: unknown): Promise<T | null> {
    const text = await http(url, method, body);
    return text ? JSON.parse(text) : null;
}

@Injectable()
export class ApiService {

    configuration() {
        return json<Configuration>("configuration.json");
    }

    reboot() {
        return http("/api/reboot?reboot=true", "POST", {});
    }

    version() {
        return http("/api/version");
    }

    wifiInfo() {
        return json<WifiInfo>("/api/wifi");
    }

    modemInfo() {
        return json<ModemInfo>("/api/modem");
    }

    dtcs() {
        return json<DTCs>("/api/DTCs");
    }

    settings() {
        return json<Settings>("/api/settings");
    }

    updateSettings(settings: Settings) {
        return json<Settings>("/api/settings", "PUT", settings);
    }

    states() {
        return json<Array<OBDState>>("/api/states");
    }

    canDeepSleep() {
        return json<{ "canDeepSleep": boolean }>("/api/canDeepSleep");
    }

    updateStates(states: Array<OBDState>) {
        return json<Array<OBDState>>("/api/states", "PUT", states);
    }

    discoveredDevices() {
        return json<DiscoveredDevices>("/api/discoveredDevices");
    }

    otaEnabled() {
        return fetch("/api/ota/status").then((res) => res.status === 200, () => false);
    }

    otaStart(mode: OTAMode = OTAMode.FIRMWARE) {
        return http(`/api/ota/start?mode=${mode}`);
    }

    otaUpload(file: File, onProgress?: (percent: number) => void) {
        return new Promise<{ status: number }>((resolve, reject) => {
            const url = "/api/ota/upload";
            const body = new FormData();
            body.append("file", file, file.name);

            const xhr = new XMLHttpRequest();
            xhr.open("POST", url);
            xhr.upload.onprogress = (e) => {
                if (e.lengthComputable) {
                    onProgress?.(Math.round(100 * e.loaded / e.total));
                }
            };
            xhr.onerror = () => reject(httpError(url, 0, "Unknown Error", xhr.responseText));
            xhr.onload = () => {
                if (xhr.status >= 200 && xhr.status < 300) {
                    onProgress?.(100);
                    resolve({status: xhr.status});
                } else {
                    reject(httpError(url, xhr.status, xhr.statusText, xhr.responseText));
                }
            };
            xhr.send(body);
        });
    }

}
