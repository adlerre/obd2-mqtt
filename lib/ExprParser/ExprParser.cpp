/*
 * This program is free software; you can use it, redistribute it
 * and / or modify it under the terms of the GNU General Public License
 * (GPL) as published by the Free Software Foundation; either version 3
 * of the License or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program, in a file called gpl.txt or license.txt.
 * If not, write to the Free Software Foundation Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307 USA
 */

#include "ExprParser.h"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cmath>

// Parser constructor.
ExprParser::ExprParser(): token{}, tokType(0) {
    exp_ptr = nullptr;
    for (double &var: vars) {
        var = 0.0;
    }
    errormsg[0] = '\0';
}

ExprParser::~ExprParser() = default;

int ExprParser::strcicmp(char const *a, char const *b) {
    for (;; a++, b++) {
        int d = tolower(static_cast<unsigned char>(*a)) - tolower(static_cast<unsigned char>(*b));
        if (d != 0 || !*a)
            return d;
    }
}

void ExprParser::setVariable(const char var, const double value) {
    vars[var - 'A'] = value;
}

double ExprParser::getVariable(char var) const {
    return vars[var - 'A'];
}

void ExprParser::addCustomFunction(const char *name, const std::function<double(double)> &func) {
    customFunctions.insert({name, func});
}

void ExprParser::setCustomFunctions(const map<const char *, const std::function<double(double)>> &funcs) {
    customFunctions.insert(begin(funcs), end(funcs));
}

void ExprParser::setVariableResolveFunction(const std::function<double(const char *)> &func) {
    varResolveFunction = func;
}

char *ExprParser::resolveVariables(char *expression) {
    if (expression != nullptr && varResolveFunction != nullptr) {
        char exp[strlen(expression) + 256] = {'\0'};
        char varName[33] = {'\0'};
        char var[33] = {'\0'};
        size_t pos = 0;
        size_t varPos = 0;
        size_t start = -1;
        size_t end = -1;

        while (expression[pos] != '\0') {
            if (expression[pos] == '$') {
                start = pos;
            }
            if (start != -1) {
                if (std::isspace(expression[pos]) || strchr("+-*/%^&=(),", expression[pos]) != nullptr) {
                    end = pos - 1;
                    varName[varPos] = '\0';
                }
                if (end == -1) {
                    varName[varPos] = expression[pos];
                    ++varPos;
                } else {
                    break;
                }
            } else {
                exp[pos] = expression[pos];
            }
            ++pos;
        }

        if (start != -1 && end != -1) {
            snprintf(var, 32, "%.8g", varResolveFunction(varName));

            pos = start;
            varPos = 0;
            while (var[varPos] != '\0') {
                exp[pos] = var[varPos];
                ++pos;
                ++varPos;
            }

            varPos = end + 1;
            while (expression[varPos] != '\0') {
                exp[pos] = expression[varPos];
                ++pos;
                ++varPos;
            }
            exp[pos] = '\0';

            strcpy(expression, resolveVariables(exp));
        }
    }

    return expression;
}

// Parser entry point.
double ExprParser::evalExp(const char *expression) {
    errormsg[0] = '\0';
    double result;

    if (expression != nullptr) {
        char exp[strlen(expression) + 1] = {'\0'};
        strcpy(exp, expression);
        exp_ptr = resolveVariables(exp);
        getToken();
        if (!*token) {
            strcpy(errormsg, "No Expression Present"); // no expression present
            return 0;
        }
        evalExp1(result);
        if (*token) {
            strcpy(errormsg, "Syntax Error"); // last token must be null
        }
    } else {
        strcpy(errormsg, "No Expression Present"); // no expression present
        return 0;
    }
    return result;
}

// Process an assignment.
void ExprParser::evalExp1(double &result) {
    if (tokType == VARIABLE) {
        char tempToken[80];
        // save old token
        char *t_ptr = exp_ptr;
        strcpy(tempToken, token);
        // compute the index of the variable
        int slot = *token - 'A';
        getToken();
        if (*token != '=') {
            exp_ptr = t_ptr; // return current token
            strcpy(token, tempToken); // restore old token
            tokType = VARIABLE;
        } else {
            getToken(); // get next part of exp
            evalExp2(result);
            vars[slot] = result;
            return;
        }
    }
    evalExp2(result);
}

// Add or subtract two terms.
void ExprParser::evalExp2(double &result) {
    char op;
    double temp;
    evalExp3(result);
    while ((op = *token) == '+' || op == '-') {
        getToken();
        evalExp3(temp);
        if (op == '-') {
            result = result - temp;
        } else if (op == '+') {
            result = result + temp;
        }
    }
}

// Multiply or divide two factors.
void ExprParser::evalExp3(double &result) {
    char op;
    double temp;
    evalExp4(result);
    while ((op = *token) == '*' || op == '/') {
        getToken();
        evalExp4(temp);
        if (op == '*') {
            result = result * temp;
        } else if (op == '/') {
            result = result / temp;
        }
    }
}

