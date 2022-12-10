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
#include "../../geometry/text.h"
#include "../../graphics/graphicslayer.h"
#include "../../graphics/graphicspainter.h"
#include "../../utils/transform.h"
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
  // Determine what to paint on which layer.
  initContentByLayer();

  // Draw pad circles only if holes are enabled, but pads not.
  const bool drawPadHoles =
      settings.getLayerPaintOrder().contains(GraphicsLayer::sBoardDrillsNpth) &&
      (!settings.getLayerPaintOrder().contains(GraphicsLayer::sBoardPadsTht));

  // Draw each layer in reverse order for correct stackup.
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());
  foreach (const QString& layer, settings.getLayerPaintOrder()) {
    LayerContent content = mContentByLayer.value(layer);

    // Draw areas.
    foreach (const QPainterPath& area, content.areas) {
      p.drawPath(area, Length(0), QColor(), settings.getColor(layer));
    }

    // Draw polygons.
    foreach (const Polygon& polygon, content.polygons) {
      p.drawPolygon(polygon.getPath(), *polygon.getLineWidth(),
                    settings.getColor(layer),
                    settings.getFillColor(layer, polygon.isFilled(),
                                          polygon.isGrabArea()));
    }

    // Draw circles.
    foreach (const Circle& circle, content.circles) {
      p.drawCircle(
          circle.getCenter(), *circle.getDiameter(), *circle.getLineWidth(),
          settings.getColor(layer),
          settings.getFillColor(layer, circle.isFilled(), circle.isGrabArea()));
    }

    // Draw holes.
    foreach (const Hole& hole, content.holes) {
      p.drawSlot(*hole.getPath(), hole.getDiameter(), Length(0),
                 settings.getColor(layer), QColor());
    }

    // Draw pad holes.
    if (drawPadHoles) {
      foreach (const Hole& hole, content.padHoles) {
        p.drawSlot(*hole.getPath(), hole.getDiameter(), Length(0),
                   settings.getColor(layer), QColor());
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

void FootprintPainter::initContentByLayer() const noexcept {
  QMutexLocker lock(&mMutex);
  if (mContentByLayer.isEmpty()) {
    // Footprint polygons.
    foreach (Polygon polygon, mPolygons) {
      mContentByLayer[*polygon.getLayerName()].polygons.append(polygon);
    }

    // Footprint circles.
    foreach (Circle circle, mCircles) {
      mContentByLayer[*circle.getLayerName()].circles.append(circle);
    }

    // Footprint holes.
    foreach (Hole hole, mHoles) {
      mContentByLayer[GraphicsLayer::sBoardDrillsNpth].holes.append(hole);
    }

    // Footprint pads.
    foreach (FootprintPad pad, mPads) {
      QPainterPath path = pad.getOutline()
                              .rotated(pad.getRotation())
                              .translated(pad.getPosition())
                              .toQPainterPathPx();
      path.setFillRule(Qt::OddEvenFill);  // To subtract the hole!
      path.addEllipse(pad.getPosition().toPxQPointF(),
                      pad.getDrillDiameter()->toPx() / 2,
                      pad.getDrillDiameter()->toPx() / 2);
      QString layer;
      switch (pad.getBoardSide()) {
        case FootprintPad::BoardSide::TOP:
          layer = GraphicsLayer::sTopCopper;
          break;
        case FootprintPad::BoardSide::BOTTOM:
          layer = GraphicsLayer::sBotCopper;
          break;
        default:
          layer = GraphicsLayer::sBoardPadsTht;
          break;
      }
      mContentByLayer[layer].areas.append(path);

      // Also add the hole for THT pads.
      if ((pad.getBoardSide() == FootprintPad::BoardSide::THT) &&
          (pad.getDrillDiameter() > 0)) {
        Hole hole(pad.getUuid(), PositiveLength(*pad.getDrillDiameter()),
                  makeNonEmptyPath(pad.getPosition()));
        mContentByLayer[GraphicsLayer::sBoardDrillsNpth].padHoles.append(hole);
      }
    }

    // Texts.
    foreach (StrokeText text, mStrokeTexts) {
      Transform transform(text);
      foreach (Path path, transform.map(text.generatePaths(mStrokeFont))) {
        mContentByLayer[*text.getLayerName()].polygons.append(
            Polygon(text.getUuid(), text.getLayerName(), text.getStrokeWidth(),
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
      mContentByLayer[*text.getLayerName()].texts.append(
          Text(text.getUuid(), text.getLayerName(), text.getText(),
               text.getPosition() + baselineOffset, rotation,
               PositiveLength(totalHeight), align));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
