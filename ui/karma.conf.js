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

// Karma configuration file, see link for more information
// https://karma-runner.github.io/1.0/config/configuration-file.html

module.exports = function (config) {
    config.set({
        basePath: "..",
        frameworks: ["jasmine", "@angular-devkit/build-angular"],
        plugins: [
            require("karma-jasmine"),
            require("karma-chrome-launcher"),
            require("karma-jasmine-html-reporter"),
            require("karma-junit-reporter"),
            require("karma-coverage"),
            require("@angular-devkit/build-angular/plugins/karma")
        ],
        client: {
            jasmine: {
                // you can add configuration options for Jasmine here
                // the possible options are listed at https://jasmine.github.io/api/edge/Configuration.html
                // for example, you can disable the random execution with `random: false`
                // or set a specific seed with `seed: 4321`
            },
            clearContext: false // leave Jasmine Spec Runner output visible in browser
        },
        jasmineHtmlReporter: {
            suppressAll: true // removes the duplicated traces
        },
        coverageReporter: {
            dir: require("path").join(__dirname, "../.results/coverage/"),
            subdir: ".",
            reporters: [
                {type: "cobertura"},
                {type: "html"},
                {type: "text-summary"}
            ]
        },
        junitReporter: {
            outputDir: require("path").join(__dirname, "../.results/junit/"),
        },
        reporters: ["progress", "junit", "kjhtml"],
        browsers: ["ChromeCI"],
        customLaunchers: {
            ChromeCI: {
                base: "Chrome",
                flags: ["--no-sandbox", "--disable-search-engine-choice-screen"],
            },
            ChromeHeadlessCI: {
                base: "ChromeHeadless",
                flags: ["--no-sandbox", "--disable-gpu", "--disable-search-engine-choice-screen"],
            },
        },
        failOnEmptyTestSuite: false,
        restartOnFileChange: true
    });
};
