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
const CompressionPlugin = require("compression-webpack-plugin");

let settingsJson = {};

module.exports = {
    devServer: {
        setupMiddlewares: (middlewares, devServer) => {
            if (!devServer) {
                throw new Error("webpack-dev-server is not defined");
            }

            devServer.app.get("/api/wifi", (_, response) => {
                const wifi = {
                    SSID: "Test AP",
                    ip: "127.0.0.1",
                    mac: "11:22:33:44:af:fe"
                };
                response.json(wifi);
            });

            devServer.app.get("/api/modem", (_, response) => {
                const modem = {
                    name: "Test Modem",
                    signalQuality: "30",
                    ip: "123.40.5.1",
                    IMEI: "5654646464646546465654645",
                    IMSI: "4564531645646456464434",
                    CCID: "56546456554"
                };
                response.json(modem);
            });

            devServer.app.post("/api/reboot", (request, response) => {
                if (request.query.reboot) {
                    console.log("Reboot device ;)");
                } else {
                    response.status(406);
                }
                response.send();
            });

            devServer.app.get("/api/settings", (_, response) => {
                response.json(settingsJson);
            });

            devServer.app.put("/api/settings", (request, response) => {
                if (request.body) {
                    settingsJson = JSON.parse(request.body);
                } else {
                    response.status(406);
                }
                response.send();
            });

            return middlewares;
        }
    },
    plugins: [
        new CompressionPlugin({
            filename: "[path][base].gz",
            algorithm: "gzip",
            test: /\.js$|\.css$|\.html$|\.txt$/,
        }),
    ]
};