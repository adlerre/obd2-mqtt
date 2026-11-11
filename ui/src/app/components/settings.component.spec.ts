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

import { Configuration, Settings } from "../definitions";
import { ApiService, ToastService } from "../services";
import { ComponentFixture, TestBed, waitForAsync } from "@angular/core/testing";
import { SettingsComponent } from "./settings.component";
import { ReactiveFormsModule } from "@angular/forms";
import { TypeaheadDirective } from "./typeahead.directive";

const testSettings: Settings = {
    wifi: {
        ssid: "Test AP"
    }
};

export class MockApiService {

    configuration() {
        return Promise.resolve({deviceType: "T-A7670X"} as Configuration);
    }

    settings() {
        return Promise.resolve(testSettings);
    }

    canDeepSleep() {
        return Promise.resolve({canDeepSleep: true});
    }

    updateSettings(settings: Settings) {
        expect(settings.mobile?.apn).toBe("apn.test.provider");
        expect(settings.mqtt?.hostname).toBe("broker.obd2-mqtt.test");
        return Promise.resolve(settings);
    }

    discoveredDevices() {
        return Promise.resolve({"device": [{"name": "OBDII", "mac": "11:22:de:ad:be:ef"}]});
    }

}

describe("SettingsComponent", () => {
    let component: SettingsComponent, fixture: ComponentFixture<SettingsComponent>, service: ApiService,
        toast: ToastService;
    const getElement: (selector: string) => HTMLElement = (selector) =>
        fixture.elementRef.nativeElement.querySelector(selector);

    beforeEach(
        waitForAsync(() => {
            TestBed.configureTestingModule({
                declarations: [SettingsComponent],
                imports: [TypeaheadDirective, ReactiveFormsModule],
                providers: [{provide: ApiService, useClass: MockApiService}, ToastService],
                teardown: {destroyAfterEach: true},
            }).compileComponents();
        })
    );

    beforeEach(async () => {
        service = TestBed.inject(ApiService);
        toast = TestBed.inject(ToastService);
        fixture = TestBed.createComponent(SettingsComponent);
        component = fixture.componentInstance;

        fixture.detectChanges();
        await fixture.whenStable();
        fixture.detectChanges();
    });

    it("should submit be disabled", () => {
        expect(getElement("#settings")).toBeTruthy();

        expect((<HTMLInputElement>getElement("input[type='submit']")).disabled).toBeTrue();
    });

    it("should submit be enabled", () => {
        expect(getElement("#settings")).toBeTruthy();

        const apnInput = component.mobile.get("apn");
        expect(apnInput).toBeTruthy();
        apnInput?.setValue("apn.test.provider");

        const mqttHostnameInput = component.mqtt.get("hostname");
        expect(mqttHostnameInput).toBeTruthy();
        mqttHostnameInput?.setValue("broker.obd2-mqtt.test");

        expect(component.form.valid).toBeTruthy();

        fixture.detectChanges();

        expect((<HTMLInputElement>getElement("input[type='submit']")).disabled).toBeFalse();
    });

    it("should update done", async () => {
        const spyToast = spyOn(toast, "show");

        expect(getElement("#settings")).toBeTruthy();

        const apnInput = component.mobile.get("apn");
        expect(apnInput).toBeTruthy();
        apnInput?.setValue("apn.test.provider");

        expect(component.form.valid).toBeFalsy();

        const devInput = component.obd2.get("name");
        expect(devInput).toBeTruthy();
        devInput?.setValue("OBDII");

        fixture.detectChanges();

        const mqttHostnameInput = component.mqtt.get("hostname");
        expect(mqttHostnameInput).toBeTruthy();
        mqttHostnameInput?.setValue("broker.obd2-mqtt.test");

        expect(component.form.valid).toBeTruthy();

        fixture.detectChanges();

        const submitBtn = <HTMLInputElement>getElement("input[type='submit']");
        expect(submitBtn.disabled).toBeFalse();
        submitBtn?.click();
        await fixture.whenStable();

        expect(spyToast).toHaveBeenCalledWith({
            text: "Settings updated successfully.",
            classname: "bg-success text-light",
            delay: 10000
        });
    });

});
