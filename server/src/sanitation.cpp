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

#include "sanitation.h"

//TODO: Lots of opportunity to optimise things here!!!

std::set<QString> Sanitation::tagWhitelist;

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
}

QString Sanitation::sanitisePlainText(QString input)
//Processes input string, replaces special HTML characters like < with their equivalent nonfunctional
//representations, like &lt;. This is used to prevent HTML injection attacks.
{
    QString output;
    for(int i=0; i<input.size(); i++) {
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

bool Sanitation::isAllowedAttribute(QString input)
//returns true iff use of the tag attribute is permitted by Hipe.
//List of attributes obtained from https://www.w3.org/TR/html4/index/attributes.html
//This is a whitelist of safe attributes that the user can use freely without special instructions.
{
    //TODO: optimise this!!


    if(input == "abbr") return true;
    if(input == "accept-charset") return true;
    if(input == "accesskey") return true;
    if(input == "align") return true;
    if(input == "alt") return true;
    if(input == "border") return true;
    if(input == "cellpadding") return true;
    if(input == "cellspacing") return true;
    if(input == "char") return true;
    if(input == "charoff") return true;
    if(input == "checked") return true;
    if(input == "cols") return true;
    if(input == "colspan") return true;
    if(input == "coords") return true;
    if(input == "dir") return true;
    if(input == "disabled") return true;
    if(input == "for") return true;
    if(input == "frame") return true;
    if(input == "frameborder") return true;
    if(input == "headers") return true;
    if(input == "height") return true;
    if(input == "id") return true;
    if(input == "label") return true;
    if(input == "maxlength") return true;
    if(input == "multiple") return true;
    if(input == "name") return true;
    if(input == "noresize") return true;
    if(input == "readonly") return true;
    if(input == "rows") return true;
    if(input == "rowspan") return true;
    if(input == "rules") return true;
    if(input == "scope") return true;
    if(input == "scrolling") return true;
    if(input == "selected") return true;
    if(input == "shape") return true;
    if(input == "size") return true;
    if(input == "span") return true;
    if(input == "summary") return true;
    if(input == "tabindex") return true;
    if(input == "title") return true;
    if(input == "type") return true;
    if(input == "usemap") return true;
    if(input == "valign") return true;
    if(input == "value") return true;
    if(input == "width") return true;

    return false;
}

bool Sanitation::isAllowedTag(QString input)
//returns true if the specified HTML tag is an allowed type. (e.g. "button" tags are allowed, "script" tags are not).
{
    if(tagWhitelist.find(input) != tagWhitelist.end()) return true;
    return false;
}

bool Sanitation::isAllowedCSS(QString input)
{
    for(int i=0; i<input.size(); i++) {
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
                    if(input[i].isSpace())
                        i++;
                if(i<input.size() && input[i] == '(') return false;
            }
        }
    }
    return true;
}
