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
import { ApiService } from "../services/api.service";
import { FormControl, FormGroup, Validators } from "@angular/forms";
import { OBD2Protocol, Settings } from "../definitions";

@Component({
    selector: "ui-settings",
    templateUrl: "./settings.component.html"
})
export class SettingsComponent implements OnInit {

    protocols = Object.values(OBD2Protocol);

    form: FormGroup;

    wifi: FormGroup;

    mobile: FormGroup;

    obd2: FormGroup;

    mqtt: FormGroup;

    constructor(private $api: ApiService) {
        this.wifi = new FormGroup({
            ssid: new FormControl("", Validators.maxLength(64)),
            password: new FormControl("", [Validators.minLength(8), Validators.maxLength(32)])
        });
        this.mobile = new FormGroup({
            pin: new FormControl("", Validators.maxLength(4)),
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
            hostname: new FormControl("", [
                Validators.required,
                Validators.maxLength(64),
                Validators.pattern(
                    /^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*$/
                )
            ]),
            port: new FormControl("", [Validators.min(1), Validators.max(65384)]),
            username: new FormControl("", Validators.maxLength(32)),
            password: new FormControl("", Validators.maxLength(32))
        });

        this.form = new FormGroup({
            wifi: this.wifi,
            mobile: this.mobile,
            obd2: this.obd2,
            mqtt: this.mqtt
        });
    }

    ngOnInit(): void {
        this.$api.settings().subscribe(settings => this.form.patchValue(settings));
    }

    getProtocols(): Array<{ key: string, value: string }> {
        return Object.keys(OBD2Protocol).map(key => ({key: key.replaceAll("_", " "), value: OBD2Protocol[key]}));
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
