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

#ifndef LIBREPCB_CORE_STROKETEXTPATHBUILDER_H
#define LIBREPCB_CORE_STROKETEXTPATHBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"
#include "../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Alignment;
class StrokeFont;
class StrokeTextSpacing;

/*******************************************************************************
 *  Class StrokeTextPathBuilder
 ******************************************************************************/

/**
 * @brief The StrokeTextPathBuilder class
 */
class StrokeTextPathBuilder final {
  Q_DECLARE_TR_FUNCTIONS(StrokeTextPathBuilder)

public:
  // Constructors / Destructor
  StrokeTextPathBuilder() = delete;
  StrokeTextPathBuilder(const StrokeTextPathBuilder& other) = delete;
  ~StrokeTextPathBuilder() = delete;

  // Static Methods
  static QVector<Path> build(const StrokeFont& font,
                             const StrokeTextSpacing& letterSpacing,
                             const StrokeTextSpacing& lineSpacing,
                             const PositiveLength& height,
                             const UnsignedLength& strokeWidth,
                             const Alignment& align, const Angle& rotation,
                             bool autoRotate, const QString& text) noexcept;
  static Length calcLetterSpacing(const StrokeFont& font,
                                  const StrokeTextSpacing& spacing,
                                  const PositiveLength& height,
                                  const UnsignedLength& strokeWidth) noexcept;
  static Length calcLineSpacing(const StrokeFont& font,
                                const StrokeTextSpacing& spacing,
                                const PositiveLength& height,
                                const UnsignedLength& strokeWidth) noexcept;

  // Operator Overloadings
  StrokeTextPathBuilder& operator=(const StrokeTextPathBuilder& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
