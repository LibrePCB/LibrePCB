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
#include "../../export/graphicsexportsettings.h"
#include "../../export/graphicspainter.h"
#include "../../geometry/text.h"
#include "../../library/pkg/footprint.h"
#include "../../workspace/theme.h"
#include "../project.h"
#include "board.h"
#include "items/bi_device.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netsegment.h"
#include "items/bi_pad.h"
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
  : mMonospaceFont(Application::getDefaultMonospaceFont()),
    mCopperLayers(board.getCopperLayers()) {
  // Helper to add a pad.
  auto addPad = [this](const BI_Pad* pad) {
    Pad padObj;
    padObj.transform = Transform(*pad);
    for (const PadHole& hole : pad->getProperties().getHoles()) {
      padObj.holes.append(hole);
    }
    for (auto it = pad->getGeometries().begin();
         it != pad->getGeometries().end(); it++) {
      foreach (const PadGeometry& geometry, it.value()) {
        padObj.layerGeometries.append(std::make_pair(it.key(), geometry));
      }
    }
    mPads.append(padObj);
  };

  foreach (const BI_Device* device, board.getDeviceInstances()) {
    Footprint fpt;
    fpt.transform = Transform(*device);
    foreach (const BI_Pad* pad, device->getPads()) {
      addPad(pad);
    }
    for (const Polygon& polygon : device->getLibFootprint().getPolygons()) {
      fpt.polygons.append(PolygonData{
          &polygon.getLayer(), polygon.getPath(), polygon.getLineWidth(),
          polygon.isFilled(), polygon.isGrabArea()});
    }
    for (const Circle& circle : device->getLibFootprint().getCircles()) {
      fpt.circles.append(circle);
    }
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      // Memorize stop mask offset now to avoid needing design rules later.
      fpt.holes.append(
          HoleData{hole.getDiameter(), hole.getPath(),
                   device->getHoleStopMasks().value(hole.getUuid())});
    }
    foreach (const BI_StrokeText* text, device->getStrokeTexts()) {
      mStrokeTexts.append(StrokeTextData{
          Transform(text->getData()), &text->getData().getLayer(),
          text->getPaths(), text->getData().getHeight(),
          text->getData().getStrokeWidth(), text->getSubstitutedText(),
          text->getData().getAlign()});
    }
    mFootprints.append(fpt);
  }
  foreach (const BI_Plane* plane, board.getPlanes()) {
    mPlanes.append(Plane{&plane->getLayer(), plane->getFragments()});
  }
  foreach (const BI_Polygon* polygon, board.getPolygons()) {
    mPolygons.append(PolygonData{
        &polygon->getData().getLayer(), polygon->getData().getPath(),
        polygon->getData().getLineWidth(), polygon->getData().isFilled(),
        polygon->getData().isGrabArea()});
  }
  foreach (const BI_StrokeText* text, board.getStrokeTexts()) {
    mStrokeTexts.append(
        StrokeTextData{Transform(text->getData()), &text->getData().getLayer(),
                       text->getPaths(), text->getData().getHeight(),
                       text->getData().getStrokeWidth(),
                       text->getSubstitutedText(), text->getData().getAlign()});
  }
  foreach (const BI_Hole* hole, board.getHoles()) {
    mHoles.append(HoleData{hole->getData().getDiameter(),
                           hole->getData().getPath(),
                           hole->getStopMaskOffset()});
  }
  foreach (const BI_NetSegment* segment, board.getNetSegments()) {
    foreach (const BI_Pad* pad, segment->getPads()) {
      addPad(pad);
    }
    for (const BI_Via* via : segment->getVias()) {
      mVias.append(ViaData{
          via->getPosition(), via->getSize(), via->getDrillDiameter(),
          &via->getVia().getStartLayer(), &via->getVia().getEndLayer(),
          via->getStopMaskDiameterTop(), via->getStopMaskDiameterBottom()});
    }
    for (const BI_NetLine* netline : segment->getNetLines()) {
      mTraces.append(Trace{&netline->getLayer(), netline->getP1().getPosition(),
                           netline->getP2().getPosition(),
                           netline->getWidth()});
    }
  }
}

