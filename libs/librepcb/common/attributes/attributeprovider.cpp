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
#include "attributeprovider.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class AttributeProvider
 ****************************************************************************************/

int AttributeProvider::replaceVariablesWithAttributes(QString& rawText, bool passToParents) const noexcept
{
    int count = 0;
    int startPos = 0;
    int length = 0;
    QString varName;
    QString varValue;
    while (searchVariableInText(rawText, startPos, startPos, length, varName))
    {
        if (getAttributeValue(varName, passToParents, varValue))
        {
            // avoid endless recursion
            QString complete = rawText.mid(startPos, length);
            varValue.replace(complete, QCoreApplication::translate("AttributeProvider",
                                                                   "[RECURSION REMOVED]"));
            rawText.replace(startPos, length, varValue);
        }
        else
            rawText.remove(startPos, length);
        count++;
    }
    return count;
}

bool AttributeProvider::searchVariableInText(const QString& text, int startPos, int& pos,
                                                int& length, QString& varName) noexcept
{
    pos = text.indexOf("#", startPos);          // index of '#'
    if (pos < 0) return false;
    length = getLengthOfKey(text, pos + 1) + 1; // length inclusive '#'
    if (length < 2) return false;
    varName = text.mid(pos+1, length-1);
    return true;
}

int AttributeProvider::getLengthOfKey(const QString& text, int startPos) noexcept
{
    QString allowedChars("_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    for (int i = startPos; i < text.length(); ++i) {
        if (!allowedChars.contains(text.at(i))) {
            return i - startPos;
        }
    }
    return text.length() - startPos;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
