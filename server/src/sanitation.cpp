/*  Copyright (c) 2016-2018 Daniel Kos, General Development Systems

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

#include "sanitation.h"
#include <ctype.h>
#include <QByteArray>


std::set<std::string> Sanitation::tagWhitelist;
std::set<std::string> Sanitation::attrWhitelist;
std::map<char, QWebPage::WebAction> Sanitation::editCodeMap;

void Sanitation::init()
{
    //Populate the whitelist of allowed HTML elements.
    //List taken from https://www.w3.org/community/webed/wiki/HTML/Elements, with forbidden/nonapplicable tags deleted.
    tagWhitelist = {    "section",
                        "nav",
                        "article"
                        "aside",
                        "h1",
                        "h2",
                        "h3",
                        "h4",
                        "h5",
                        "h6",
                        "hgroup",
                        "header",
                        "footer",
                        "address",
                        "p",
                        "hr",
                        "pre",
                        "blockquote",
                        "ol",
                        "ul",
                        "li",
                        "dl",
                        "dt",
                        "dd",
                        "figure",
                        "figcaption",
                        "div",
                        "center",
                        "a",
                        "abbr",
                        "b",
                        "bdo",
                        "big",
                        "br",
                        "cite",
                        "code",
                        "dfn",
                        "em",
                        "i",
                        "kbd",
                        "mark",
                        "q",
                        "rp",
                        "rt",
                        "ruby",
                        "s",
                        "samp",
                        "small",
                        "spacer",
                        "span",
                        "strong",
                        "sub",
                        "sup",
                        "time",
                        "tt",
                        "u",
                        "var",
                        "wbr",
                        "ins",
                        "del",
                        "img",
                        "iframe",
                        "video",
                        "audio",
                        "source",
                        "track",
                        "canvas", //will need to make an instruction for manipulating canvas.
                        "map",
                        "area",
                        "svg",
                        "frame",
                        "frameset",
                        "table",
                        "caption",
                        "colgroup",
                        "col",
                        "tbody",
                        "thead",
                        "tfoot",
                        "tr",
                        "td",
                        "th",
                        "form",
                        "fieldset",
                        "legend",
                        "label",
                        "input",
                        "button",
                        "select",
                        "datalist",
                        "optgroup",
                        "option",
                        "textarea",
                        "output",
                        "progress",
                        "meter",
                        "details",
                        "summary",
                        "command",
                        "menu"
                   };


    //List of attributes obtained from https://www.w3.org/TR/html4/index/attributes.html
    //This is a whitelist of safe attributes that the user can use freely without special instructions.
    attrWhitelist = {   "abbr",
                        "accept-charset",
                        "accesskey",
                        "align",
                        "alt",
                        "autoplay",
                        "border",
                        "cellpadding",
                        "cellspacing",
                        "char",
                        "charoff",
                        "checked",
                        "cols",
                        "colspan",
                        "contenteditable",
                        "controls",
                        "coords",
                        "dir",
                        "disabled",
                        "for",
                        "frame",
                        "frameborder",
                        "headers",
                        "height",
                        "id",
                        "label",
                        "loop",
                        "maxlength",
                        "multiple",
                        "muted",
                        "name",
                        "noresize",
                        "readonly",
                        "rows",
                        "rowspan",
                        "rules",
                        "scope",
                        "scrolling",
                        "selected",
                        "shape",
                        "size",
                        "span",
                        "summary",
                        "tabindex",
                        "title",
                        "type",
                        "usemap",
                        "valign",
                        "value",
                        "width"
                    };

    //hipe's EDIT instructions (HIPE_OP_EDIT_ACTION and HIPE_OP_EDIT_STATUS)
    //use character codes such as 'x' (cut), 'z' (undo), etc. to specify
    //the actions required. These need to be converted into Qt's enumerated
    //constants as an intermediary step to looking up the relevant QAction object(s)
    //needed to trigger those actions.
    editCodeMap = {     {'z', QWebPage::Undo},
                        {'Z', QWebPage::Redo},
                        {'a', QWebPage::SelectAll},

                        {'x', QWebPage::Cut},
                        {'c', QWebPage::Copy},
                        {'v', QWebPage::PasteAndMatchStyle}, //match destination formatting
                        {'V', QWebPage::Paste}, //match source formatting

                        {'b', QWebPage::ToggleBold},
                        {'i', QWebPage::ToggleItalic},
                        {'u', QWebPage::ToggleUnderline},
                        {'k', QWebPage::ToggleStrikethrough},

                        {'l', QWebPage::AlignLeft},
                        {'c', QWebPage::AlignCenter},
                        {'r', QWebPage::AlignRight},
                        {'j', QWebPage::AlignJustified}

                    };
}

std::string Sanitation::sanitisePlainText(std::string input, bool convertLayout)
//Processes input string, replaces special HTML characters like < with their
//equivalent nonfunctional representations, like &lt;. This is used to prevent
//HTML injection attacks.
//And a bonus feature (if convertLayout set): '\n' and '\r' will be replaced by
//<br/> and <p/> respectively. This makes inserting line breaks in Hipe a lot
//less painful - just put actual line feeds (CRs for parags) in any text to be
//appended. Tabs can also be entered using '\t' -- they are replaced with an
//analogous HTML character entity for a wider-than-usual space.
{
    std::string output;
    for(size_t i=0; i<input.size(); i++) {
        if(convertLayout) {
            if(input[i] == '\n')
                output += "<br/>";
            else if(input[i] == '\r')
                output += "<p></p>"; //I'd use <p/>, but webkit doesn't parse it right when adding one element at a time.
            else if(input[i] == '\t')
                output += "&emsp;";  //tab
        }
        if(input[i] == '<')
            output += "&lt;";
        else if(input[i] == '>')
            output += "&gt;";
        else if(input[i] == '"')
            output += "&quot;";
        else if(input[i] == '\'')
            output += "&#39;";
        else
            output += input[i];
    }
    return output;
}

std::string Sanitation::sanitiseCanvasInstruction(std::string input) {
//removes parentheses, braces and semicolons from strings intended as canvas instructions.
//This prevents injections of arbitrary javascript code, which can degrade systemwide
//performance among other things.
//If sanitation fails for any reason, the whole string is rejected.

    for(size_t i=0; i<input.size(); i++) {
        if(input[i] == '(') return "";
        else if(input[i] == ')') return "";
        else if(input[i] == ';') return "";
    }
    return input;
}

std::string Sanitation::toBase64(const std::string& binaryData) {
    QByteArray b64qData = QByteArray(binaryData.data(), binaryData.size()).toBase64();
    //Qt provides a nice convenience function here.
    return std::string(b64qData.data(), b64qData.size());
}

std::string Sanitation::toLower(const char* text, size_t length) {
    std::string result(text, length);
    for(size_t i=0; i<length; i++)
        if(result[i] >= 'A' && result[i] <= 'Z') result[i] -= ('A'-'a');
    return result;
}


bool Sanitation::isAllowedAttribute(std::string input)
//returns true iff use of the tag attribute is permitted by Hipe.
//List of attributes adapted from https://www.w3.org/TR/html4/index/attributes.html
//This is a whitelist of safe attributes that the user can use freely without special instructions.
{
    if(attrWhitelist.find(input) != attrWhitelist.end())
        return true;
    return false;
}

bool Sanitation::isAllowedTag(std::string input)
//returns true if the specified HTML tag is an allowed type. (e.g. "button" tags are allowed, "script" tags are not).
{
    if(tagWhitelist.find(input) != tagWhitelist.end())
        return true;
    return false;
}

bool Sanitation::isAllowedCSS(std::string input)
{
    for(size_t i=0; i<input.size(); i++) {
        if(input[i] == '<') return false; //don't let the user break out of the stylesheet to inject html code.
        if(input[i] == '>') return false;
        if(input[i] == '{') return false; //the user isn't allowed to fill in a whole stylesheet directly, so has no need of these.
        if(input[i] == '}') return false;
        if((input[i] == 'u' || input[i] == 'U') && i+3 < input.size()) { //screen for URLs, which are not allowed to be entered directly.
            if((input[i+1] == 'r' || input[i+1] == 'R')
                    && (input[i+1] == 'l' || input[i+1] == 'L')) {
                i+=3;
                //we've detected an instance of the string "url".
                //at this point, we'll reject the input if there is a '(' or whitespace followed by a '('.
                while(i<input.size()) //skip any whitespace.
                    if(isspace(input[i]))
                        i++;
                if(i<input.size() && input[i] == '(') return false;
            }
        }
    }
    return true;
}

QWebPage::WebAction Sanitation::editCodeLookup(char code) {
    try {
        return editCodeMap.at(code);
    } catch(std::out_of_range) {
        return QWebPage::NoWebAction;
    }

}