BoardPainter::~BoardPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardPainter::paint(
    QPainter& painter, const GraphicsExportSettings& settings) const noexcept {
  // Determine what to paint on which color layer.
  initContentByColor();

  // Draw THT pads & vias depending on copper layers visibility.
  const QSet<QString> enabledCopperLayers =
      Toolbox::toSet(settings.getPaintOrder()) & Theme::getCopperColorNames();
  bool thtOnlyOnCopperLayers = !enabledCopperLayers.isEmpty();

  // Draw pad circles only if holes are enabled, but pads not.
  const bool drawPadHoles =
      settings.getPaintOrder().contains(Theme::Color::sBoardHoles) &&
      (!settings.getPaintOrder().contains(Theme::Color::sBoardPads));

  // Draw each color in reverse order for correct stackup.
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());
  foreach (const QString& color, settings.getPaintOrder()) {
    if (thtOnlyOnCopperLayers &&
        ((color == Theme::Color::sBoardPads) ||
         (color == Theme::Color::sBoardVias))) {
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

    // Draw via areas.
    foreach (const QPainterPath& area, content.viaAreas) {
      p.drawPath(area, Length(0), QColor(),
                 settings.getColor(Theme::Color::sBoardVias));
    }

    // Draw traces.
    foreach (const Trace& trace, content.traces) {
      p.drawLine(trace.startPosition, trace.endPosition, *trace.width,
                 settings.getColor(color));
    }

    // Draw polygons.
    foreach (const PolygonData& polygon, content.polygons) {
      p.drawPolygon(
          polygon.path, *polygon.lineWidth, settings.getColor(color),
          settings.getFillColor(color, polygon.filled, polygon.grabArea));
    }

    // Draw circles.
    foreach (const Circle& circle, content.circles) {
      p.drawCircle(
          circle.getCenter(), *circle.getDiameter(), *circle.getLineWidth(),
          settings.getColor(color),
          settings.getFillColor(color, circle.isFilled(), circle.isGrabArea()));
    }

    // Draw holes.
    foreach (const HoleData& hole, content.holes) {
      p.drawSlot(*hole.path, hole.diameter, Length(0), settings.getColor(color),
                 QColor());
    }

    // Draw pad holes.
    if (drawPadHoles) {
      foreach (const HoleData& hole, content.padHoles) {
        p.drawSlot(*hole.path, hole.diameter, Length(0),
                   settings.getColor(color), QColor());
      }
    }

    // Draw invisible texts to make them selectable and searchable in PDF and
    // SVG output.
    foreach (const TextData& text, content.texts) {
      p.drawText(text.position, text.rotation, *text.height, text.align,
                 text.text, mMonospaceFont, Qt::transparent, true,
                 settings.getMirror(), false);
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
      foreach (PolygonData polygon, footprint.polygons) {
        polygon.layer = &footprint.transform.map(*polygon.layer);
        polygon.path = footprint.transform.map(polygon.path);
        const QString color = polygon.layer->getThemeColor();
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
      foreach (HoleData hole, footprint.holes) {
        hole.path = NonEmptyPath(footprint.transform.map(hole.path));
        mContentByColor[Theme::Color::sBoardHoles].holes.append(hole);
        PadGeometry geometry =
            PadGeometry::stroke(hole.diameter, hole.path, {});
        if (const std::optional<Length>& offset = hole.stopMaskOffset) {
          geometry = geometry.withOffset(*offset);
        }
        const QPainterPath stopMask = geometry.toFilledQPainterPathPx();
        mContentByColor[Theme::Color::sBoardStopMaskTop].areas.append(stopMask);
        mContentByColor[Theme::Color::sBoardStopMaskBot].areas.append(stopMask);
      }
    }

    // Planes.
    foreach (const Plane& plane, mPlanes) {
      const QString color = plane.layer->getThemeColor();
      foreach (const Path& path, plane.fragments) {
        mContentByColor[color].areas.append(path.toQPainterPathPx());
      }
    }

    // Pads.
    foreach (const Pad& pad, mPads) {
      foreach (const auto& layerGeometry, pad.layerGeometries) {
        const QPainterPath path =
            pad.transform.mapPx(layerGeometry.second.toQPainterPathPx());
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
            HoleData{hole.getDiameter(), pad.transform.map(hole.getPath()),
                     std::nullopt});
      }
    }

    // Vias.
    foreach (const ViaData& via, mVias) {
      foreach (const Layer* layer, mCopperLayers) {
        if ((layer->getCopperNumber() >= via.startLayer->getCopperNumber()) &&
            (layer->getCopperNumber() <= via.endLayer->getCopperNumber())) {
          const QPainterPath path = Via::toQPainterPathPx(via.size, via.drill)
                                        .translated(via.position.toPxQPointF());
          ColorContent& vias = mContentByColor[Theme::Color::sBoardVias];
          if (!vias.areas.contains(path)) {
            vias.areas.append(path);
          }
          mContentByColor[layer->getThemeColor()].viaAreas.append(path);
        }
      }
      auto stopMasks = {
          std::make_pair(QString(Theme::Color::sBoardStopMaskTop),
                         via.stopMaskDiameterTop),
          std::make_pair(QString(Theme::Color::sBoardStopMaskBot),
                         via.stopMaskDiameterBottom),
      };
      for (const auto& cfg : stopMasks) {
        if (const auto diameter = cfg.second) {
          const Path outline = Path::circle(*diameter).translated(via.position);
          mContentByColor[cfg.first].areas.append(outline.toQPainterPathPx());
        }
      }
    }

    // Traces.
    foreach (const Trace& trace, mTraces) {
      const QString color = trace.layer->getThemeColor();
      mContentByColor[color].traces.append(trace);
    }

    // Polygons.
    foreach (const PolygonData& polygon, mPolygons) {
      const QString color = polygon.layer->getThemeColor();
      mContentByColor[color].polygons.append(polygon);
    }

    // Holes.
    foreach (const HoleData& hole, mHoles) {
      mContentByColor[Theme::Color::sBoardHoles].holes.append(hole);
      PadGeometry geometry = PadGeometry::stroke(hole.diameter, hole.path, {});
      if (const std::optional<Length>& offset = hole.stopMaskOffset) {
        geometry = geometry.withOffset(*offset);
      }
      const QPainterPath stopMask = geometry.toFilledQPainterPathPx();
      mContentByColor[Theme::Color::sBoardStopMaskTop].areas.append(stopMask);
      mContentByColor[Theme::Color::sBoardStopMaskBot].areas.append(stopMask);
    }

    // Texts.
    foreach (const StrokeTextData& text, mStrokeTexts) {
      const QString color = text.layer->getThemeColor();
      foreach (const Path& path, text.transform.map(text.paths)) {
        mContentByColor[color].polygons.append(
            PolygonData{text.layer, path, text.strokeWidth, false, false});
      }

      Angle rotation = text.transform.mapNonMirrorable(Angle::deg0());
      Alignment align =
          text.transform.getMirrored() ? text.align.mirroredV() : text.align;
      Length totalHeight = *text.height + *text.strokeWidth;
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
          TextData{text.transform.getPosition() + baselineOffset, rotation,
                   PositiveLength(totalHeight), align, text.text});
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
