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

const buildDeviceButtons = (parent: HTMLElement | null) => {
    if (parent) {
        DEVICES.forEach(device => {
            const espBtn = document.createElement("esp-web-install-button");
            espBtn.classList.add("row");
            espBtn.setAttribute("manifest", `manifest-${device}.json`);

            const btn = document.createElement("button");
            btn.classList.add("btn");
            btn.classList.add("btn-primary");
            btn.setAttribute("slot", "activate");
            btn.innerText = device;
            espBtn.appendChild(btn);

            parent.append(espBtn);
        });
    }
}

setColorScheme(
    window.matchMedia("(prefers-color-scheme: dark)").matches ?
        COLOR_SCHEME_DARK : COLOR_SCHEME_LIGHT
);
buildDeviceButtons(document.querySelector("#device-select"));
