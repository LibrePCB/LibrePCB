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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "attributesubstitutor.h"

#include "attributeprovider.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

QString AttributeSubstitutor::substitute(QString                  str,
                                         const AttributeProvider* ap,
                                         FilterFunction filter) noexcept {
  int           startPos           = 0;
  int           length             = 0;
  int           outerVariableStart = -1;
  int           outerVariableEnd   = -1;  // counted from end of string
  QString       value;
  QStringList   keys;
  QSet<QString> keyBacktrace;  // avoid endless recursion
  while (searchVariablesInText(str, startPos, startPos, length, keys)) {
    if (filter && (startPos + length > str.length() - outerVariableEnd)) {
      applyFilter(str, outerVariableStart, outerVariableEnd, filter);
    }
    if (filter && (outerVariableStart < 0)) {
      outerVariableStart = startPos;
      outerVariableEnd   = str.length() - length - startPos;
    }
    bool keyFound = false;
    foreach (const QString& key, keys) {
      if (key.startsWith('\'') && key.endsWith('\'')) {
        // replace "{{'VALUE'}}" with "VALUE"
        str.replace(startPos, length, key.mid(1, key.length() - 2));
        startPos +=
            key.length() - 2;  // do not search for variables in the value
        keyFound = true;
        break;
      } else if ((getValueOfKey(key, value, ap)) &&
                 (!keyBacktrace.contains(key))) {
        // replace "{{KEY}}" with the value of KEY
        str.replace(startPos, length, value);
        keyBacktrace.insert(key);
        keyFound = true;
        break;
      }
    }
    if (!keyFound) {
      // attribute not found, remove "{{KEY}}" from str
      str.remove(startPos, length);
    }
  }
  if (filter && (outerVariableStart >= 0)) {
    applyFilter(str, outerVariableStart, outerVariableEnd, filter);
  }
  return str;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool AttributeSubstitutor::searchVariablesInText(const QString& text,
                                                 int startPos, int& pos,
                                                 int&         length,
                                                 QStringList& keys) noexcept {
  QRegularExpression      re("\\{\\{(.*?)\\}\\}");
  QRegularExpressionMatch match = re.match(text, startPos);
  if (match.hasMatch() && match.capturedLength() > 0) {
    pos = match.capturedStart();
    if (text.midRef(pos).startsWith("{{ '}}' }}")) {
      // special case to escape '}}' as it doesn't work with the regex above
      length = 10;
      keys   = QStringList{"'}}'"};
    } else {
      length = match.capturedLength();
      keys   = match.captured(1).split(" or ");
      for (QString& key : keys) {
        key = key.trimmed();
      }
    }
    return true;
  } else {
    return false;
  }
}

void AttributeSubstitutor::applyFilter(QString& str, int& start, int& end,
                                       FilterFunction filter) noexcept {
  int length = str.length() - end - start;
  str.replace(start, length, filter(str.mid(start, length)));
  start = -1;
  end   = -1;
}

bool AttributeSubstitutor::getValueOfKey(const QString& key, QString& value,
                                         const AttributeProvider* ap) noexcept {
  if (ap) {
    value = ap->getAttributeValue(key);
    return !value.isEmpty();
  } else {
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
