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

import { BrowserModule } from "@angular/platform-browser";
import { provideHttpClient, withInterceptorsFromDi } from "@angular/common/http";
import { NgModule } from "@angular/core";
import { ReactiveFormsModule } from "@angular/forms";

import { AppComponent } from "./app.component";
import { ApiService } from "./services/api.service";
import { RouterModule } from "@angular/router";
import { DeviceInfoComponent, OTAComponent, SettingsComponent } from "./components";
import { NgbTypeaheadModule } from "@ng-bootstrap/ng-bootstrap";
import { OBDStatesComponent } from "./components/obdStates.component";
import { ToastsComponent } from "./components/toasts.component";
import { ToastService } from "./services/toast.service";

@NgModule({
    bootstrap: [AppComponent],
    declarations: [
        AppComponent,
        DeviceInfoComponent,
        OBDStatesComponent,
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
                {path: "states", component: OBDStatesComponent},
                {path: "ota", component: OTAComponent},
            ],
            {useHash: true}
        ),
        ToastsComponent
    ],
    providers: [
        provideHttpClient(withInterceptorsFromDi()),
        ApiService,
        ToastService
    ]
})
export class AppModule {
}
