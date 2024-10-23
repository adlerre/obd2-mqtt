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

import { Component, OnInit, ViewChild } from "@angular/core";
import { ApiService } from "../services/api.service";
import { FormControl, FormGroup, Validators } from "@angular/forms";
import {
    dataIntervals,
    diagnosticIntervals,
    DiscoveredDevice,
    DiscoveredDevices,
    discoveryIntervals,
    locationIntervals,
    MeasurementSystems,
    MQTTProtocol,
    OBD2Protocol,
    Settings
} from "../definitions";
import { NgbTypeahead, NgbTypeaheadSelectItemEvent } from "@ng-bootstrap/ng-bootstrap";
import {
    catchError,
    debounceTime,
    distinctUntilChanged,
    filter,
    map,
    merge,
    Observable,
    of,
    OperatorFunction,
    Subject
} from "rxjs";

@Component({
    selector: "ui-settings",
    templateUrl: "./settings.component.html",
    styleUrls: ["./settings.component.scss"]
})
export class SettingsComponent implements OnInit {

    @ViewChild("devTypeahead", {static: true}) devTypeahead: NgbTypeahead;

    discoveredDevices: DiscoveredDevices | undefined;

    form: FormGroup;

    general: FormGroup;

    wifi: FormGroup;

    mobile: FormGroup;

    obd2: FormGroup;

    mqtt: FormGroup;

    focus$ = new Subject<string>();

    click$ = new Subject<string>();

    protected readonly dataIntervals = dataIntervals;

    protected readonly diagnosticIntervals = diagnosticIntervals;

    protected readonly discoveryIntervals = discoveryIntervals;

    protected readonly locationIntervals = locationIntervals;

    constructor(private $api: ApiService) {
        this.general = new FormGroup({
            measurementSystem: new FormControl<number>(0, Validators.required),
        });
        this.wifi = new FormGroup({
            ssid: new FormControl("", Validators.maxLength(64)),
            password: new FormControl("", [Validators.minLength(8), Validators.maxLength(32)])
        });
        this.mobile = new FormGroup({
            pin: new FormControl("", [Validators.maxLength(4), Validators.pattern("^[0-9]{4}")]),
            apn: new FormControl("", [Validators.required, Validators.maxLength(64)]),
            username: new FormControl("", Validators.maxLength(32)),
            password: new FormControl("", Validators.maxLength(32))
        });
        this.obd2 = new FormGroup({
            name: new FormControl("", Validators.maxLength(64)),
            mac: new FormControl("", Validators.pattern(/^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/)),
            checkPIDSupport: new FormControl(false),
            protocol: new FormControl(OBD2Protocol.AUTOMATIC),
        });
        this.mqtt = new FormGroup({
            protocol: new FormControl<number>(MQTTProtocol.MQTT),
            hostname: new FormControl("", [
                Validators.required,
                Validators.maxLength(64),
                Validators.pattern(
                    /^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*$/
                )
            ]),
            port: new FormControl<number>(1883, [Validators.min(1), Validators.max(65384)]),
            username: new FormControl("", Validators.maxLength(32)),
            password: new FormControl("", Validators.maxLength(32)),
            dataInterval: new FormControl<number>(1),
            diagnosticInterval: new FormControl<number>(30),
            discoveryInterval: new FormControl<number>(1800),
            locationInterval: new FormControl<number>(30)
        });

        this.form = new FormGroup({
            general: this.general,
            wifi: this.wifi,
            mobile: this.mobile,
            obd2: this.obd2,
            mqtt: this.mqtt
        });
    }

    ngOnInit(): void {
        this.$api.settings().subscribe(settings => this.form.patchValue(settings));
        this.$api.discoveredDevices()
            .pipe(catchError(() => of({} as DiscoveredDevices)))
            .subscribe(dd => this.discoveredDevices = dd);
    }

    getMeasurementSystems(): Array<{ key: string, value: number }> {
        return Object.keys(MeasurementSystems)
            .filter((k: any) => typeof k === "string" && isNaN(parseInt(k, 10)))
            .map(key => ({
                key: key,
                value: MeasurementSystems[key as keyof typeof MeasurementSystems]
            }));
    }

    getOBDProtocols(): Array<{ key: string, value: string }> {
        return Object.keys(OBD2Protocol).map(key => ({
            key: key.replaceAll("_", " "),
            value: OBD2Protocol[key as keyof typeof OBD2Protocol]
        }));
    }

    getMQTTProtocols(): Array<{ key: string, value: number }> {
        return Object.keys(MQTTProtocol)
            .filter((k: any) => typeof k === "string" && isNaN(parseInt(k, 10)))
            .map(key => ({
                key: key,
                value: MQTTProtocol[key as keyof typeof MQTTProtocol]
            }));
    }

    searchDevice: OperatorFunction<string, readonly DiscoveredDevice[]> = (text$: Observable<string>) => {
        const debouncedText$ = text$.pipe(debounceTime(200), distinctUntilChanged());
        const clicksWithClosedPopup$ = this.click$.pipe(filter(() => !this.devTypeahead.isPopupOpen()));
        const inputFocus$ = this.focus$;

        return merge(debouncedText$, inputFocus$, clicksWithClosedPopup$).pipe(
            map((term) =>
                (
                    (term === "" ?
                            this.discoveredDevices?.device :
                            (this.discoveredDevices?.device || [])
                                .filter((v) =>
                                    v.name.toLowerCase().indexOf(term.toLowerCase()) > -1 ||
                                    v.mac.toLowerCase().indexOf(term.toLowerCase()) > -1
                                )
                    ) || []
                ).slice(0, 10)
            )
        );
    }

    onSelectDevice(event: NgbTypeaheadSelectItemEvent) {
        event.preventDefault();
        if (event.item) {
            this.obd2.patchValue({"name": event.item.name, "mac": event.item.mac});
        }
    }

    onSubmit({value, valid}: { value: Settings, valid: boolean }) {
        if (valid) {
            this.$api.updateSettings(value).subscribe({
                next: () => {
                    window.alert("Settings updated successfully.");
                }, error: (err) => {
                    window.alert(err.message);
                }
            });
        }
    }

}
