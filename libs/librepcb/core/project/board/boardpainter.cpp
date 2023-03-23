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
#include "boardpainter.h"

#include "../../application.h"
#include "../../attribute/attributesubstitutor.h"
#include "../../export/graphicsexportsettings.h"
#include "../../export/graphicspainter.h"
#include "../../font/strokefontpool.h"
#include "../../geometry/text.h"
#include "../../library/pkg/footprint.h"
#include "../../workspace/theme.h"
#include "../project.h"
#include "board.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPainter::BoardPainter(const Board& board)
  : mStrokeFont(board.getProject().getStrokeFonts().getFont(
        board.getDefaultFontName()))  // can throw
{
  foreach (const BI_Device* device, board.getDeviceInstances()) {
    Footprint fpt;
    fpt.transform = Transform(*device);
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      Pad padObj;
      padObj.transform = Transform(pad->getLibPad().getPosition(),
                                   pad->getLibPad().getRotation());
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        padObj.holes.append(hole);
      }
      for (auto it = pad->getGeometries().begin();
           it != pad->getGeometries().end(); it++) {
        foreach (const PadGeometry& geometry, it.value()) {
          padObj.layerGeometries.append(std::make_pair(it.key(), geometry));
        }
      }
      fpt.pads.append(padObj);
    }
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      fpt.polygons.append(polygon);
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      fpt.circles.append(circle);
    }
    for (Hole hole : device->getLibFootprint().getHoles()) {
      // Memorize stop mask offset now to avoid needing design rules later.
      hole.setStopMaskConfig(
          MaskConfig::maybe(device->getHoleStopMasks().value(hole.getUuid())));
      fpt.holes.append(hole);
    }
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      StrokeText copy(text->getTextObj());
      copy.setText(AttributeSubstitutor::substitute(copy.getText(), device));
      mStrokeTexts.append(copy);
    }
    mFootprints.append(fpt);
  }
  foreach (const BI_Plane* plane, board.getPlanes()) {
    mPlanes.append(Plane{&plane->getLayer(), plane->getFragments()});
  }
  foreach (const BI_Polygon* polygon, board.getPolygons()) {
    mPolygons.append(polygon->getPolygon());
  }
  foreach (const BI_StrokeText* text, board.getStrokeTexts()) {
    StrokeText copy(text->getTextObj());
    copy.setText(AttributeSubstitutor::substitute(copy.getText(), &board));
    mStrokeTexts.append(copy);
  }
  foreach (const BI_Hole* hole, board.getHoles()) {
    Hole holeObj = hole->getHole();
    // Memorize stop mask offset now to avoid needing design rules later.
    holeObj.setStopMaskConfig(MaskConfig::maybe(hole->getStopMaskOffset()));
    mHoles.append(holeObj);
  }
  foreach (const BI_NetSegment* segment, board.getNetSegments()) {
    for (const BI_Via* via : segment->getVias()) {
      mVias.append(via->getVia());
    }
    for (const BI_NetLine* netline : segment->getNetLines()) {
      mTraces.append(
          Trace{&netline->getLayer(), netline->getStartPoint().getPosition(),
                netline->getEndPoint().getPosition(), netline->getWidth()});
    }
  }
}

