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
    Configuration,
    dataIntervals,
    diagnosticIntervals,
    DiscoveredDevice,
    DiscoveredDevices,
    discoveryIntervals,
    locationIntervals,
    MQTTProtocol,
    NetworkMode,
    OBD2Protocol,
    Settings,
    stripEmptyProps
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
import { ToastService } from "../services/toast.service";
import { DomSanitizer } from "@angular/platform-browser";

@Component({
    selector: "ui-settings",
    templateUrl: "./settings.component.html",
    styleUrls: ["./settings.component.scss"],
    standalone: false
})
export class SettingsComponent implements OnInit {

    private static DEVICE_WITH_NETWORK_MODE = [
        "T-A7670X",
        "T-A7670X_BLE",
        "T-A7670X-NO-GPS",
        "T-A7670X-NO-GPS_BLE",
        "T-A7670X-GPS-SHIELD",
        "T-A7670X-GPS-SHIELD_BLE",
        "T-Call-A7670X-V1-0",
        "T-Call-A7670X-V1-0_BLE",
        "T-Call-A7670X-V1-1",
        "T-Call-A7670X-V1-1_BLE",
        "T-A7608X",
        "T-A7608X_BLE"
    ];

    @ViewChild("devTypeahead", {static: true}) devTypeahead: NgbTypeahead;

    configuration: Configuration | undefined;

    discoveredDevices: DiscoveredDevices | undefined;

    hasBattery: boolean | undefined;

    form: FormGroup;

    general: FormGroup;

    wifi: FormGroup;

    mobile: FormGroup;

    obd2: FormGroup;

    mqtt: FormGroup;

    focus$ = new Subject<string>();

    click$ = new Subject<string>();

    downloadHref: any;

    protected readonly dataIntervals = dataIntervals;

    protected readonly diagnosticIntervals = diagnosticIntervals;

    protected readonly discoveryIntervals = discoveryIntervals;

    protected readonly locationIntervals = locationIntervals;

    constructor(private $api: ApiService, private sanitizer: DomSanitizer, private toast: ToastService) {
        this.general = new FormGroup({
            sleepTimeout: new FormControl<number>(5 * 60, Validators.min(60)),
            sleepDuration: new FormControl<number>(60 * 60, Validators.min(300)),
        });
        this.wifi = new FormGroup({
            ssid: new FormControl("", Validators.maxLength(64)),
            password: new FormControl("", [Validators.minLength(8), Validators.maxLength(32)])
        });
        this.mobile = new FormGroup({
            networkMode: new FormControl<number>(2, Validators.required),
            pin: new FormControl("", [Validators.maxLength(4), Validators.pattern("^[0-9]{4}")]),
            apn: new FormControl("", [Validators.required, Validators.maxLength(64)]),
            username: new FormControl("", Validators.maxLength(32)),
            password: new FormControl("", Validators.maxLength(32))
        });
        this.obd2 = new FormGroup({
            name: new FormControl("", Validators.maxLength(64)),
            mac: new FormControl("", Validators.pattern(/^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/)),
            checkPIDSupport: new FormControl<boolean>(false),
            debug: new FormControl<boolean>(false),
            specifyNumResponses: new FormControl<boolean>(true),
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
            secure: new FormControl<boolean>(false),
            username: new FormControl("", Validators.maxLength(32)),
            password: new FormControl("", Validators.maxLength(32)),
            allowOffline: new FormControl<boolean>(false),
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
        this.$api.configuration().subscribe((configuration: Configuration) => this.configuration = configuration);
        this.$api.hasBattery().subscribe(res => this.hasBattery = res.hasBattery);
        this.$api.settings().subscribe(settings => this.form.patchValue(settings));
        this.$api.discoveredDevices()
            .pipe(catchError(() => of({} as DiscoveredDevices)))
            .subscribe(dd => this.discoveredDevices = dd);
    }

    isNetworkModeAllowed(): boolean {
        return this.configuration &&
            SettingsComponent.DEVICE_WITH_NETWORK_MODE.indexOf(this.configuration.deviceType) !== -1 || false;
    }

    getNetworkModes(): Array<{ key: string, value: number }> {
        return Object.keys(NetworkMode)
            .filter((k: any) => typeof k === "string" && isNaN(parseInt(k, 10)))
            .map(key => ({
                key: key,
                value: NetworkMode[key as keyof typeof NetworkMode]
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

    generateDownload() {
        const theJSON = JSON.stringify(stripEmptyProps(this.fixJson(this.form.value)));
        this.downloadHref = this.sanitizer
            .bypassSecurityTrustUrl("data:text/json;charset=UTF-8," + encodeURIComponent(theJSON));
    }

    onFileChange(event: Event) {
        const target: any = event.target;
        if (target.files && target.files.length) {
            const fileReader = new FileReader();
            fileReader.onload = () => {
                const json = JSON.parse(fileReader.result as string);
                if (json) {
                    this.form.patchValue(json);
                    setTimeout(() => this.form.updateValueAndValidity({emitEvent: true, onlySelf: true}));
                }
            }
            fileReader.readAsText(target.files[0]);
        }
    }

    onSubmit({value, valid}: { value: Settings, valid: boolean }) {
        if (valid) {
            this.$api.updateSettings(this.fixJson(value)).subscribe({
                next: () => {
                    this.toast.show({
                        text: "Settings updated successfully.",
                        classname: "bg-success text-light",
                        delay: 10000
                    });
                }, error: (err) => {
                    this.toast.show({
                        text: err.message,
                        classname: "bg-danger text-light",
                        delay: 10000
                    });
                }
            });
        }
    }

    private fixJson(input: any): Settings {
        const res = Object.assign({}, input);
        res.mqtt.dataInterval = typeof input.mqtt.dataInterval == "string" ?
            parseInt(input.mqtt.dataInterval, 10) : input.mqtt.dataInterval;
        res.mqtt.discoveryInterval = typeof input.mqtt.discoveryInterval == "string" ?
            parseInt(input.mqtt.discoveryInterval, 10) : input.mqtt.discoveryInterval;
        res.mqtt.diagnosticInterval = typeof input.mqtt.diagnosticInterval == "string" ?
            parseInt(input.mqtt.diagnosticInterval, 10) : input.mqtt.diagnosticInterval;
        res.mqtt.locationInterval = typeof input.mqtt.locationInterval == "string" ?
            parseInt(input.mqtt.locationInterval, 10) : input.mqtt.locationInterval;
        return res;
    }

}
