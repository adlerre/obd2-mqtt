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

import { Component, OnInit } from "@angular/core";
import { DTCs, ModemInfo, WifiInfo } from "../definitions";
import { ApiService } from "../services/api.service";

@Component({
    selector: "ui-device-info",
    templateUrl: "./deviceInfo.component.html",
    standalone: false
})
export class DeviceInfoComponent implements OnInit {

    version: string | undefined;

    wifiInfo: WifiInfo | undefined;

    modemInfo: ModemInfo | undefined;

    dtcs: DTCs | undefined;

    constructor(private $api: ApiService) {
    }

    ngOnInit(): void {
        this.$api.version().subscribe(ver => this.version = ver);
        this.$api.wifiInfo().subscribe((wi) => this.wifiInfo = wi);
        this.$api.modemInfo().subscribe((mi) => this.modemInfo = mi);
        this.$api.dtcs().subscribe((d: DTCs) => this.dtcs = d);
    }

    calcRSSI(signalQuality: number): number {
        if (signalQuality > 0 && signalQuality <= 32) {
            return (111 - signalQuality * 2 - 2) * -1;
        }

        return signalQuality;
    }

}
