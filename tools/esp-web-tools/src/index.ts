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

import { DEVICES } from "./devices";

require("./index.scss");

const COLOR_SCHEME_LIGHT = "light";
const COLOR_SCHEME_DARK = "dark";

const setColorScheme = (scheme: string) => {
    document.documentElement.setAttribute("data-ui-theme", scheme);
}

const buildDeviceButton = (device: string) => {
    const espBtn = document.createElement("esp-web-install-button");
    espBtn.classList.add("row");
    espBtn.classList.add("px-1");
    espBtn.setAttribute("manifest", `manifest-${device}.json`);

    const btn = document.createElement("button");
    btn.classList.add("btn");
    btn.classList.add("btn-primary");
    btn.setAttribute("slot", "activate");
    btn.innerText = device;
    espBtn.appendChild(btn);

    return espBtn;
}

const buildDeviceButtons = (parent: HTMLElement | null) => {
    if (parent) {
        const row = document.createElement("div");
        row.classList.add("row");
        row.classList.add("m-1");

        [
            (dev: string) => dev.indexOf("_BLE") === -1,
            (dev: string) => dev.indexOf("_BLE") !== -1
        ].forEach(filter => {
            const col = document.createElement("div");
            col.classList.add("col-sm-6");

            DEVICES.filter(filter).forEach(device => {
                col.append(buildDeviceButton(device));
            });

            row.append(col);

        });

        parent.append(row);
    }
}

setColorScheme(
    window.matchMedia("(prefers-color-scheme: dark)").matches ?
        COLOR_SCHEME_DARK : COLOR_SCHEME_LIGHT
);
buildDeviceButtons(document.querySelector("#device-select"));
