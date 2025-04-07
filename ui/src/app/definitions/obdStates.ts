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

export enum OBDStateType {
    READ,
    CALC,
}

export enum ValueTypes {
    BOOL = "bool",
    FLOAT = "float",
    INT = "int"
}

export interface PID {
    service: number;
    pid: number;
    header: number;
    numResponses: number;
    numExpectedBytes: number;
    scaleFactor?: string;
    bias: number;
}

export interface ValueFormat {
    format?: string;
    func?: string;
    expr?: string;
}

export interface OBDState {
    type: OBDStateType;
    valueType: ValueTypes;
    enabled: boolean;
    visible: boolean;
    interval: number;

    name: string;
    description: string;
    icon?: string;
    unit?: string;
    deviceClass?: string;
    measurement: boolean;
    diagnostic: boolean;

    expr?: string;
    readFunc?: string;
    pid: PID;
    value: ValueFormat;
}

export interface NamedItem {
    name: string;
    description: string;
}

export const BuildInExpressionVars = ["millis"];
export const BuildInExpressionFuncs = [
    "afRatio",
    "density",
    "numDTCs"
];

export const ReadFunctions: Array<NamedItem> = [
    {
        name: "batteryVoltage",
        description: "Battery Voltage"
    }
];

export const ValueFormatFunctions: Array<NamedItem> = [
    {
        name: "toBitStr",
        description: "Format as Bit String"
    },
    {
        name: "toMiles",
        description: "Convert km to mi"
    },
    {
        name: "toGallons",
        description: "Convert L to gal"
    },
    {
        name: "toMPG",
        description: "Convert L/100km to MPG"
    }
];
