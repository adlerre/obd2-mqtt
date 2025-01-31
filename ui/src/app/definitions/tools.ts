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

export const stripEmptyProps = (arr: any): any => {
    for (const i in arr) {
        if (typeof arr[i] === "object") {
            if (arr[i]) {
                for (const k in arr[i]) {
                    if (typeof arr[i][k] === "string" && (arr[i][k] === "" || arr[i][k] === null)) {
                        delete arr[i][k];
                    } else if (typeof arr[i][k] === "object") {
                        arr[i][k] = stripEmptyProps(arr[i][k]);
                        if (!arr[i][k]) {
                            delete arr[i][k];
                        }
                    }
                }
            } else {
                delete arr[i];
            }
        } else if (typeof arr[i] === "string" && (arr[i] === "" || arr[i] === null)) {
            delete arr[i];
        }
    }

    return arr;
}
