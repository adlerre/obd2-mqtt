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
#include <Arduino.h>
#include <vector>
#include <string>
#include <regex>

/**
 * Spilt string with given delimiter.
 *
 * @param s the string
 * @param delimiter the delimiter
 * @return a vector of strings
 */
std::vector<std::string> split(const std::string &s, const std::string &delimiter);

/**
 * Parse chars to byte array with given separator, length and the base.
 *
 * @param str the chars/string
 * @param sep the separator
 * @param bytes the target byte array
 * @param maxBytes the number of bytes
 * @param base the base for calculation, e.g. 10 for decimal or 16 for hex
 */
void parseBytes(const char *str, char sep, byte *bytes, int maxBytes, int base);

/**
 * Strips chars with regex from given string.
 *
 * @param str the string
 * @param reg the regex, default <code>[^a-zA-Z0-9_-]</code>
 */
std::string stripChars(const std::string &str, std::regex reg = std::regex("[^a-zA-Z0-9_-]"));
