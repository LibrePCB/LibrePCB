/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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
#include "mathparser.h"

#include <muParser.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MathParser::MathParser() noexcept : mLocale() {
}

MathParser::~MathParser() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void MathParser::setLocale(const QLocale& locale) noexcept {
  mLocale = locale;
}

MathParser::Result MathParser::parse(QString expression) const noexcept {
  MathParser::Result result;

  try {
    const QString decimalPoint(mLocale.decimalPoint());
    const QString groupSeparator(mLocale.groupSeparator());

    // Always support '.' as decimal separator in addition to the
    // locale-dependent separator, especially for German people
    // (https://github.com/LibrePCB/LibrePCB/issues/1367).
    if (decimalPoint != ".") {
      expression.replace(".", decimalPoint);
    }

    mu::Parser parser;
    parser.SetArgSep(';');  // avoid conflict with other separators
    if (decimalPoint.length() == 1) {
      parser.SetDecSep(decimalPoint.at(0).toLatin1());
    }
    if (groupSeparator.length() == 1) {
      parser.SetThousandsSep(groupSeparator.at(0).toLatin1());
    }
#if defined(_UNICODE)
    parser.SetExpr(expression.toStdWString());
#else
    parser.SetExpr(expression.toStdString());
#endif
    result.value = static_cast<qreal>(parser.Eval());  // can throw
    result.valid = true;
  } catch (const mu::Parser::exception_type& e) {
    result.valid = false;
    result.error = tr("Failed to parse expression:") % "\n\n";
#if defined(_UNICODE)
    result.error += QString::fromStdWString(e.GetMsg());
#else
    result.error += QString::fromStdString(e.GetMsg());
#endif
  }

  return result;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
