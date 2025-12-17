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

#pragma once

#ifndef OBD2_MQTT_DEVICE_SIM7XXX_H
#define OBD2_MQTT_DEVICE_SIM7XXX_H

#if defined(LILYGO_SIM7000G) || defined(LILYGO_SIM7070G)

    #define MODEM_DTR_PIN                       (25)
    #define MODEM_RX_PIN                        (26)
    #define MODEM_TX_PIN                        (27)

    // The modem boot pin needs to follow the startup sequence.
    #define BOARD_PWRKEY_PIN                    (4)
    #define BOARD_LED_PIN                       (12)
    #define LED_ON                              (LOW)

    #define BOARD_MISO_PIN                      (2)
    #define BOARD_MOSI_PIN                      (15)
    #define BOARD_SCK_PIN                       (14)
    #define BOARD_SD_CS_PIN                     (13)

    #define BOARD_BAT_ADC_PIN                   (35)
    #define BOARD_SOLAR_ADC_PIN                 (36)

    #define SerialAT                            Serial1

    #ifdef LILYGO_SIM7000G
        // Modem model:SIM7000G
    #ifndef TINY_GSM_MODEM_SIM7000SSL
        #define TINY_GSM_MODEM_SIM7000SSL
    #endif
    #define PRODUCT_MODEL_NAME                  "LilyGo-SIM7000G ESP32 Version"
    #endif

    #ifdef LILYGO_SIM7070G
        // Modem model:SIM7070G
    #ifndef TINY_GSM_MODEM_SIM7080
        #define TINY_GSM_MODEM_SIM7080
    #endif
    #define PRODUCT_MODEL_NAME                  "LilyGo-SIM7070G ESP32 Version"
    #endif

    // 127 is defined in GSM as the AUXVDD index
    #define MODEM_GPS_ENABLE_GPIO               (48)
    #define MODEM_GPS_ENABLE_LEVEL              (1)

    //! The following pins are for SimShield and need to be used with SimShield
    //! 以下引脚针对SimShield,需要搭配SimShield
    #define SIMSHIELD_MOSI                      (23)
    #define SIMSHIELD_MISO                      (19)
    #define SIMSHIELD_SCK                       (18)
    #define SIMSHIELD_SD_CS                     (32)
    #define SIMSHIELD_RADIO_BUSY                (39)
    #define SIMSHIELD_RADIO_CS                  (5)
    #define SIMSHIELD_RADIO_IRQ                 (34)
    #define SIMSHIELD_RADIO_RST                 (15)
    #define SIMSHIELD_RS_RX                     (13)
    #define SIMSHIELD_RS_TX                     (14)
    #define SIMSHIELD_SDA                       (21)
    #define SIMSHIELD_SCL                       (22)
    #define SerialRS485                         Serial2
#endif

#endif //OBD2_MQTT_DEVICE_SIM7XXX_H