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
import { HttpClient, HttpEvent, HttpEventType, HttpHeaders, HttpRequest } from "@angular/common/http";
import { Configuration, DiscoveredDevices, ModemInfo, Settings, WifiInfo } from "../definitions";
import { distinctUntilChanged, last, map, Subject } from "rxjs";
import { OTAMode } from "../definitions/ota";

@Injectable()
export class ApiService {

    constructor(public $http: HttpClient) {
    }

    configuration() {
        return this.$http.get<Configuration>("configuration.json");
    }

    reboot() {
        return this.$http.post("/api/reboot?reboot=true", {}, {responseType: "text"});
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

    otaStart(mode: OTAMode = OTAMode.FIRMWARE) {
        return this.$http.get(`/api/ota/start?mode=${mode}`, {responseType: "text"});
    }

    otaUpload(file: File, progress?: Subject<number>) {
        const formData = new FormData();
        formData.append("file", file, file.name);

        const req = new HttpRequest(
            "POST",
            "/api/ota/upload",
            formData,
            {
                headers: new HttpHeaders(
                    {"ngsw-bypass": "true"},
                ),
                reportProgress: progress !== undefined,
                responseType: "text"
            }
        );

        return this.$http.request(req).pipe(
            distinctUntilChanged(),
            map((event: HttpEvent<any>) => {
                if (event) {
                    if (event.type === HttpEventType.UploadProgress && progress && event.total) {
                        const percentDone = Math.round(100 * event.loaded / event.total);
                        progress.next(percentDone);
                    } else if (event.type === HttpEventType.Response) {
                        if (progress) {
                            progress.next(100);
                            progress.complete();
                        }

                        return event;
                    }
                }

                return event;
            }),
            last()
        );
    }


}
