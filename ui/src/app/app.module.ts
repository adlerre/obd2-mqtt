import { BrowserModule } from "@angular/platform-browser";
import { provideHttpClient, withFetch, withInterceptorsFromDi } from "@angular/common/http";
import { NgModule } from "@angular/core";
import { ReactiveFormsModule } from "@angular/forms";

import { AppComponent } from "./app.component";
import { ApiService } from "./services/api.service";
import { RouterModule } from "@angular/router";
import { DeviceInfoComponent, OTAComponent, SettingsComponent } from "./components";
import { NgbTypeaheadModule } from "@ng-bootstrap/ng-bootstrap";

@NgModule({
    bootstrap: [AppComponent],
    declarations: [
        AppComponent,
        DeviceInfoComponent,
        OTAComponent,
        SettingsComponent
    ],
    imports: [
        BrowserModule,
        NgbTypeaheadModule,
        ReactiveFormsModule,
        RouterModule.forRoot(
            [
                {path: "", component: DeviceInfoComponent},
                {path: "settings", component: SettingsComponent},
                {path: "ota", component: OTAComponent},
            ],
            {useHash: true}
        )
    ],
    providers: [
        provideHttpClient(withInterceptorsFromDi(), withFetch()),
        ApiService
    ]
})
export class AppModule {
}
