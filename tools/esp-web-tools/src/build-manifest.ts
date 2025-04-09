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
import { parse } from "ini";

const os = require("os")
const fs = require("node:fs");
const path = require("node:path");
const csv = require("csv-parser");

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

const baseDir = path.resolve(__dirname, "../../..")
const pioDir = path.resolve(__dirname, "../../../.pio/build");
const webToolsDir = path.resolve(__dirname, "..");

const deviceBins = [
    "bootloader",
    "partitions",
    "firmware",
    "littlefs"
];

const part2pname: { [key: string]: string } = {
    "boot_app0": "ota",
    "firmware": "ota_0",
    "littlefs": "spiffs"
};

const buildManifest = async (partCsv: string, manifest: Manifest) => {
    const findLine = (subType: string, results: Array<{ [key: string]: string }>): { [key: string]: string } => {
        return results.find(l => l["subtype"] === subType);
    };

    return await new Promise<Manifest>(resolve => {
        const results: Array<{ [key: string]: string }> = [];
        return fs.createReadStream(partCsv)
            .pipe(csv({
                mapHeaders: ({header, index}: {
                    header: string,
                    index: number
                }) => header.trim().replaceAll(/#\s*/g, "").toLowerCase(),
                mapValues: ({header, index, value}: { header: string, index: number, value: string }) => value.trim()
            }))
            .on("data", (data) => results.push(data))
            .on("end", () => {
                manifest.builds.forEach(b =>
                    b.parts.forEach(p => {
                        Object.keys(part2pname).forEach(pn => {
                            if (p.path.indexOf(pn) !== -1) {
                                const line = findLine(part2pname[pn], results);
                                p.offset = parseInt(line["offset"], 16);
                            }
                        })
                    })
                )
                resolve(manifest);
            });
    });
};

const getPartitions = (pio_ini: any, name: string): string => {
    let part_csv = pio_ini[name]['board_build.partitions'];

    if (!part_csv) {
        let e = pio_ini[name]["extends"];
        if (e) {
            part_csv = pio_ini[e]['board_build.partitions'];

            e = pio_ini[e]["extends"];
            if (!part_csv && e) {
                part_csv = getPartitions(pio_ini, e);
            }
        }
    }

    return part_csv;
};

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

    const pio_ini = parse(fs.readFileSync(`${baseDir}/platformio.ini`, "utf8"));
    const manifest: Manifest = JSON.parse(fs.readFileSync(`${webToolsDir}/manifest.json`, "utf8"));

    if (manifest && manifest.builds && manifest.builds.length > 0) {
        DEVICES.forEach(device => {
            const build: Build = {
                chipFamily: "ESP32",
                parts: []
            };

            const part_csv = getPartitions(pio_ini, "env:" + device);

            if (!part_csv) {
                throw new Error("No partition table was found.");
            }

            buildManifest(part_csv, JSON.parse(JSON.stringify(manifest))).then(mani => {
                const devTmpl = Object.assign({}, mani.builds[0]);

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

                const m = Object.assign({builds: [{}]}, mani);
                m.builds[0] = build;

                fs.writeFileSync(`${distDir}/manifest-${device}.json`, JSON.stringify(m, null, 2));
            });
        });
    }
} catch (err) {
    console.error(err);
}