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

import { DTCs, ModemInfo, WifiInfo } from "../definitions";
import { of } from "rxjs";
import { DeviceInfoComponent } from "./deviceInfo.component";
import { ApiService } from "../services/api.service";
import { ComponentFixture, inject, TestBed, waitForAsync } from "@angular/core/testing";

const testWiFiInfo: WifiInfo = {
    SSID: "Test AP",
    ip: "127.0.0.1",
    mac: "11:22:33:44:af:fe"
};

const testModemInfo: ModemInfo = {
    name: "Test Modem",
    signalQuality: 30,
    ip: "123.40.5.1",
    IMEI: "5654646464646546465654645",
    IMSI: "4564531645646456464434",
    CCID: "56546456554"
};

const dtcs: DTCs = {
    dtc: ["P0195", "P0197"]
};

export class MockApiService {

    version() {
        return of("1.0.0");
    }

    wifiInfo() {
        return of<WifiInfo>(testWiFiInfo);
    }

    modemInfo() {
        return of<ModemInfo>(testModemInfo);
    }

    dtcs() {
        return of<DTCs>(dtcs);
    }

}

describe("DeviceInfoComponent", () => {
    let component: DeviceInfoComponent, fixture: ComponentFixture<DeviceInfoComponent>,
        service: ApiService;
    const getElement: (selector: string) => HTMLElement = (selector) =>
        fixture.elementRef.nativeElement.querySelector(selector);
    const getElements: (selector: string) => Array<HTMLElement> = (selector) => {
        const elms = []
        const nl = fixture.elementRef.nativeElement.querySelectorAll(selector);
        for (let i = 0, ref = elms.length = nl.length; i < ref; i++) {
            elms[i] = nl[i];
        }
        return elms;
    };

    beforeEach(
        waitForAsync(() => {
            TestBed.configureTestingModule({
                declarations: [DeviceInfoComponent],
                providers: [{provide: ApiService, useClass: MockApiService}],
                teardown: {destroyAfterEach: true},
            }).compileComponents();
        })
    );

    beforeEach(inject([ApiService], (apiService: ApiService) => {
        fixture = TestBed.createComponent(DeviceInfoComponent);
        component = fixture.componentInstance;
        service = apiService;

        fixture.detectChanges();
    }));

    it("should show infos", () => {
        expect(getElement("#deviceInfo")).toBeTruthy();

        const legends = getElements("legend");
        expect(legends.length).toBe(4);
        expect(legends[0].textContent).toBe("Firmware");
        expect(legends[1].textContent).toBe("WiFi Info");
        expect(legends[2].textContent).toBe("Modem Info");
        expect(legends[3].textContent).toBe("Dagnostic Trouble Codes");
    });
});
