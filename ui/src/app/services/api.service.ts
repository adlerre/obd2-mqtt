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
import { HttpClient } from "@angular/common/http";
import { DiscoveredDevices, ModemInfo, Settings, WifiInfo } from "../definitions";

@Injectable()
export class ApiService {

    constructor(public $http: HttpClient) {
    }

    reboot() {
        return this.$http.post("/api/reboot?reboot=true", {});
    }

    wifiInfo() {
        return this.$http.get<WifiInfo>("/api/wifi");
    }

    modemInfo() {
        return this.$http.get<ModemInfo>("/api/modem");
    }

    settings() {
        return this.$http.get<Settings>("/api/settings");
    }

    updateSettings(settings: Settings) {
        return this.$http.put<Settings>("/api/settings", settings);
    }

    discoveredDevices() {
        return this.$http.get<DiscoveredDevices>("/api/discoveredDevices");
    }

}
