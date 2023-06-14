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

#ifndef LIBREPCB_CORE_OVERLINEMARKUPPARSER_H
#define LIBREPCB_CORE_OVERLINEMARKUPPARSER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class OverlineMarkupParser
 ******************************************************************************/

/**
 * @brief Extract overlines of text with markup
 *
 * Parses text like `RST/!SHDN`, removes the functional `!` and returns the
 * coordinates where to draw overlines instead.
 *
 * Markup rules:
 *   1. A single `!` toggles the overline on/off, depending on its previous
 *      state (initial state is off).
 *   2. The character `/` implicitly switches off the overline before rendering.
 *      This can be prevented by prefixing it with `!`.
 *   3. A double `!!` has no effect on overlines, they are rendered as a
 *      single `!`. In case of an odd number of `!` (e.g. `!!!`), the **last**
 *      one toggles overline on/off.
 *   4. Any trailing `!` have no effect, they are rendered as-is (bypassing
 *      rules 1 and 3).
 */
class OverlineMarkupParser final {
  Q_DECLARE_TR_FUNCTIONS(OverlineMarkupParser)

public:
  // Constructors / Destructor
  OverlineMarkupParser() = delete;
  OverlineMarkupParser(const OverlineMarkupParser& other) = delete;

  // General Methods
  static QVector<std::pair<int, int>> extract(const QString& input,
                                              QString& output) noexcept;
  static QVector<QLineF> calculate(const QString& text, const QFontMetricsF& fm,
                                   int flags,
                                   const QVector<std::pair<int, int>>& spans,
                                   QRectF& boundingRect) noexcept;
  static void process(const QString& input, const QFontMetricsF& fm, int flags,
                      QString& output, QVector<QLineF>& overlines,
                      QRectF& boundingRect) noexcept;
  static qreal getLineWidth(qreal heightPx) noexcept;

  // Operator Overloadings
  OverlineMarkupParser& operator=(const OverlineMarkupParser& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
