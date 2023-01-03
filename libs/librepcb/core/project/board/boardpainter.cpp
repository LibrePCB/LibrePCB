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
#include "../../font/strokefontpool.h"
#include "../../geometry/text.h"
#include "../../graphics/graphicslayer.h"
#include "../../graphics/graphicspainter.h"
#include "../../library/pkg/footprint.h"
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
      fpt.pads.append(pad->getLibPad());
    }
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      fpt.polygons.append(polygon);
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      fpt.circles.append(circle);
    }
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      fpt.holes.append(hole);
    }
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      StrokeText copy(text->getText());
      copy.setText(AttributeSubstitutor::substitute(copy.getText(), device));
      mStrokeTexts.append(copy);
    }
    mFootprints.append(fpt);
  }
  foreach (const BI_Plane* plane, board.getPlanes()) {
    mPlanes.append(Plane{*plane->getLayerName(), plane->getFragments()});
  }
  foreach (const BI_Polygon* polygon, board.getPolygons()) {
    mPolygons.append(polygon->getPolygon());
  }
  foreach (const BI_StrokeText* text, board.getStrokeTexts()) {
    StrokeText copy(text->getText());
    copy.setText(AttributeSubstitutor::substitute(copy.getText(), &board));
    mStrokeTexts.append(copy);
  }
  foreach (const BI_Hole* hole, board.getHoles()) {
    mHoles.append(hole->getHole());
  }
  foreach (const BI_NetSegment* segment, board.getNetSegments()) {
    for (const BI_Via* via : segment->getVias()) {
      mVias.append(via->getVia());
    }
    for (const BI_NetLine* netline : segment->getNetLines()) {
      mTraces.append(Trace{
          netline->getLayer().getName(), netline->getStartPoint().getPosition(),
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

    // Draw traces.
    foreach (const Trace& trace, content.traces) {
      p.drawLine(trace.startPosition, trace.endPosition, *trace.width,
                 settings.getColor(layer));
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

void BoardPainter::initContentByLayer() const noexcept {
  QMutexLocker lock(&mMutex);
  if (mContentByLayer.isEmpty()) {
    // Footprints.
    foreach (const Footprint& footprint, mFootprints) {
      // Footprint polygons.
      foreach (Polygon polygon, footprint.polygons) {
        polygon.setLayerName(footprint.transform.map(polygon.getLayerName()));
        polygon.setPath(footprint.transform.map(polygon.getPath()));
        mContentByLayer[*polygon.getLayerName()].polygons.append(polygon);
      }

      // Footprint circles.
      foreach (Circle circle, footprint.circles) {
        circle.setLayerName(footprint.transform.map(circle.getLayerName()));
        circle.setCenter(footprint.transform.map(circle.getCenter()));
        mContentByLayer[*circle.getLayerName()].circles.append(circle);
      }

      // Footprint holes.
      foreach (Hole hole, footprint.holes) {
        hole.setPath(NonEmptyPath(footprint.transform.map(hole.getPath())));
        mContentByLayer[GraphicsLayer::sBoardDrillsNpth].holes.append(hole);
      }

      // Footprint pads.
      foreach (FootprintPad pad, footprint.pads) {
        const Transform transform(pad.getPosition(), pad.getRotation());
        const QPainterPath path =
            footprint.transform.mapPx(transform.mapPx(pad.toQPainterPathPx()));
        const QString layer = footprint.transform.map(pad.getLayerName());
        mContentByLayer[layer].areas.append(path);

        // Also add the holes for THT pads.
        for (const Hole& hole : pad.getHoles()) {
          mContentByLayer[GraphicsLayer::sBoardDrillsNpth].padHoles.append(
              Hole(hole.getUuid(), hole.getDiameter(),
                   footprint.transform.map(transform.map(hole.getPath()))));
        }
      }
    }

    // Planes.
    foreach (const Plane& plane, mPlanes) {
      foreach (const Path& path, plane.fragments) {
        mContentByLayer[plane.layerName].areas.append(path.toQPainterPathPx());
      }
    }

    // Vias.
    foreach (const Via& via, mVias) {
      mContentByLayer[GraphicsLayer::sBoardViasTht].areas.append(
          via.toQPainterPathPx().translated(via.getPosition().toPxQPointF()));
    }

    // Traces.
    foreach (const Trace& trace, mTraces) {
      mContentByLayer[trace.layerName].traces.append(trace);
    }

    // Polygons.
    foreach (const Polygon& polygon, mPolygons) {
      mContentByLayer[*polygon.getLayerName()].polygons.append(polygon);
    }

    // Holes.
    foreach (const Hole& hole, mHoles) {
      mContentByLayer[GraphicsLayer::sBoardDrillsNpth].holes.append(hole);
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
