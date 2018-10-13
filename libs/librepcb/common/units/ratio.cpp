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
#include "ratio.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Ratio
 ******************************************************************************/

QString Ratio::toNormalizedString() const noexcept {
  QString str = QLocale::c().toString(toNormalized(), 'f', 6);
  for (int i = 0; (i < 5) && str.endsWith(QLatin1Char('0')); ++i) {
    str.chop(1);
  }
  return str;
}

// Static Methods

Ratio Ratio::fromPercent(qreal percent) noexcept {
  Ratio ratio;
  ratio.setRatioPercent(percent);
  return ratio;
}

Ratio Ratio::fromNormalized(qreal normalized) noexcept {
  Ratio ratio;
  ratio.setRatioNormalized(normalized);
  return ratio;
}

Ratio Ratio::fromNormalized(const QString& normalized) {
  Ratio ratio;
  ratio.setRatioNormalized(normalized);
  return ratio;
}

// Private Static Methods

qint32 Ratio::normalizedStringToPpm(const QString& normalized) {
  bool  ok;
  qreal ratio = qRound(QLocale::c().toDouble(normalized, &ok) * 1e6);
  if (!ok) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Invalid ratio string: \"%1\"")).arg(normalized));
  }
  return ratio;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
