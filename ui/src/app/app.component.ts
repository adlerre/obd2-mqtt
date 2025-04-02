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

import { Component, Inject, OnInit, ViewEncapsulation } from "@angular/core";
import { ApiService } from "./services/api.service";
import { DOCUMENT } from "@angular/common";
import { Observable } from "rxjs";

@Component({
    selector: "ui-root",
    templateUrl: "./app.component.html",
    styleUrls: ["./app.component.scss"],
    encapsulation: ViewEncapsulation.None,
    standalone: false
})
export class AppComponent implements OnInit {

    private static ATTR_COLOR_SCHEME = "colorScheme";

    private static COLOR_SCHEME_LIGHT = "light";

    private static COLOR_SCHEME_DARK = "dark";

    public otaEnabled$: Observable<boolean>;

    public colorScheme: string;

    private readonly prefersColorScheme: string;

    constructor(private $api: ApiService, @Inject(DOCUMENT) private document: Document) {
        this.otaEnabled$ = $api.otaEnabled();
        this.prefersColorScheme = window.matchMedia("(prefers-color-scheme: dark)").matches ?
            AppComponent.COLOR_SCHEME_DARK : AppComponent.COLOR_SCHEME_LIGHT;
    }

    ngOnInit(): void {
        this.getColorScheme();
        this.changeColorScheme(this.colorScheme);
    }


    rebootConfirm() {
        if (window.confirm("Are you sure you want to reboot?")) {
            this.$api.reboot().subscribe(() => {
            });
        }
    }

    switchColorScheme() {
        this.changeColorScheme(this.colorScheme === AppComponent.COLOR_SCHEME_LIGHT ?
            AppComponent.COLOR_SCHEME_DARK : AppComponent.COLOR_SCHEME_LIGHT)
    }

    changeColorScheme(scheme: string | null) {
        if (scheme) {
            if (scheme !== this.colorScheme) {
                this.setColorScheme(scheme);
            }
        } else {
            this.setColorScheme(null);
        }

        this.document.documentElement.setAttribute("data-ui-theme", scheme || this.prefersColorScheme);
    }

    private setColorScheme(scheme: string | null) {
        if (scheme) {
            window.localStorage.setItem(AppComponent.ATTR_COLOR_SCHEME, scheme);
        } else {
            window.localStorage.removeItem(AppComponent.ATTR_COLOR_SCHEME);
        }
        this.colorScheme = scheme;
    }

    private getColorScheme() {
        this.colorScheme = window.localStorage.getItem(AppComponent.ATTR_COLOR_SCHEME);
    }

}
