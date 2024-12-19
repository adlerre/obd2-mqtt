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

import { Component, inject } from "@angular/core";

import { ToastService } from "../services/toast.service";
import { NgTemplateOutlet } from "@angular/common";
import { NgbToastModule } from "@ng-bootstrap/ng-bootstrap";

@Component({
    selector: "ui-toast",
    imports: [NgbToastModule, NgTemplateOutlet],
    template: `
        @for (toast of toastService.toasts; track toast) {
            <ngb-toast
                    [class]="toast.classname"
                    [autohide]="true"
                    [delay]="toast.delay || 5000"
                    (hidden)="toastService.remove(toast)"
            >
                @if (toast.template) {
                    <ng-template [ngTemplateOutlet]="toast.template"></ng-template>
                } @else {
                    {{ toast.text }}
                }
            </ngb-toast>
        }
    `,
    host: {
        class: "toast-container p-3",
        style: "position: fixed; top: 0; right: 0; z-index: 1200"
    }
})
export class ToastsComponent {
    toastService = inject(ToastService);
}
