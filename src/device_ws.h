// This program is free software; you can use it, redistribute it
// and / or modify it under the terms of the GNU General Public License
// (GPL) as published by the Free Software Foundation; either version 3
// of the License or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in a file called gpl.txt or license.txt.
// If not, write to the Free Software Foundation Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
#ifndef DEVICE_WS_H
#define DEVICE_WS_H

#pragma once

#if defined(WS_A7670E)
#define MODEM_BAUDRATE                      (115200)
#define MODEM_DTR_PIN                       (25)
#define MODEM_TX_PIN                        (18)
#define MODEM_RX_PIN                        (17)
// The modem boot pin needs to follow the startup sequence.
#define BOARD_PWRKEY_PIN                    (4)
#define BOARD_ADC_PIN                       (35)
// The modem power switch must be set to HIGH for the modem to supply power.
#define BOARD_POWERON_PIN                   (12)
#define MODEM_RING_PIN                      (33)
#define MODEM_RESET_PIN                     (5)
#define BOARD_MISO_PIN                      (2)
#define BOARD_MOSI_PIN                      (15)
#define BOARD_SCK_PIN                       (14)
#define BOARD_SD_CS_PIN                     (13)
#define MODEM_RESET_LEVEL                   HIGH
#define SerialAT                            Serial1

#define MODEM_GPS_ENABLE_GPIO               (-1)

#ifndef TINY_GSM_MODEM_A7670
#define TINY_GSM_MODEM_A7670
#endif

#undef TINY_GSM_MODEM_HAS_GPS

#endif

#endif //DEVICE_WS_H
