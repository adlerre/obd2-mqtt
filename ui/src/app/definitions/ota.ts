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

import { Subject } from "rxjs";

export enum OTAMode {
    FIRMWARE = "fw",
    LITTLE_FS = "fs"
}

export interface FileItem {
    file: File | any;
    mode?: OTAMode;
    progressEvent?: Subject<number>;
    progress?: number;
    complete: boolean;
    processing: boolean;
    error: boolean;
    errorMsg?: string;
    created?: number;
    started?: number;
    completed?: number;
}
