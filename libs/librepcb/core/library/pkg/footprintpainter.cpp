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
#include "footprintpainter.h"

#include "../../application.h"
#include "../../export/graphicsexportsettings.h"
#include "../../export/graphicspainter.h"
#include "../../geometry/text.h"
#include "../../utils/transform.h"
#include "../../workspace/theme.h"
#include "footprint.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintPainter::FootprintPainter(const Footprint& footprint) noexcept
  : mStrokeFont(qApp->getDefaultStrokeFont()) {
  for (const FootprintPad& pad : footprint.getPads()) {
    mPads.append(pad);
  }
  for (const Polygon& polygon : footprint.getPolygons()) {
    mPolygons.append(polygon);
  }
  for (const Circle& circle : footprint.getCircles()) {
    mCircles.append(circle);
  }
  for (const StrokeText& text : footprint.getStrokeTexts()) {
    mStrokeTexts.append(text);
  }
  for (const Hole& hole : footprint.getHoles()) {
    mHoles.append(hole);
  }
}

FootprintPainter::~FootprintPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FootprintPainter::paint(QPainter& painter,
                             const GraphicsExportSettings& settings) const
    noexcept {
  // Determine what to paint on which color layer.
  initContentByColor();

  // Draw pad circles only if holes are enabled, but pads not.
  const bool drawPadHoles =
      settings.getPaintOrder().contains(Theme::Color::sBoardHoles) &&
      (!settings.getPaintOrder().contains(Theme::Color::sBoardPads));

  // Draw each layer in reverse order for correct stackup.
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());
  foreach (const QString& color, settings.getPaintOrder()) {
    ColorContent content = mContentByColor.value(color);

    // Draw areas.
    foreach (const QPainterPath& area, content.areas) {
      p.drawPath(area, Length(0), QColor(), settings.getColor(color));
    }

    // Draw polygons.
    foreach (const Polygon& polygon, content.polygons) {
      p.drawPolygon(polygon.getPath(), *polygon.getLineWidth(),
                    settings.getColor(color),
                    settings.getFillColor(color, polygon.isFilled(),
                                          polygon.isGrabArea()));
    }

    // Draw circles.
    foreach (const Circle& circle, content.circles) {
      p.drawCircle(
          circle.getCenter(), *circle.getDiameter(), *circle.getLineWidth(),
          settings.getColor(color),
          settings.getFillColor(color, circle.isFilled(), circle.isGrabArea()));
    }

    // Draw holes.
    foreach (const Hole& hole, content.holes) {
      p.drawSlot(*hole.getPath(), hole.getDiameter(), Length(0),
                 settings.getColor(color), QColor());
    }

    // Draw pad holes.
    if (drawPadHoles) {
      foreach (const Hole& hole, content.padHoles) {
        p.drawSlot(*hole.getPath(), hole.getDiameter(), Length(0),
                   settings.getColor(color), QColor());
      }
    }

    // Draw invisible texts to make them selectable and searchable in PDF and
    // SVG output.
    foreach (const Text& text, content.texts) {
      p.drawText(text.getPosition(), text.getRotation(), *text.getHeight(),
                 text.getAlign(), text.getText(),
                 qApp->getDefaultMonospaceFont(), Qt::transparent, true,
                 settings.getMirror());
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintPainter::initContentByColor() const noexcept {
  QMutexLocker lock(&mMutex);
  if (mContentByColor.isEmpty()) {
    // Footprint polygons.
    foreach (Polygon polygon, mPolygons) {
      const QString color = polygon.getLayer().getThemeColor();
      mContentByColor[color].polygons.append(polygon);
    }

    // Footprint circles.
    foreach (Circle circle, mCircles) {
      const QString color = circle.getLayer().getThemeColor();
      mContentByColor[color].circles.append(circle);
    }

    // Footprint holes.
    foreach (Hole hole, mHoles) {
      mContentByColor[Theme::Color::sBoardHoles].holes.append(hole);
    }

    // Footprint pads.
    foreach (FootprintPad pad, mPads) {
      const Transform transform(pad.getPosition(), pad.getRotation());
      const QPainterPath path =
          transform.mapPx(pad.getGeometry().toQPainterPathPx());
      const QString color = pad.isTht() ? Theme::Color::sBoardPads
                                        : pad.getSmtLayer().getThemeColor();
      mContentByColor[color].areas.append(path);

      // Also add the holes for THT pads.
      for (const PadHole& hole : pad.getHoles()) {
        mContentByColor[Theme::Color::sBoardHoles].padHoles.append(
            Hole(hole.getUuid(), hole.getDiameter(),
                 transform.map(hole.getPath()), MaskConfig::off()));
      }
    }

    // Texts.
    foreach (StrokeText text, mStrokeTexts) {
      Transform transform(text);
      const QString color = text.getLayer().getThemeColor();
      foreach (Path path, transform.map(text.generatePaths(mStrokeFont))) {
        mContentByColor[color].polygons.append(
            Polygon(text.getUuid(), text.getLayer(), text.getStrokeWidth(),
                    false, false, path));
      }

      Angle rotation = transform.map(Angle::deg0());
      Alignment align =
          text.getMirrored() ? text.getAlign().mirroredV() : text.getAlign();
      Length totalHeight = *text.getHeight() + *text.getStrokeWidth();
      totalHeight += totalHeight / 2;  // Correction factor from TTF to stroke.
      Length baseline = totalHeight / 4;  // Baseline correction TTF -> stroke.
      Point baselineOffset;
      if (align.getV() == VAlign::bottom()) {
        baselineOffset.setY(-baseline);
      } else if (align.getV() == VAlign::top()) {
        baselineOffset.setY(baseline);
      }
      baselineOffset.rotate(rotation);
      mContentByColor[color].texts.append(
          Text(text.getUuid(), text.getLayer(), text.getText(),
               text.getPosition() + baselineOffset, rotation,
               PositiveLength(totalHeight), align));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
