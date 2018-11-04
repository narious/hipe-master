/*  Copyright (c) 2016 Daniel Kos, General Development Systems

    This file is part of Hipe.

    Hipe is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Hipe is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Hipe.  If not, see <http://www.gnu.org/licenses/>.
*/

/// Sanitation class.
/// PURPOSE: provides static utility functions and resources for sanitising user input.

#ifndef SANITATION_H
#define SANITATION_H

#include <string>
#include <set>

class Sanitation
{
private:
    //sets to store whitelists of allowed HTML tags and attributes.
    static std::set<std::string> tagWhitelist;
    static std::set<std::string> attrWhitelist;
public:
    static void init();
    //initialises whitelists and related sanitation data.
    //Call this before using any of the sanitisation functions in this class.

    static std::string sanitisePlainText(std::string input, bool convertLayout=false);
    //convert HTML syntactical characters in input into harmless escaped character entities.

    static std::string toBase64(const std::string& binaryData);
    static std::string toLower(const char* text, size_t size); //convert to lowercase
    static bool isAllowedAttribute(std::string input);
    static bool isAllowedTag(std::string input);
    static bool isAllowedCSS(std::string input);
};

#endif // SANITATION_H