// Process an exponent or and.
void ExprParser::evalExp4(double &result) {
    char op;
    double temp;
    evalExp5(result);
    while ((op = *token) == '^' || op == '&') {
        getToken();
        evalExp5(temp);
        if (op == '^') {
            result = pow(result, temp);
        } else if (op == '&') {
            result = static_cast<int>(result) & static_cast<int>(temp);
        }
    }
}

// Evaluate a unary + or -.
void ExprParser::evalExp5(double &result) {
    char op = 0;
    if ((tokType == DELIMITER) && *token == '+' || *token == '-') {
        op = *token;
        getToken();
    }
    evalExp6(result);
    if (op == '-')
        result = -result;
}

// Process a function, a parenthesized expression, a value or a variable
void ExprParser::evalExp6(double &result) {
    char *err_ptr;
    const bool isfunc = (tokType == FUNCTION);
    char tempToken[80];
    if (isfunc) {
        strcpy(tempToken, token);
        getToken();
    }
    if (*token == '(') {
        getToken();
        evalExp2(result);
        if (*token != ',' && *token != ')')
            strcpy(errormsg, "Unbalanced Parentheses");
        if (isfunc) {
            if (!strcmp(tempToken, "SIN"))
                result = sin(PI / 180 * result);
            else if (!strcmp(tempToken, "COS"))
                result = cos(PI / 180 * result);
            else if (!strcmp(tempToken, "TAN"))
                result = tan(PI / 180 * result);
            else if (!strcmp(tempToken, "ASIN"))
                result = 180 / PI * asin(result);
            else if (!strcmp(tempToken, "ACOS"))
                result = 180 / PI * acos(result);
            else if (!strcmp(tempToken, "ATAN"))
                result = 180 / PI * atan(result);
            else if (!strcmp(tempToken, "SINH"))
                result = sinh(result);
            else if (!strcmp(tempToken, "COSH"))
                result = cosh(result);
            else if (!strcmp(tempToken, "TANH"))
                result = tanh(result);
            else if (!strcmp(tempToken, "ASINH"))
                result = asinh(result);
            else if (!strcmp(tempToken, "ACOSH"))
                result = acosh(result);
            else if (!strcmp(tempToken, "ATANH"))
                result = atanh(result);
            else if (!strcmp(tempToken, "LN"))
                result = log(result);
            else if (!strcmp(tempToken, "LOG"))
                result = log10(result);
            else if (!strcmp(tempToken, "EXP"))
                result = exp(result);
            else if (!strcmp(tempToken, "SQRT"))
                result = sqrt(result);
            else if (!strcmp(tempToken, "SQR"))
                result = result * result;
            else if (!strcmp(tempToken, "ROUND"))
                result = round(result);
            else if (!strcmp(tempToken, "INT"))
                result = floor(result);
            else if (*token == ',' && (!strcmp(tempToken, "MIN") || !strcmp(tempToken, "MAX"))) {
                getToken(); // get next token, should be a numeric
                const double val = strtod(token, &err_ptr);
                if (*err_ptr == '\0') {
                    result = !strcmp(tempToken, "MIN") ? result < val ? result : val : result > val ? result : val;
                    getToken();
                    if (*token != ')')
                        strcpy(errormsg, "Unbalanced Parentheses");
                } else {
                    strcpy(errormsg, "Is not a number");
                }
            } else {
                bool found = false;
                for (const auto &item: customFunctions) {
                    if (!strcicmp(tempToken, item.first)) {
                        result = item.second(result);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    strcpy(errormsg, "Unknown Function");
                }
            }
        }
        getToken();
    } else {
        switch (tokType) {
            case VARIABLE:
                result = vars[*token - 'A'];
                getToken();
                return;
            case NUMBER:
                result = strtod(token, &err_ptr);
                if (*err_ptr == '\0') {
                    getToken();
                } else {
                    strcpy(errormsg, "Is not a number");
                }
                return;
            default:
                strcpy(errormsg, "Syntax Error");
        }
    }
}

// Obtain the next token.
void ExprParser::getToken() {
    tokType = 0;
    char *temp = token;
    *temp = '\0';
    if (!*exp_ptr) {
        // at end of expression
        return;
    }
    while (isspace(*exp_ptr)) // skip over white space
        ++exp_ptr;
    if (strchr("+-*/%^&=(),", *exp_ptr)) {
        tokType = DELIMITER;
        *temp++ = *exp_ptr++; // advance to next char
    } else if (isalpha(*exp_ptr)) {
        while (!strchr(" +-/*%^&=(),\t\r", *exp_ptr) && (*exp_ptr)) {
            *temp++ = toupper(*exp_ptr++);
        }
        while (isspace(*exp_ptr)) {
            // skip over white space
            ++exp_ptr;
        }
        tokType = (*exp_ptr == '(') ? FUNCTION : VARIABLE;
    } else if (isdigit(*exp_ptr) || *exp_ptr == '.') {
        while (!strchr(" +-/*%^&=(),\t\r", *exp_ptr) && (*exp_ptr)) {
            *temp++ = toupper(*exp_ptr++);
        }
        tokType = NUMBER;
    }
    *temp = '\0';
    if ((tokType == VARIABLE) && (token[1])) {
        strcpy(errormsg, "Only first letter of variables is considered");
    }
}
