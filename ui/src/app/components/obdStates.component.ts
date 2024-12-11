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

import { Component, OnInit } from "@angular/core";
import { ApiService } from "../services/api.service";
import {
    AbstractControl,
    FormArray,
    FormControl,
    FormGroup,
    ValidationErrors,
    ValidatorFn,
    Validators
} from "@angular/forms";
import {
    BuildInExpressionFuncs,
    BuildInExpressionVars,
    DeviceClasses,
    NamedItem,
    OBDState,
    OBDStateType,
    ReadFunctions,
    ValueFormatFunctions,
    ValueTypes
} from "../definitions";
import { DomSanitizer } from "@angular/platform-browser";

export function expressionValidator(checkStates: boolean = true, allowedVariables: Array<string> = [],
                                    allowedFunctions: Array<string> = []): ValidatorFn {
    const varReg = /\$([^\s),$]+)/g;
    const funcReg = /([a-zA-Z0-9_-]+)\([^)]+\)/g

    const buildInFuncs = [
        "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh",
        "acosh", "atanh", "ln", "log", "exp", "sqrt", "sqr", "round", "int", "min", "max"
    ];

    const buildVariables = (parent: FormGroup | FormArray): Array<string> => {
        const vars = [];
        if (parent && parent instanceof FormArray) {
            for (const idx of Object.keys(parent.controls)) {
                const ctrl = parent.get(idx)?.get("name");
                if (ctrl && ctrl instanceof FormControl) {
                    const enCtrl = parent.get(idx)?.get("enabled");
                    if (enCtrl && enCtrl instanceof FormControl && enCtrl.value === true) {
                        vars.push(ctrl.value);
                    }
                }
            }
        }
        return vars;
    };

    const extractVarName = (str: string): string => {
        const idx1 = str.indexOf("$");
        const idx2 = str.indexOf(".");
        return idx1 !== -1 ? idx2 !== -1 ? str.substring(idx1 + 1, idx2) : str.substring(idx1 + 1) : str;
    };

    const extractFuncName = (str: string): string => {
        const idx = str.indexOf("(");
        return idx !== -1 ? str.substring(0, idx) : str;
    };

    const validate = (i: Array<string>, n: Array<string>): Array<string> | null => {
        let res = null;

        for (const a of i) {
            let f = false;
            for (const b of n) {
                if (a.toLowerCase() === b.toLowerCase()) {
                    f = true;
                    break
                }
            }

            if (!f) {
                if (!res) {
                    res = [];
                }
                res.push(a);
            }
        }

        return res;
    };

    const validateVar = (i: Array<string>): Array<string> | null => {
        const ap = ["pu", "lu", "ov", "a", "b", "c", "d"];
        let res = null;

        for (const a of i) {
            const idx = a.indexOf(".");
            if (idx !== -1) {
                const p = a.substring(idx + 1);
                if (ap.indexOf(p.toLowerCase()) === -1) {
                    if (!res) {
                        res = [];
                    }
                    res.push(a);
                }
            }
        }

        return res;
    };

    return (control: AbstractControl): ValidationErrors | null => {
        const value = control.value;

        if (!value) {
            return null;
        }

        let invalidVars: Array<string> | null = null;
        let invalidVarExt: Array<string> | null = null;
        let invalidFuncs: Array<string> | null = null;

        const vm: Array<string> = value.match(varReg);
        if (vm) {
            const vars = allowedVariables
                .concat(checkStates ? buildVariables(control.parent?.parent) : []);
            invalidVars = validate(vm.map(extractVarName), vars);
            invalidVarExt = validateVar(vm);
        }
        const fm: Array<string> = value.match(funcReg);
        if (fm) {
            const funcs = buildInFuncs
                .concat(allowedFunctions);
            invalidFuncs = validate(fm.map(extractFuncName), funcs);
        }

        return invalidVars !== null || invalidFuncs !== null || invalidVarExt !== null ? {
            invalidExpression: true,
            invalidVars: invalidVars,
            invalidVarExtension: invalidVarExt,
            invalidFuncs: invalidFuncs
        } : null;
    }
}

@Component({
    selector: "ui-obd-states",
    templateUrl: "./obdStates.component.html"
})
export class OBDStatesComponent implements OnInit {

    form: FormGroup;

    states: FormArray;

    downloadHref: any;

    constructor(private $api: ApiService, private sanitizer: DomSanitizer) {
        this.states = new FormArray([]);
        this.form = new FormGroup({states: this.states});
    }

    ngOnInit(): void {
        this.$api.states().subscribe(states => {
            if (states && states.length !== 0) {
                states.forEach(state => this.states.push(this.buildState(state)))
            } else {
                this.states.push(this.buildState());
            }
        });
    }