BoardPainter::~BoardPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardPainter::paint(QPainter& painter,
                         const GraphicsExportSettings& settings) const
    noexcept {
  // Determine what to paint on which color layer.
  initContentByColor();

  // Draw THT pads depending on copper layers visibility.
  const QSet<QString> enabledCopperLayers =
      settings.getPaintOrder().toSet() & Theme::getCopperColorNames();
  bool thtPadsOnlyOnCopperLayers = !enabledCopperLayers.isEmpty();

  // Draw pad circles only if holes are enabled, but pads not.
  const bool drawPadHoles =
      settings.getPaintOrder().contains(Theme::Color::sBoardHoles) &&
      (!settings.getPaintOrder().contains(Theme::Color::sBoardPads));

  // Draw each color in reverse order for correct stackup.
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());
  foreach (const QString& color, settings.getPaintOrder()) {
    if (thtPadsOnlyOnCopperLayers && (color == Theme::Color::sBoardPads)) {
      continue;
    }

    ColorContent content = mContentByColor.value(color);

    // Draw areas.
    foreach (const QPainterPath& area, content.areas) {
      p.drawPath(area, Length(0), QColor(), settings.getColor(color));
    }

    // Draw THT pad areas.
    foreach (const QPainterPath& area, content.thtPadAreas) {
      p.drawPath(area, Length(0), QColor(),
                 settings.getColor(Theme::Color::sBoardPads));
    }

    // Draw traces.
    foreach (const Trace& trace, content.traces) {
      p.drawLine(trace.startPosition, trace.endPosition, *trace.width,
                 settings.getColor(color));
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

void BoardPainter::initContentByColor() const noexcept {
  QMutexLocker lock(&mMutex);
  if (mContentByColor.isEmpty()) {
    // Footprints.
    foreach (const Footprint& footprint, mFootprints) {
      // Footprint polygons.
      foreach (Polygon polygon, footprint.polygons) {
        polygon.setLayer(footprint.transform.map(polygon.getLayer()));
        polygon.setPath(footprint.transform.map(polygon.getPath()));
        const QString color = polygon.getLayer().getThemeColor();
        mContentByColor[color].polygons.append(polygon);
      }

      // Footprint circles.
      foreach (Circle circle, footprint.circles) {
        circle.setLayer(footprint.transform.map(circle.getLayer()));
        circle.setCenter(footprint.transform.map(circle.getCenter()));
        const QString color = circle.getLayer().getThemeColor();
        mContentByColor[color].circles.append(circle);
      }

      // Footprint holes.
      foreach (Hole hole, footprint.holes) {
        hole.setPath(NonEmptyPath(footprint.transform.map(hole.getPath())));
        mContentByColor[Theme::Color::sBoardHoles].holes.append(hole);
        PadGeometry geometry =
            PadGeometry::stroke(hole.getDiameter(), hole.getPath(), {});
        if (hole.getStopMaskConfig().isEnabled() &&
            hole.getStopMaskConfig().getOffset()) {
          geometry = geometry.withOffset(*hole.getStopMaskConfig().getOffset());
        }
        const QPainterPath stopMask = geometry.toFilledQPainterPathPx();
        mContentByColor[Theme::Color::sBoardStopMaskTop].areas.append(stopMask);
        mContentByColor[Theme::Color::sBoardStopMaskBot].areas.append(stopMask);
      }

      // Footprint pads.
      foreach (const Pad& pad, footprint.pads) {
        foreach (const auto& layerGeometry, pad.layerGeometries) {
          const QPainterPath path = footprint.transform.mapPx(
              pad.transform.mapPx(layerGeometry.second.toQPainterPathPx()));
          const QString color = layerGeometry.first->getThemeColor();
          if ((!pad.holes.isEmpty()) &&
              Theme::getCopperColorNames().contains(color)) {
            ColorContent& tht = mContentByColor[Theme::Color::sBoardPads];
            if (!tht.areas.contains(path)) {
              tht.areas.append(path);
            }
            mContentByColor[color].thtPadAreas.append(path);
          } else {
            mContentByColor[color].areas.append(path);
          }
        }
        // Also add the holes for THT pads.
        for (const PadHole& hole : pad.holes) {
          mContentByColor[Theme::Color::sBoardHoles].padHoles.append(
              Hole(hole.getUuid(), hole.getDiameter(),
                   footprint.transform.map(pad.transform.map(hole.getPath())),
                   MaskConfig::off()));
        }
      }
    }

    // Planes.
    foreach (const Plane& plane, mPlanes) {
      const QString color = plane.layer->getThemeColor();
      foreach (const Path& path, plane.fragments) {
        mContentByColor[color].areas.append(path.toQPainterPathPx());
      }
    }

    // Vias.
    foreach (const Via& via, mVias) {
      mContentByColor[Theme::Color::sBoardVias].areas.append(
          via.toQPainterPathPx().translated(via.getPosition().toPxQPointF()));
    }

    // Traces.
    foreach (const Trace& trace, mTraces) {
      const QString color = trace.layer->getThemeColor();
      mContentByColor[color].traces.append(trace);
    }

    // Polygons.
    foreach (const Polygon& polygon, mPolygons) {
      const QString color = polygon.getLayer().getThemeColor();
      mContentByColor[color].polygons.append(polygon);
    }

    // Holes.
    foreach (const Hole& hole, mHoles) {
      mContentByColor[Theme::Color::sBoardHoles].holes.append(hole);
      PadGeometry geometry =
          PadGeometry::stroke(hole.getDiameter(), hole.getPath(), {});
      if (hole.getStopMaskConfig().isEnabled() &&
          hole.getStopMaskConfig().getOffset()) {
        geometry = geometry.withOffset(*hole.getStopMaskConfig().getOffset());
      }
      const QPainterPath stopMask = geometry.toFilledQPainterPathPx();
      mContentByColor[Theme::Color::sBoardStopMaskTop].areas.append(stopMask);
      mContentByColor[Theme::Color::sBoardStopMaskBot].areas.append(stopMask);
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
