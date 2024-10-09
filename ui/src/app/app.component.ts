import { Component, Inject, OnInit, ViewEncapsulation } from "@angular/core";
import { ApiService } from "./services/api.service";
import { DOCUMENT } from "@angular/common";

@Component({
    selector: "ui-root",
    templateUrl: "./app.component.html",
    styleUrls: ["./app.component.scss"],
    encapsulation: ViewEncapsulation.None
})
export class AppComponent implements OnInit {

    private static ATTR_COLOR_SCHEME = "colorScheme";

    private static COLOR_SCHEME_LIGHT = "light";

    private static COLOR_SCHEME_DARK = "dark";

    public colorScheme: string;

    private readonly prefersColorScheme: string;

    constructor(private $api: ApiService, @Inject(DOCUMENT) private document: Document) {
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