    buildState(state: OBDState | undefined = null): FormGroup {
        const fg: FormGroup = new FormGroup({
            type: new FormControl<number>(0, Validators.required),
            valueType: new FormControl<string>(null, Validators.required),
            enabled: new FormControl<boolean>(true, Validators.required),
            visible: new FormControl<boolean>(true, Validators.required),
            interval: new FormControl<number>(1000, [Validators.required, Validators.min(-1)]),
            name: new FormControl<string>(null, [Validators.required, Validators.maxLength(32), Validators.pattern("[a-zA-Z0-9_]+")]),
            description: new FormControl<string>(null, [Validators.required, Validators.maxLength(256)]),
            icon: new FormControl<string>(null, Validators.maxLength(32)),
            unit: new FormControl<string>(null, Validators.maxLength(8)),
            deviceClass: new FormControl<string | null>("", Validators.maxLength(32)),
            measurement: new FormControl<boolean>(false),
            diagnostic: new FormControl<boolean>(false),
            expr: new FormControl<string | null>(null, [
                expressionValidator(true, BuildInExpressionVars, BuildInExpressionFuncs),
                Validators.maxLength(256)
            ]),
            readFunc: new FormControl<string>("", Validators.maxLength(32)),
            pid: new FormGroup({
                service: new FormControl<number>(0, [Validators.required, Validators.min(0), Validators.max(255)]),
                pid: new FormControl<number>(0, [Validators.required, Validators.min(0)]),
                numResponses: new FormControl<number>(0, [Validators.required, Validators.min(0), Validators.max(16)]),
                numExpectedBytes: new FormControl<number>(0, [Validators.required, Validators.min(0), Validators.max(16)]),
                scaleFactor: new FormControl<string | null>(null, [Validators.maxLength(256)]),
                bias: new FormControl<number>(0),
            }),
            value: new FormGroup({
                format: new FormControl<string | null>(null),
                func: new FormControl<string | null>(""),
                expr: new FormControl<string | null>(null, [expressionValidator(false, ["value"])])
            })
        });

        if (state) {
            if (state.readFunc && state.readFunc.length !== 0) {
                fg.get("pid").disable();
            }
            if (state.value && state.value.func && state.value.func.length !== 0) {
                fg.get("value.format").disable();
                fg.get("value.expr").disable();
            }

            fg.patchValue(state);
        }

        return fg;
    }

    getTypes(): Array<{ key: string, value: number }> {
        return Object.keys(OBDStateType)
            .filter((k: any) => typeof k === "string" && isNaN(parseInt(k, 10)))
            .map(key => ({
                key: key,
                value: OBDStateType[key as keyof typeof OBDStateType]
            }));
    }

    getValueTypes(): Array<{ key: string, value: string }> {
        return Object.keys(ValueTypes)
            .map(key => ({
                key: key,
                value: ValueTypes[key as keyof typeof ValueTypes]
            }));
    }

    getDeviceClasses(): Array<{ name: string, description: string }> {
        return DeviceClasses;
    }

    getDeviceClassUnits(name: string | undefined): Array<string> {
        return name && DeviceClasses.find(dc => dc.name.indexOf(name) !== -1)?.units || [];
    }

    getReadFunctions(): Array<NamedItem> {
        return ReadFunctions;
    }

    getValueFormatFunctions(): Array<NamedItem> {
        return ValueFormatFunctions;
    }

    onAddSate(index: number) {
        this.states.insert(index + 1, this.buildState());
        this.jumpTo("state-" + (index + 1));
    }

    onRemoveState(index: number) {
        this.states.removeAt(index);
    }

    onTypeChange(event: Event, index: number) {
        const sel = event.target as HTMLSelectElement;
        const ctrl = this.states.at(index).get("expr");
        if (parseInt(sel.value, 10) === OBDStateType.CALC) {
            if (!ctrl.hasValidator(Validators.required)) {
                ctrl.addValidators(Validators.required);
                ctrl.updateValueAndValidity({emitEvent: true, onlySelf: false});
            }
        } else if (ctrl.hasValidator(Validators.required)) {
            ctrl.removeValidators(Validators.required);
        }
    }

    onReadFunctionChange(event: Event, index: number) {
        const sel = event.target as HTMLSelectElement;
        if (sel.value.length !== 0) {
            this.states.at(index).get("pid").disable();
        } else {
            this.states.at(index).get("pid").enable();
        }
    }

    onValueFormatFuncChange(event: Event, index: number) {
        const sel = event.target as HTMLSelectElement;
        if (sel.value.length !== 0) {
            this.states.at(index).get("value.format").disable();
            this.states.at(index).get("value.expr").disable();
        } else {
            this.states.at(index).get("value.format").enable();
            this.states.at(index).get("value.expr").enable();
        }
    }

    generateDownload() {
        const theJSON = JSON.stringify(this.stripEmptyProps(this.states.value));
        const uri = this.sanitizer.bypassSecurityTrustUrl("data:text/json;charset=UTF-8," + encodeURIComponent(theJSON));
        this.downloadHref = uri;
    }

    onFileChange(event: Event) {
        const target: any = event.target;
        if (target.files && target.files.length) {
            const fileReader = new FileReader();
            fileReader.onload = () => {
                const json = JSON.parse(fileReader.result as string);
                if (json && json instanceof Array) {
                    this.states.clear();
                    json.forEach(state => this.states.push(this.buildState(state)))
                    setTimeout(() => this.states.updateValueAndValidity({emitEvent: true, onlySelf: true}));
                }
            }
            fileReader.readAsText(target.files[0]);
        }
    }

    onSubmit({value, valid}: { value: { states: Array<OBDState> }, valid: boolean }) {
        if (valid) {
            const states: Array<OBDState> = this.stripEmptyProps(value.states);
            this.$api.updateStates(states).subscribe({
                next: () => {
                    window.alert("States updated successfully.");
                }, error: (err) => {
                    window.alert(err.message);
                }
            });
        }
    }

    private stripEmptyProps(arr: any): any {
        for (const i in arr) {
            if (typeof arr[i] === "object") {
                if (arr[i]) {
                    for (const k in arr[i]) {
                        if (typeof arr[i][k] === "string" && (arr[i][k] === "" || arr[i][k] === null)) {
                            delete arr[i][k];
                        } else if (typeof arr[i][k] === "object") {
                            arr[i][k] = this.stripEmptyProps(arr[i][k]);
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

    private jumpTo(id: string) {
        setTimeout(() => {
            const top = document.getElementById(id)?.offsetTop;
            window.scrollTo(0, top);
        });
    }

}
