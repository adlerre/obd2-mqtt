#!/usr/bin/env node
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

import mqtt from "mqtt";
import { parseArgs } from "node:util";

const mainTopic = "homeassistant/#";

const {
    values: {
        help,
        srcHostname,
        srcPort,
        srcUsername,
        srcPassword,
        dstHostname,
        dstPort,
        dstUsername,
        dstPassword
    },
} = parseArgs({
    options: {
        help: {
            type: "boolean",
            short: "h",
        },
        srcHostname: {
            type: "string"
        },
        srcPort: {
            type: "string"
        },
        srcUsername: {
            type: "string"
        },
        srcPassword: {
            type: "string"
        },
        dstHostname: {
            type: "string"
        },
        dstPort: {
            type: "string"
        },
        dstUsername: {
            type: "string"
        },
        dstPassword: {
            type: "string"
        }
    },
});

if (help) {
    console.log(
        "mqtt-proxy [ -h | --help ] " +
        " --srcHostname <hostname> [--srcPort <port> | --srcUsername <username> | --srcPassword <password>]" +
        " --dstHostname <hostname> [--dstPort <port> | --dstUsername <username> | --dstPassword <password>]"
    );
    process.exit();
}

if (!srcHostname && !dstHostname) {
    console.log("The source and destination hostname is required.");
    process.exit(1);
}

const srcOptions = {};
const dstOptions = {};

srcOptions.port = srcPort && parseInt(srcPort, 10) || 1883;
if (srcUsername && srcPassword) {
    srcOptions.username = srcUsername;
    srcOptions.password = srcPassword;
}

dstOptions.port = dstPort && parseInt(dstPort, 10) || 1883;
if (dstUsername && dstPassword) {
    dstOptions.username = dstUsername;
    dstOptions.password = dstPassword;
}

const srcClient = mqtt.connect(`mqtt://${srcHostname}`, srcOptions);
const dstClient = mqtt.connect(`mqtt://${dstHostname}`, dstOptions);

srcClient.on("connect", () => {
    process.stdout.write(`Connected to ${srcHostname} and waiting for topics ${mainTopic.replace("#", "*")}...\n`);
    srcClient.subscribe(mainTopic, (err) => {
        if (err) {
            console.error(err);
        }
    });
});
srcClient.on("disconnect", (err) => {
    process.stdout.write("\n");
});
srcClient.on("reconnect", (err) => {
    process.stdout.write("\n");
});

dstClient.on("connect", () => {
    process.stdout.write(`Connected to ${dstHostname}...\n`);
});
dstClient.on("disconnect", (err) => {
    process.stdout.write("\n");
});
dstClient.on("reconnect", (err) => {
    process.stdout.write("\n");
});

srcClient.on("message", (topic, message) => {
    if (topic.indexOf("obd2mqtt") !== -1) {
        if (dstClient.connected) {
            process.stdout.write(".");
            dstClient.publish(topic, message, {
                retain: true
            });
        }
    }
});