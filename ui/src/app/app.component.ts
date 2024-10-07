import { Component, ViewEncapsulation } from "@angular/core";
import { ApiService } from "./services/api.service";

@Component({
    selector: "ui-root",
    templateUrl: "./app.component.html",
    styleUrls: ["./app.component.scss"],
    encapsulation: ViewEncapsulation.None
})
export class AppComponent {

    constructor(private $api: ApiService) {
    }

    rebootConfirm() {
        if (window.confirm("Are you sure you want to reboot?")) {
            this.$api.reboot().subscribe(() => {
            });
        }
    }
}
