// Flat config (ESLint 9+). Replaces the legacy src/ui/.eslintrc.json.
import tsParser from "@typescript-eslint/parser";
import tsPlugin from "@typescript-eslint/eslint-plugin";
import ngPlugin from "@angular-eslint/eslint-plugin";
import angular from "angular-eslint";
import importPlugin from "eslint-plugin-import";

export default [
    {
        ignores: ["**/node_modules/**", "**/dist/**", "ui/src/**/*.d.ts"]
    },
    {
        files: ["ui/src/**/*.ts"],
        languageOptions: {
            parser: tsParser,
            sourceType: "module",
            parserOptions: {
                project: [
                    "ui/tsconfig.json",
                    "ui/src/tsconfig.app.json"
                ],
                tsconfigRootDir: import.meta.dirname,
                createDefaultProgram: true
            }
        },
        plugins: {
            "@typescript-eslint": tsPlugin,
            "@angular-eslint": ngPlugin,
            "import": importPlugin
        },
        processor: angular.processInlineTemplates,
        rules: {
            "@angular-eslint/component-class-suffix": "error",
            "@angular-eslint/component-selector": [
                "error",
                {
                    "type": "element",
                    "prefix": "ui",
                    "style": "kebab-case"
                }
            ],
            "@angular-eslint/directive-class-suffix": "error",
            "@angular-eslint/directive-selector": [
                "error",
                {
                    "type": "attribute",
                    "prefix": "",
                    "style": "camelCase"
                }
            ],
            "@angular-eslint/no-input-rename": "error",
            "@angular-eslint/no-output-on-prefix": "error",
            "@angular-eslint/use-pipe-transform-interface": "error",
            "@angular-eslint/use-lifecycle-interface": "error",
            "@typescript-eslint/consistent-type-definitions": "error",
            "@typescript-eslint/member-ordering": [
                "error",
                {
                    "classes": [
                        "signature",
                        "call-signature",
                        "public-static-field",
                        "protected-static-field",
                        "private-static-field",
                        "#private-static-field",
                        "public-decorated-field",
                        "protected-decorated-field",
                        "private-decorated-field",
                        "public-instance-field",
                        "protected-instance-field",
                        "private-instance-field",
                        "#private-instance-field",
                        "public-abstract-field",
                        "protected-abstract-field",
                        "public-field",
                        "protected-field",
                        "private-field",
                        "#private-field",
                        "static-field",
                        "instance-field",
                        "abstract-field",
                        "decorated-field",
                        "field",
                        "static-initialization",
                        "public-static-get",
                        "protected-static-get",
                        "private-static-get",
                        "#private-static-get",
                        "public-static-set",
                        "protected-static-set",
                        "private-static-set",
                        "#private-static-set",
                        "public-static-method",
                        "protected-static-method",
                        "private-static-method",
                        "#private-static-method",
                        "public-constructor",
                        "protected-constructor",
                        "private-constructor",
                        "constructor",
                        "public-decorated-get",
                        "protected-decorated-get",
                        "private-decorated-get",
                        "public-instance-get",
                        "protected-instance-get",
                        "private-instance-get",
                        "#private-instance-get",
                        "public-abstract-get",
                        "protected-abstract-get",
                        "public-get",
                        "protected-get",
                        "private-get",
                        "#private-get",
                        "static-get",
                        "instance-get",
                        "abstract-get",
                        "decorated-get",
                        "get",
                        "public-decorated-set",
                        "protected-decorated-set",
                        "private-decorated-set",
                        "public-instance-set",
                        "protected-instance-set",
                        "private-instance-set",
                        "#private-instance-set",
                        "public-abstract-set",
                        "protected-abstract-set",
                        "public-set",
                        "protected-set",
                        "private-set",
                        "#private-set",
                        "static-set",
                        "instance-set",
                        "abstract-set",
                        "decorated-set",
                        "set",
                        "public-decorated-method",
                        "protected-decorated-method",
                        "private-decorated-method",
                        "public-instance-method",
                        "protected-instance-method",
                        "private-instance-method",
                        "#private-instance-method",
                        "public-abstract-method",
                        "protected-abstract-method",
                        "public-method",
                        "protected-method",
                        "private-method",
                        "#private-method",
                        "static-method",
                        "instance-method",
                        "abstract-method",
                        "decorated-method",
                        "method"
                    ]
                }
            ],
            "@typescript-eslint/naming-convention": [
                "error",
                {
                    "selector": "variable",
                    "format": [
                        "camelCase",
                        "PascalCase",
                        "UPPER_CASE"
                    ],
                    "leadingUnderscore": "forbid",
                    "trailingUnderscore": "forbid"
                }
            ],
            "@typescript-eslint/no-empty-interface": "error",
            "@typescript-eslint/no-inferrable-types": [
                "error",
                {
                    "ignoreParameters": true
                }
            ],
            "@typescript-eslint/no-misused-new": "error",
            "@typescript-eslint/no-non-null-assertion": "error",
            "@typescript-eslint/no-shadow": [
                "error",
                {
                    "hoist": "all"
                }
            ],
            "@typescript-eslint/no-unused-expressions": "error",
            "@typescript-eslint/prefer-function-type": "error",
            "@typescript-eslint/unified-signatures": "error",
            "arrow-body-style": "error",
            "brace-style": [
                "error",
                "1tbs"
            ],
            "constructor-super": "error",
            "curly": "error",
            "eol-last": "error",
            "eqeqeq": [
                "error",
                "smart"
            ],
            "guard-for-in": "error",
            "import/no-deprecated": "warn",
            "max-len": [
                "error",
                {
                    "code": 140
                }
            ],
            "no-caller": "error",
            "no-console": [
                "error",
                {
                    "allow": [
                        "log",
                        "warn",
                        "dir",
                        "timeLog",
                        "assert",
                        "clear",
                        "count",
                        "countReset",
                        "group",
                        "groupEnd",
                        "table",
                        "dirxml",
                        "error",
                        "groupCollapsed",
                        "Console",
                        "profile",
                        "profileEnd",
                        "timeStamp",
                        "context",
                        "createTask"
                    ]
                }
            ],
            "no-debugger": "error",
            "no-eval": "error",
            "no-fallthrough": "error",
            "no-new-wrappers": "error",
            "no-restricted-imports": [
                "error",
                "rxjs/Rx"
            ],
            "no-throw-literal": "error",
            "no-trailing-spaces": "error",
            "no-undef-init": "error",
            "no-unused-labels": "error",
            "no-var": "error",
            "prefer-const": "error",
            "radix": "error",
            "spaced-comment": [
                "error",
                "always",
                {
                    "markers": [
                        "/"
                    ]
                }
            ]
        }
    },
    // restrict the template configs to HTML, otherwise they may also apply to *.ts and override the TS parser
    ...[...angular.configs.templateRecommended, ...angular.configs.templateAccessibility]
        .map((c) => ({...c, files: ["ui/src/**/*.html"]})),
    {
        files: ["ui/src/**/*.html"],
        rules: {
            "@angular-eslint/template/alt-text": "error",
            "@angular-eslint/template/click-events-have-key-events": "error",
            "@angular-eslint/template/elements-content": "error",
            "@angular-eslint/template/label-has-associated-control": "error",
            "@angular-eslint/template/mouse-events-have-key-events": "error",
            "@angular-eslint/template/no-autofocus": "error",
            "@angular-eslint/template/no-distracting-elements": "error",
            "@angular-eslint/template/no-positive-tabindex": "error",
            "@angular-eslint/template/table-scope": "error",
            "@angular-eslint/template/valid-aria": "error"
        }
    }
];
