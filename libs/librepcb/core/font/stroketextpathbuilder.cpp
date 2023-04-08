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
#include "stroketextpathbuilder.h"

#include "../font/strokefont.h"
#include "../types/stroketextspacing.h"
#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QVector<Path> StrokeTextPathBuilder::build(
    const StrokeFont& font, const StrokeTextSpacing& letterSpacing,
    const StrokeTextSpacing& lineSpacing, const PositiveLength& height,
    const UnsignedLength& strokeWidth, const Alignment& align,
    const Angle& rotation, bool autoRotate, bool mirror,
    const QString& text) noexcept {
  Point bottomLeft, topRight;
  QVector<Path> paths = font.stroke(
      text, height, calcLetterSpacing(font, letterSpacing, height, strokeWidth),
      calcLineSpacing(font, lineSpacing, height, strokeWidth), align,
      bottomLeft, topRight);
  if (autoRotate && Toolbox::isTextUpsideDown(rotation, mirror)) {
    const Point center = (bottomLeft + topRight) / 2;
    for (Path& p : paths) {
      p.rotate(Angle::deg180(), center);
    }
  }
  return paths;
}

Length StrokeTextPathBuilder::calcLetterSpacing(
    const StrokeFont& font, const StrokeTextSpacing& spacing,
    const PositiveLength& height, const UnsignedLength& strokeWidth) noexcept {
  if (const tl::optional<Ratio>& ratio = spacing.getRatio()) {
    // Use given letter spacing without additional factor or stroke width
    // offset. Also don't use recommended letter spacing of font.
    return Length(height->toNm() * ratio->toNormalized());
  } else {
    // Use recommended letter spacing of font, but add stroke width to avoid
    // overlapped glyphs caused by thick lines.
    return Length(height->toNm() * font.getLetterSpacing().toNormalized()) +
        strokeWidth;
  }
}

Length StrokeTextPathBuilder::calcLineSpacing(
    const StrokeFont& font, const StrokeTextSpacing& spacing,
    const PositiveLength& height, const UnsignedLength& strokeWidth) noexcept {
  if (const tl::optional<Ratio>& ratio = spacing.getRatio()) {
    // Use given line spacing without additional factor or stroke width offset.
    // Also don't use recommended line spacing of font.
    return Length(height->toNm() * ratio->toNormalized());
  } else {
    // Use recommended line spacing of font, but add stroke width to avoid
    // overlapped glyphs caused by thick lines.
    return Length(height->toNm() * font.getLineSpacing().toNormalized()) +
        strokeWidth;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
