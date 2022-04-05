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

#ifndef UNITTESTS_CORE_SEXPRESSIONLEGACYMODE_H
#define UNITTESTS_CORE_SEXPRESSIONLEGACYMODE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/core/serialization/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Class SExpressionLegacyMode
 ******************************************************************************/

class SExpressionLegacyMode final {
  bool mOldValue;

public:
  SExpressionLegacyMode() = delete;
  SExpressionLegacyMode(bool newValue) : mOldValue(SExpression::legacyMode()) {
    SExpression::legacyMode() = newValue;
  }
  SExpressionLegacyMode(const SExpressionLegacyMode& other) = delete;
  ~SExpressionLegacyMode() { SExpression::legacyMode() = mOldValue; }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

#endif
