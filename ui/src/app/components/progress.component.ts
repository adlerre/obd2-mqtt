/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, in a file called gpl.txt or license.txt.
 * If not, write to the Free Software Foundation Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */

import { Component, input } from "@angular/core";

@Component({
    selector: "ui-progress",
    template: `
        <div class="progress-bar progress-bar-striped text-white"
             role="progressbar"
             [class]="'bg-' + type()"
             [class.progress-bar-animated]="animated()"
             [style.width.%]="value()"
             [attr.aria-valuenow]="value()"
             aria-valuemin="0"
             aria-valuemax="100">{{ value() }}%
        </div>`,
    host: {class: "progress"},
})
export class ProgressComponent {
    readonly value = input(0);
    readonly type = input("info");
    readonly animated = input(false);
}
