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
 *  Public Methods
 ****************************************************************************************/

int AttributeProvider::replaceVariablesWithAttributes(QString& rawText, bool passToParents) const noexcept
{
    Q_UNUSED(passToParents);
    int count = 0;
    int startPos = 0;
    int length = 0;
    QString varName;
    QString varValue;
    QSet<QString> keys;
    while (searchVariableInText(rawText, startPos, startPos, length, varName))
    {
        if (!keys.contains(varName)) {
            varValue = getAttributeValue(varName);
            rawText.replace(startPos, length, varValue);
        }
        else {
            rawText.remove(startPos, length);
        }
        keys.insert(varName);
        count++;
    }
    return count;
}

QString AttributeProvider::getAttributeValue(const QString& key) const noexcept
{
   QVector<const AttributeProvider*> backtrace; // for endless loop detection
   return getAttributeValue(key, backtrace);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

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

QString AttributeProvider::getAttributeValue(const QString& key,
                                             QVector<const AttributeProvider*>& backtrace) const noexcept
{
    // priority 1: user defined attributes of this object
    QString value = getUserDefinedAttributeValue(key);
    if (!value.isEmpty()) return value;

    // priority 2: built-in attributes of this object
    value = getBuiltInAttributeValue(key);
    if (!value.isEmpty()) return value;

    // priority 3: attributes from all parent objects in specific order
    backtrace.append(this);
    foreach (const AttributeProvider* parent, getAttributeProviderParents()) {
        if (parent && (!backtrace.contains(parent))) { // break possible endless loop
            value = parent->getAttributeValue(key, backtrace);
            if (!value.isEmpty()) return value;
        }
    }

    // attribute not set...
    return QString();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
