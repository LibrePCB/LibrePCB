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
#include "boardinteractivehtmlbomgenerator.h"

#include "../../attribute/attributesubstitutor.h"
#include "../../export/interactivehtmlbom.h"
#include "../../geometry/padgeometry.h"
#include "../../geometry/padhole.h"
#include "../../library/pkg/footprint.h"
#include "../../library/pkg/package.h"
#include "../../types/layer.h"
#include "../../utils/toolbox.h"
#include "../../utils/transform.h"
#include "../circuit/assemblyvariant.h"
#include "../circuit/circuit.h"
#include "../circuit/componentinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "../projectattributelookup.h"
#include "board.h"
#include "items/bi_device.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netsegment.h"
#include "items/bi_pad.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardInteractiveHtmlBomGenerator::BoardInteractiveHtmlBomGenerator(
    const Board& board, const std::shared_ptr<AssemblyVariant>& av) noexcept
  : mBoard(board), mAssemblyVariant(av) {
}

BoardInteractiveHtmlBomGenerator::~BoardInteractiveHtmlBomGenerator() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardInteractiveHtmlBomGenerator::setCustomAttributes(
    const QStringList& attributes) noexcept {
  mCustomAttributes = attributes;
}

void BoardInteractiveHtmlBomGenerator::setComponentOrder(
    const QStringList& order) noexcept {
  mComponentOrder = order;
}

