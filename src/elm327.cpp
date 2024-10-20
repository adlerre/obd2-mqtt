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

#include "elm327.h"

uint32_t ELM327Extended::supportedPIDs_161_191() {
    return (uint32_t) processPID(SERVICE_01, SUPPORTED_PIDS_161_191, 1, 4);
}

float ELM327Extended::odometer() {
    return processPID(SERVICE_01, ODOMETER, 1, 4, 1.0 / 10);
}
