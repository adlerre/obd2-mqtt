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

export interface DeviceClass {
    name: string;
    description: string;
    units: Array<string>;
}

export const DeviceClasses: Array<DeviceClass> = [
    {
        name: "battery",
        description: "Percentage of battery that is left",
        units: ["%"]
    },
    {
        name: "current",
        description: "Current",
        units: ["A", "mA"]
    },
    {
        name: "distance",
        description: "Generic distance",
        units: ["km", "m", "mi", "yd", "in"]
    },
    {
        name: "duration",
        description: "Time period",
        units: ["h", "min", "s", "ms"]
    },
    {
        name: "energy",
        description: "Energy",
        units: ["Wh", "kWh"]
    },
    {
        name: "power",
        description: "Power",
        units: ["W", "kW"]
    },
    {
        name: "pressure",
        description: "Pressure",
        units: ["bar", "hPa", "kPa", "psi"]
    },
    {
        name: "speed",
        description: "Generic speed",
        units: ["km/h", "mph"]
    },
    {
        name: "temperature",
        description: "Temperature",
        units: ["°C", "°F", "K"]
    },
    {
        name: "voltage",
        description: "Voltage",
        units: ["V", "mV"]
    },
    {
        name: "volume",
        description: "Generic volume",
        units: ["L", "gal"]
    },
    {
        name: "volume_storage",
        description: "Generic stored volume",
        units: ["L", "gal"]
    }
];