std::shared_ptr<InteractiveHtmlBom> BoardInteractiveHtmlBomGenerator::generate(
    const QDateTime& dt) {
  if (!mAssemblyVariant) throw LogicError(__FILE__, __LINE__);

  // Create IBOM.
  QString name = *mBoard.getProject().getName();
  if (mBoard.getProject().getCircuit().getAssemblyVariants().count() > 1) {
    name.append(QString(" (%1)").arg(*mAssemblyVariant->getName()));
  }
  const std::optional<std::pair<Point, Point>> bbox =
      mBoard.calculateBoundingRect();
  std::shared_ptr<InteractiveHtmlBom> ibom(new InteractiveHtmlBom(
      name, mBoard.getProject().getAuthor(), *mBoard.getProject().getVersion(),
      dt.toString("yyyy-MM-dd hh:mm:ss"), bbox ? bbox->first : Point(),
      bbox ? bbox->second : Point()));

  // Set configuration.
  ibom->setFields(QStringList{"Value / MPN", "Package"} + mCustomAttributes);

  // Add board drawings.
  const QHash<const Layer*, InteractiveHtmlBom::Layer> layerMap = {
      {&Layer::topCopper(), InteractiveHtmlBom::Layer::Top},
      {&Layer::botCopper(), InteractiveHtmlBom::Layer::Bottom},
  };
  const QMap<const Layer*, InteractiveHtmlBom::DrawingKind> drawingKindMap = {
      {&Layer::topNames(), InteractiveHtmlBom::DrawingKind::ReferenceText},
      {&Layer::botNames(), InteractiveHtmlBom::DrawingKind::ReferenceText},
      {&Layer::topValues(), InteractiveHtmlBom::DrawingKind::ValueText},
      {&Layer::botValues(), InteractiveHtmlBom::DrawingKind::ValueText},
  };
  QMap<const Layer*, InteractiveHtmlBom::DrawingLayer> drawingLayerMap = {
      {&Layer::boardOutlines(), InteractiveHtmlBom::DrawingLayer::Edge},
      {&Layer::boardCutouts(), InteractiveHtmlBom::DrawingLayer::Edge},
      {&Layer::topDocumentation(),
       InteractiveHtmlBom::DrawingLayer::FabricationFront},
      {&Layer::botDocumentation(),
       InteractiveHtmlBom::DrawingLayer::FabricationBack},
  };
  for (const Layer* layer : mBoard.getSilkscreenLayersTop()) {
    drawingLayerMap.insert(layer,
                           InteractiveHtmlBom::DrawingLayer::SilkscreenFront);
  }
  for (const Layer* layer : mBoard.getSilkscreenLayersBot()) {
    drawingLayerMap.insert(layer,
                           InteractiveHtmlBom::DrawingLayer::SilkscreenBack);
  }
  auto addDrawing = [&](const Layer& layer, const Path& path,
                        UnsignedLength width, bool filled) {
    auto drawingLayer = drawingLayerMap.find(&layer);
    auto copperLayer = layerMap.find(&layer);
    if (drawingLayer != drawingLayerMap.end()) {
      if (*drawingLayer == InteractiveHtmlBom::DrawingLayer::Edge) {
        width = UnsignedLength(100000);
      }
      const auto drawingKind = drawingKindMap.value(
          &layer, InteractiveHtmlBom::DrawingKind::Polygon);
      ibom->addDrawing(drawingKind, *drawingLayer, path, width, filled);
    } else if ((copperLayer != layerMap.end()) && (width > 0)) {
      for (int i = 1; i < path.getVertices().count(); ++i) {
        const Point p0 = path.getVertices().at(i - 1).getPos();
        const Point p1 = path.getVertices().at(i).getPos();
        // Note: Arcs not handled yet as we don't use them yet...
        ibom->addTrack(*copperLayer, p0, p1, PositiveLength(*width),
                       std::nullopt);
      }
    }
  };
  for (const auto p : mBoard.getPolygons()) {
    addDrawing(p->getData().getLayer(), p->getData().getPath(),
               p->getData().getLineWidth(), p->getData().isFilled());
  }
  for (const auto t : mBoard.getStrokeTexts()) {
    const Transform transform(t->getData());
    for (const Path& p : t->getPaths()) {
      addDrawing(t->getData().getLayer(), transform.map(p),
                 t->getData().getStrokeWidth(), false);
    }
  }
  for (const auto h : mBoard.getHoles()) {
    for (const Path& p :
         h->getData().getPath()->toOutlineStrokes(h->getData().getDiameter())) {
      addDrawing(Layer::boardCutouts(), p, UnsignedLength(0), false);
    }
  }
  // Currently we do not directly support drawing pads which are not part of
  // a footprint. Also I think we cannot draw copper for documentation purpose
  // only? Let's just draw the pad hole for now, and imrpve it when necessary.
  for (const auto seg : mBoard.getNetSegments()) {
    for (const auto pad : seg->getPads()) {
      const Transform transform(*pad);
      for (const PadHole& hole : pad->getProperties().getHoles()) {
        for (const Path& p :
             hole.getPath()->toOutlineStrokes(hole.getDiameter())) {
          addDrawing(Layer::boardCutouts(), transform.map(p), UnsignedLength(0),
                     false);
        }
      }
    }
  }

  // Add tracks & vias.
  for (const auto seg : mBoard.getNetSegments()) {
    const NetSignal* net = seg->getNetSignal();
    const auto netName =
        net ? std::make_optional(*net->getName()) : std::nullopt;
    for (const auto nl : seg->getNetLines()) {
      if (layerMap.contains(&nl->getLayer())) {
        ibom->addTrack(layerMap[&nl->getLayer()], nl->getP1().getPosition(),
                       nl->getP2().getPosition(), nl->getWidth(), netName);
      }
    }
    for (const auto via : seg->getVias()) {
      QSet<InteractiveHtmlBom::Layer> layers;
      if (via->getVia().isOnLayer(Layer::topCopper())) {
        layers.insert(InteractiveHtmlBom::Layer::Top);
      }
      if (via->getVia().isOnLayer(Layer::botCopper())) {
        layers.insert(InteractiveHtmlBom::Layer::Bottom);
      }
      ibom->addVia(layers, via->getVia().getPosition(), via->getActualSize(),
                   via->getActualDrillDiameter(), netName);
    }
  }

  // Add planes.
  for (const auto& plane : mBoard.getPlanes()) {
    const NetSignal* net = plane->getNetSignal();
    const auto netName =
        net ? std::make_optional(*net->getName()) : std::nullopt;
    if (layerMap.contains(&plane->getLayer())) {
      for (const Path& fragment : plane->getFragments()) {
        ibom->addPlaneFragment(layerMap[&plane->getLayer()], fragment, netName);
      }
    }
  }

  // Add footprints and their drawings.
  struct BomItem {
    QString ref;
    std::size_t footprintId;
    InteractiveHtmlBom::Sides side;  // Top or Bottom (not Both).
  };
  QMap<QStringList, QList<BomItem>> bomItems;
  for (const auto d : mBoard.getDeviceInstances()) {
    const Transform transform(*d);
    const std::pair<Point, Point> bbox =
        d->getLibFootprint().calculateBoundingRect(true);
    QList<InteractiveHtmlBom::Pad> pads;
    for (const auto& p : d->getPads()) {
      const NetSignal* net = p->getNetSignal();
      const bool pin1 = (d->getPads().count() > 1) &&
          (QStringList{"1", "A"}.contains(*p->getLibPackagePad()->getName()));
      pads.append(InteractiveHtmlBom::Pad{
          p->isOnLayer(Layer::topCopper()),
          p->isOnLayer(Layer::botCopper()),
          p->getPosition(),
          p->getRotation(),
          p->getMirrored(),
          p->getGeometries().value(&p->getSolderLayer()),
          p->getProperties().getHoles(),
          net ? std::make_optional(*net->getName()) : std::nullopt,
          pin1,
      });
    }
    const auto parts = d->getParts(mAssemblyVariant->getUuid());
    const bool mount = !parts.isEmpty();
    auto lookup = ProjectAttributeLookup(*d, parts.value(0));
    const QString mpn = lookup("MPN");
    QString valueMpn =
        AttributeSubstitutor::substitute(lookup("VALUE"), lookup);
    if (!valueMpn.contains(mpn)) {
      if (valueMpn.trimmed().isEmpty()) {
        valueMpn = mpn;
      } else {
        valueMpn.append(QString(" (%1)").arg(mpn));
      }
    }
    QStringList fields = {
        valueMpn.split("\n", Qt::SkipEmptyParts).join(" "),
        lookup("PACKAGE"),
    };
    for (const QString& attr : mCustomAttributes) {
      fields.append(AttributeSubstitutor::substitute(lookup(attr), lookup));
    }
    const std::size_t id =
        ibom->addFootprint(d->getMirrored() ? InteractiveHtmlBom::Layer::Bottom
                                            : InteractiveHtmlBom::Layer::Top,
                           d->getPosition(), d->getRotation(), bbox.first,
                           bbox.second, mount, fields, pads);
    if (mount) {
      bomItems[fields].append(BomItem{
          *d->getComponentInstance().getName(),
          id,
          d->getMirrored() ? InteractiveHtmlBom::Sides::Bottom
                           : InteractiveHtmlBom::Sides::Top,
      });
    }

    for (const Polygon& p : d->getLibFootprint().getPolygons()) {
      const Layer& layer = transform.map(p.getLayer());
      addDrawing(layer, transform.map(p.getPathForRendering()),
                 p.getLineWidth(), p.isFilled());
    }
    for (const Circle& c : d->getLibFootprint().getCircles()) {
      const Layer& layer = transform.map(c.getLayer());
      addDrawing(layer,
                 transform.map(
                     Path::circle(c.getDiameter()).translated(c.getCenter())),
                 c.getLineWidth(), c.isFilled());
    }
    for (const auto t : d->getStrokeTexts()) {
      const Transform transform(t->getData());
      for (const Path& p : t->getPaths()) {
        addDrawing(t->getData().getLayer(), transform.map(p),
                   t->getData().getStrokeWidth(), false);
      }
    }
    for (const Hole& h : d->getLibFootprint().getHoles()) {
      for (const Path& p : h.getPath()->toOutlineStrokes(h.getDiameter())) {
        addDrawing(Layer::boardCutouts(), transform.map(p), UnsignedLength(0),
                   false);
      }
    }
  }

  // Sort BOM items.
  QList<QList<BomItem>> sortedItems;
  for (const auto& items : bomItems) {
    QList<BomItem> sorted = items;
    Toolbox::sortNumeric(
        sorted,
        [](const QCollator& cmp, const BomItem& lhs, const BomItem& rhs) {
          return cmp(lhs.ref, rhs.ref);
        },
        Qt::CaseInsensitive, false);
    if (!sorted.isEmpty()) {
      sortedItems.append(sorted);
    }
  }
  auto getPriority = [&](const QList<BomItem>& items) {
    int prio = mComponentOrder.count();
    for (const BomItem& item : items) {
      QString prefix = item.ref;
      while ((!prefix.isEmpty()) && (prefix.back().isDigit())) {
        prefix.chop(1);
      }
      const int idx = mComponentOrder.indexOf(prefix);
      if (idx != -1) {
        prio = std::min(prio, idx);
      }
    }
    return prio;
  };
  Toolbox::sortNumeric(
      sortedItems,
      [&](const QCollator& cmp, const QList<BomItem>& lhs,
          const QList<BomItem>& rhs) {
        const int lhsPrio = getPriority(lhs);
        const int rhsPrio = getPriority(rhs);
        if (lhsPrio != rhsPrio) {
          return lhsPrio < rhsPrio;
        } else {
          return cmp(lhs.first().ref, rhs.first().ref);
        }
      },
      Qt::CaseInsensitive, false);

  // Add BOM rows.
  for (auto sides : {
           InteractiveHtmlBom::Sides::Top,
           InteractiveHtmlBom::Sides::Bottom,
           InteractiveHtmlBom::Sides::Both,
       }) {
    for (const QList<BomItem>& items : sortedItems) {
      QList<std::pair<QString, std::size_t>> parts;
      for (const auto& item : items) {
        if ((item.side == sides) ||
            (sides == InteractiveHtmlBom::Sides::Both)) {
          parts.emplace_back(std::make_pair(item.ref, item.footprintId));
        }
      }
      if (!parts.empty()) {
        ibom->addBomRow(sides, parts);
      }
    }
  }

  return ibom;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
