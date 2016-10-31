/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "if_attributeprovider.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class IF_AttributeProvider
 ****************************************************************************************/

int IF_AttributeProvider::replaceVariablesWithAttributes(QString& rawText, bool passToParents) const noexcept
{
    int count = 0;
    int startPos = 0;
    int length = 0;
    QString varNS;
    QString varName;
    QString varValue;
    while (searchVariableInText(rawText, startPos, startPos, length, varNS, varName))
    {
        if (getAttributeValue(varNS, varName, passToParents, varValue))
        {
            // avoid endless recursion
            QString complete = rawText.mid(startPos, length);
            varValue.replace(complete, QCoreApplication::translate("IF_AttributeProvider",
                                                                   "[RECURSION REMOVED]"));
            rawText.replace(startPos, length, varValue);
        }
        else
            rawText.remove(startPos, length);
        count++;
    }
    return count;
}

bool IF_AttributeProvider::searchVariableInText(const QString& text, int startPos, int& pos,
                                                int& length, QString& varNS, QString& varName) noexcept
{
    pos = text.indexOf("${", startPos);         // index of '$'
    if (pos < 0) return false;
    length = text.indexOf("}", pos) - pos + 1;  // length inclusive '${' and '}'
    if (length < 3) return false;
    int namespaceLength = text.indexOf("::", pos+2) - pos - 2; // count of chars between '${' and '::'
    if ((namespaceLength < 0) || (namespaceLength > length-3))
    {
        // no namespace defined
        varNS.clear();
        varName = text.mid(pos+2, length-3);
    }
    else
    {
        // namespace defined
        varNS = text.mid(pos+2, namespaceLength);
        varName = text.mid(pos+4+namespaceLength, length-5-namespaceLength);
    }
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
