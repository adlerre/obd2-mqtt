import { BrowserModule } from "@angular/platform-browser";
import { provideHttpClient, withInterceptorsFromDi } from "@angular/common/http";
import { NgModule } from "@angular/core";
import { ReactiveFormsModule } from "@angular/forms";

import { AppComponent } from "./app.component";
import { ApiService } from "./services/api.service";
import { RouterModule } from "@angular/router";
import { DeviceInfoComponent } from "./components/deviceInfo.component";
import { SettingsComponent } from "./components/settings.component";

@NgModule({
    bootstrap: [AppComponent],
    declarations: [
        AppComponent,
        DeviceInfoComponent,
        SettingsComponent
    ],
    imports: [
        BrowserModule,
        ReactiveFormsModule,
        RouterModule.forRoot(
            [
                {path: "", component: DeviceInfoComponent},
                {path: "settings", component: SettingsComponent},
            ],
            {useHash: true}
        )
    ],
    providers: [
        provideHttpClient(withInterceptorsFromDi()),
        ApiService
    ]
})
export class AppModule {
}
