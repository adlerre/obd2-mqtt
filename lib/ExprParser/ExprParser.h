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
#pragma once

#include <functional>
#include <map>

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

using namespace std;

enum types { DELIMITER = 1, VARIABLE, NUMBER, FUNCTION };

constexpr int NUMVARS = 26;

/**
 * This library is a modified version of math expression parser
 * presented in the book : "C++ The Complete Reference" by H.Schildt.
 *
 * It supports operators: + - * / ^ ( )
 * It supports math functions : SIN, COS, TAN, ASIN, ACOS, ATAN, SINH,
 * COSH, TANH, ASINH, ACOSH, ATANH, LN, LOG, EXP, SQRT, SQR, ROUND, INT.
 *
 * It supports variables A to Z.
 *
 * Sample:
 * <code>
 * 25 * 3 + 1.5*(-2 ^ 4 * log(30) / 3)
 * x = 3
 * y = 4
 * r = sqrt(x ^ 2 + y ^ 2)
 * t = atan(y / x)
 * </code>
 *
 * @author Hamid Soltani. (gmail: hsoltanim)
 * @author René Adler
 * @see https://csvparser.github.io/mathparser.html
 * @see https://en.wikipedia.org/wiki/Shunting_yard_algorithm
 */
class ExprParser {
    char *exp_ptr; // points to the expression
    char token[256]; // holds current token
    char tokType; // holds token's type
    double vars[NUMVARS]{}; // holds variable's values

    std::map<const char *, const std::function<double(double)>> customFunctions;

    std::function<double(const char *)> varResolveFunction = nullptr;

    static int strcicmp(char const *a, char const *b);

    char *resolveVariables(char *expression);

    void evalExp1(double &result);

    void evalExp2(double &result);

    void evalExp3(double &result);

    void evalExp4(double &result);

    void evalExp5(double &result);

    void evalExp6(double &result);

    void getToken();

public:
    ExprParser();

    ~ExprParser();

    void setVariable(char var, double value);

    double getVariable(char var) const;

    void addCustomFunction(const char *name, const std::function<double(double)> &func);

    void setCustomFunctions(const std::map<const char *, const std::function<double(double)>> &funcs);

    void setVariableResolveFunction(const std::function<double(const char *)> &func);

    double evalExp(const char *expression);

    char errormsg[64]{};
};
