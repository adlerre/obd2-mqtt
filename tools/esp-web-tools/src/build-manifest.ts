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

const os = require("os")
const fs = require("node:fs");
const path = require("node:path");

interface Part {
    path: string;
    offset: number;
}

interface Build {
    chipFamily: string;
    parts: Array<Part>;
}

interface Manifest {
    name?: string;
    version?: string;
    funding_url?: string;
    new_install_prompt_erase?: string;
    builds?: Array<Build>;
}

const pioDir = path.resolve(__dirname, "../../../.pio/build");
const webToolsDir = path.resolve(__dirname, "..");

const deviceBins = [
    "bootloader",
    "partitions",
    "firmware",
    "littlefs"
];

try {
    const distIndex = process.argv.indexOf("--dist");
    const localDev = process.argv.indexOf("--dev") !== -1;
    let distDir: string;

    if (distIndex === -1) {
        throw new Error("No dist directory specified");
    } else {
        distDir = path.resolve(process.argv[distIndex + 1]);
        if (!fs.existsSync(distDir)) {
            fs.mkdirSync(distDir);
        }
    }

    const manifest: Manifest = JSON.parse(fs.readFileSync(`${webToolsDir}/manifest.json`, "utf8"));

    if (manifest && manifest.builds && manifest.builds.length > 0) {
        const devTmpl = manifest.builds[0];

        DEVICES.forEach(device => {
            const build: Build = {
                chipFamily: "ESP32",
                parts: []
            };

            devTmpl.parts.forEach(part => {
                let binDir: string = "";
                const dirname = path.dirname(part.path);
                const basename = path.basename(part.path);

                if (localDev) {
                    binDir = path.join(distDir, dirname, device);

                    if (!fs.existsSync(binDir)) {
                        fs.mkdirSync(binDir, {recursive: true});
                    }
                }

                if (deviceBins.indexOf(basename.replace(".bin", "")) !== -1) {
                    const p = {
                        path: `${dirname}/${device}/${basename}`,
                        offset: part.offset
                    };
                    build.parts.push(p);

                    if (localDev) {
                        fs.copyFile(
                            path.resolve(pioDir, device, basename),
                            path.resolve(binDir, basename),
                            (err: any) => {
                                if (err) throw err;
                            }
                        );
                    }
                } else {
                    const p: Part = {
                        path: `${dirname}/${device}/${basename}`,
                        offset: part.offset
                    };
                    build.parts.push(p);

                    if (localDev) {
                        fs.copyFile(
                            path.resolve(
                                os.homedir(),
                                ".platformio/packages/framework-arduinoespressif32/tools/partitions",
                                basename
                            ),
                            path.resolve(binDir, basename),
                            (err: any) => {
                                if (err) throw err;
                            }
                        );
                    }
                }
            });

            const m = Object.assign({builds: [{}]}, manifest);
            m.builds[0] = build;

            fs.writeFileSync(`${distDir}/manifest-${device}.json`, JSON.stringify(m, null, 2));

        });
    }
} catch (err) {
    console.error(err);
}