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
#include "boardspecctraexport.h"

#include "../../library/pkg/footprint.h"
#include "../../library/pkg/footprintpad.h"
#include "../../library/pkg/package.h"
#include "../../serialization/sexpression.h"
#include "../../types/layer.h"
#include "../../utils/clipperhelpers.h"
#include "../../utils/transform.h"
#include "../circuit/circuit.h"
#include "../circuit/componentinstance.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "../projectlibrary.h"
#include "board.h"
#include "drc/boarddesignrulechecksettings.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardSpecctraExport::BoardSpecctraExport(const Board& board) noexcept
  : mBoard(board) {
}

BoardSpecctraExport::~BoardSpecctraExport() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

/*******************************************************************************
 *  Setters
 ******************************************************************************/

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QByteArray BoardSpecctraExport::generate() const {
  // Add via pad stacks.
  std::vector<std::unique_ptr<SExpression>> padStacks;
  QList<std::tuple<Point, QString, std::size_t>> vias;
  for (const auto& segment : mBoard.getNetSegments()) {
    for (const auto& via : segment->getVias()) {
      const std::size_t id = addToPadStacks(padStacks, genWiringPadStack(*via));
      vias.append(std::make_tuple(via->getPosition(),
                                  *segment->getNetSignal()->getName(), id));
    }
  }

  std::unique_ptr<SExpression> root = SExpression::createList("pcb");
  root->appendChild(*mBoard.getProject().getName() + " - " + *mBoard.getName());
  root->ensureLineBreak();
  root->appendChild(genParser());
  root->ensureLineBreak();
  root->appendChild(genResolution());
  root->ensureLineBreak();
  root->appendChild("unit", SExpression::createToken("mm"));
  root->ensureLineBreak();
  root->appendChild(genStructure(padStacks));
  root->ensureLineBreak();
  root->appendChild(genPlacement());
  root->ensureLineBreak();
  root->appendChild(genLibrary(padStacks));
  root->ensureLineBreak();
  root->appendChild(genNetwork());
  root->ensureLineBreak();
  root->appendChild(genWiring(vias));
  root->ensureLineBreak();
  return root->toByteArray(SExpression::Mode::Permissive);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

std::unique_ptr<SExpression> BoardSpecctraExport::genParser() const {
  std::unique_ptr<SExpression> root = SExpression::createList("parser");
  root->ensureLineBreak();
  root->appendChild("string_quote", SExpression::createToken("\""));
  root->ensureLineBreak();
  root->appendChild("space_in_quoted_tokens", SExpression::createToken("on"));
  root->ensureLineBreak();
  root->appendChild("host_cad", qApp->applicationName());
  root->ensureLineBreak();
  root->appendChild("host_version", qApp->applicationVersion());
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genResolution() const {
  std::unique_ptr<SExpression> root = SExpression::createList("resolution");
  root->appendChild(SExpression::createToken("mm"));
  root->appendChild(SExpression::createToken("1000000"));
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genStructure(
    std::vector<std::unique_ptr<SExpression>>& padStacks) const {
  std::unique_ptr<SExpression> root = SExpression::createList("structure");

  // Layers.
  for (int i = 0; i < (mBoard.getInnerLayerCount() + 2); ++i) {
    root->ensureLineBreak();
    auto& node = root->appendList("layer");
    if (i == 0) {
      node.appendChild(SExpression::createToken(Layer::topCopper().getId()));
    } else if (i <= mBoard.getInnerLayerCount()) {
      node.appendChild(
          SExpression::createToken(Layer::innerCopper(i)->getId()));
    } else {
      node.appendChild(SExpression::createToken(Layer::botCopper().getId()));
    }
    node.appendChild("type", SExpression::createToken("signal"));
  }

  // PCB boundary.
  foreach (const auto& polygon, mBoard.getPolygons()) {
    if (polygon->getData().getLayer() == Layer::boardOutlines()) {
      root->ensureLineBreak();
      auto& node = root->appendList("boundary");
      node.ensureLineBreak();
      node.appendChild(
          toPath("pcb", UnsignedLength(0), polygon->getData().getPath(), true));
      node.ensureLineBreak();
    }
  }

  // Planes.
  foreach (const auto& plane, mBoard.getPlanes()) {
    if (auto net = plane->getNetSignal()) {
      root->ensureLineBreak();
      auto& node = root->appendList("plane");
      node.appendChild(net->getName());
      node.ensureLineBreak();
      node.appendChild(toPolygon(plane->getLayer().getId(), UnsignedLength(0),
                                 plane->getOutline(), true));
      node.ensureLineBreak();
    }
  }

  // Keepout areas.
  foreach (const auto& polygon, mBoard.getPolygons()) {
    if (polygon->getData().getLayer() == Layer::boardCutouts()) {
      root->ensureLineBreak();
      root->appendChild(toKeepout(polygon->getData().getPath()));
    }
  }
  for (const auto& hole : mBoard.getHoles()) {
    root->ensureLineBreak();
    root->appendChild(toKeepout(hole->getData()));
  }

  // Vias.
  root->ensureLineBreak();
  {
    auto& node = root->appendList("via");
    for (std::size_t i = 0; i < padStacks.size(); ++i) {
      node.ensureLineBreak();
      node.appendChild("via-" + QString::number(i));
    }
    node.ensureLineBreak();
  }

  root->ensureLineBreak();
  root->appendChild(genStructureRule());
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genStructureRule() const {
  std::unique_ptr<SExpression> root = SExpression::createList("rule");
  root->ensureLineBreak();
  root->appendChild("width",
                    toToken(*mBoard.getDrcSettings().getMinCopperWidth()));
  root->ensureLineBreak();
  root->appendChild(
      "clearance",
      toToken(*mBoard.getDrcSettings().getMinCopperCopperClearance()));
  root->ensureLineBreak();
  {
    auto& node = root->appendList("clearance");
    node.appendChild(toToken(Length(0)));
    node.appendChild("type", SExpression::createToken("smd_via_same_net"));
  }
  root->ensureLineBreak();
  {
    auto& node = root->appendList("clearance");
    node.appendChild(toToken(Length(0)));
    node.appendChild("type", SExpression::createToken("via_via_same_net"));
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genPlacement() const {
  std::unique_ptr<SExpression> root = SExpression::createList("placement");
  for (const BI_Device* dev : mBoard.getDeviceInstances()) {
    const QString imageId = *dev->getLibPackage().getNames().getDefaultValue() +
        "(" + *dev->getComponentInstance().getName() + ")";
    root->ensureLineBreak();
    auto& componentNode = root->appendList("component");
    componentNode.appendChild(imageId);
    componentNode.ensureLineBreak();
    auto& placeNode = componentNode.appendList("place");
    placeNode.appendChild(*dev->getComponentInstance().getName());
    placeNode.appendChild(toToken(dev->getPosition().getX()));
    placeNode.appendChild(toToken(dev->getPosition().getY()));
    if (!dev->getMirrored()) {
      placeNode.appendChild(SExpression::createToken("front"));
    } else {
      placeNode.appendChild(SExpression::createToken("back"));
    }
    placeNode.appendChild(
        SExpression::createToken(dev->getRotation().toDegString()));
    componentNode.ensureLineBreak();
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibrary(
    std::vector<std::unique_ptr<SExpression>>& padStacks) const {
  std::unique_ptr<SExpression> root = SExpression::createList("library");
  for (const BI_Device* dev : mBoard.getDeviceInstances()) {
    root->ensureLineBreak();
    root->appendChild(genLibraryImage(*dev, padStacks));
  }
  root->ensureLineBreak();
  for (std::size_t i = 0; i < padStacks.size(); ++i) {
    padStacks[i]->getChild(0).setValue(padStacks[i]->getChild(0).getValue() +
                                       QString::number(i));
    root->ensureLineBreak();
    root->appendChild(std::move(padStacks[i]));
  }
  padStacks.clear();
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibraryImage(
    const BI_Device& dev,
    std::vector<std::unique_ptr<SExpression>>& padStacks) const {
  const QString id = *dev.getLibPackage().getNames().getDefaultValue() + "(" +
      *dev.getComponentInstance().getName() + ")";

  std::unique_ptr<SExpression> root = SExpression::createList("image");
  root->appendChild(id);
  for (const auto& polygon : dev.getLibFootprint().getPolygons()) {
    if (polygon.getLayer() == Layer::topDocumentation()) {
      root->ensureLineBreak();
      auto& outlineNode = root->appendList("outline");
      Path path = polygon.getPath();
      if (polygon.getLayer().getPolygonsRepresentAreas()) {
        path.close();
      }
      outlineNode.appendChild(
          toPath("signal", polygon.getLineWidth(), path, true));
      outlineNode.ensureLineBreak();
    } else if (polygon.getLayer() == Layer::boardCutouts()) {
      root->ensureLineBreak();
      root->appendChild(toKeepout(polygon.getPath()));
    }
  }
  for (const auto& pad : dev.getPads()) {
    const QString id = "pad-" +
        QString::number(addToPadStacks(padStacks, genLibraryPadStack(*pad)));
    root->ensureLineBreak();
    auto& pinNode = root->appendList("pin");
    pinNode.appendChild(id);
    pinNode.appendChild(
        "rotate",
        SExpression::createToken(pad->getLibPad().getRotation().toDegString()));
    pinNode.appendChild(pad->getLibPadUuid().toStr().replace("-", ""));
    pinNode.appendChild(toToken(pad->getLibPad().getPosition().getX()));
    pinNode.appendChild(toToken(pad->getLibPad().getPosition().getY()));
  }
  for (const auto& hole : dev.getLibFootprint().getHoles()) {
    root->ensureLineBreak();
    root->appendChild(toKeepout(hole));
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibraryPadStack(
    const BI_FootprintPad& pad) const {
  std::unique_ptr<SExpression> root = SExpression::createList("padstack");
  root->appendChild(QString("pad-"));
  root->ensureLineBreak();
  const Transform transform(pad);
  const auto& geometries = pad.getGeometries();
  for (auto it = geometries.begin(); it != geometries.end(); it++) {
    if (!it.key()->isCopper()) continue;
    const Layer& layer = transform.map(*it.key());

    // Always use full THT pad annular rings because automatic annulars depend
    // on whether a trace is connected or not. But connections might be made
    // in an external software, so we don't know which pads will be connected.
    // It's not a nice solution, but safer than exporting too small annular
    // rings. Probably this could be improved with 'reduced_shape_descriptor'?
    QList<PadGeometry> shapes = it.value();
    if (pad.getLibPad().isTht()) {
      const Layer* solderLayer =
          (pad.getComponentSide() == FootprintPad::ComponentSide::Top)
          ? &Layer::botCopper()
          : &Layer::topCopper();
      shapes = geometries.value(solderLayer);
    }

    for (const PadGeometry& geometry : shapes) {
      const PadGeometry::Shape s = geometry.getShape();
      const Length w = geometry.getWidth();
      const Length h = geometry.getHeight();
      const Length r = *geometry.getCornerRadius();
      if ((s == PadGeometry::Shape::RoundedRect) && (w > 0) && (h > 0) &&
          (r == 0)) {
        // Rectangular pad.
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        auto& child = node.appendList("rect");
        child.appendChild(SExpression::createToken(layer.getId()));
        child.appendChild(toToken(-w / 2));
        child.appendChild(toToken(-h / 2));
        child.appendChild(toToken(w / 2));
        child.appendChild(toToken(h / 2));
      } else if (((s == PadGeometry::Shape::RoundedRect) ||
                  (s == PadGeometry::Shape::RoundedOctagon)) &&
                 (w == h) && (w > 0) && (r >= (w / 2))) {
        // Circular pad.
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toCircle(layer.getId(), PositiveLength(w)));
      } else if ((s == PadGeometry::Shape::Stroke) &&
                 (geometry.getPath().getVertices().count() == 1) && (w > 0)) {
        // Circular pad.
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toCircle(layer.getId(), PositiveLength(w)));
      } else if (((s == PadGeometry::Shape::RoundedRect) ||
                  (s == PadGeometry::Shape::RoundedOctagon)) &&
                 (w > h) && (h > 0) && (r >= (h / 2))) {
        // Oblong pad (horizontal).
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toPath(
            layer.getId(), UnsignedLength(h),
            Path::line(Point(-(w - h) / 2, 0), Point((w - h) / 2, 0)), false));
      } else if (((s == PadGeometry::Shape::RoundedRect) ||
                  (s == PadGeometry::Shape::RoundedOctagon)) &&
                 (h > w) && (w > 0) && (r >= (w / 2))) {
        // Oblong pad (vertical).
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toPath(
            layer.getId(), UnsignedLength(w),
            Path::line(Point(0, (h - w) / 2), Point(0, (h - w) / 2)), false));
      } else if ((s == PadGeometry::Shape::Stroke) &&
                 (geometry.getPath().getVertices().count() > 1) && (w > 0)) {
        // Circular stroke pad.
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toPath(layer.getId(), UnsignedLength(w),
                                geometry.getPath(), false));
      } else {
        // Fallback: Arbitrary pads as polygons.
        for (const Path& p : geometry.toOutlines()) {
          root->ensureLineBreak();
          auto& node = root->appendList("shape");
          node.appendChild(
              toPolygon(layer.getId(), UnsignedLength(0), p, true));
        }
      }
    }
  }
  root->ensureLineBreak();
  root->appendChild("attach", SExpression::createToken("off"));
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genNetwork() const {
  std::unique_ptr<SExpression> root = SExpression::createList("network");
  for (const auto& net : mBoard.getProject().getCircuit().getNetSignals()) {
    root->ensureLineBreak();
    auto& netNode = root->appendList("net");
    netNode.appendChild(net->getName());
    netNode.ensureLineBreak();
    auto& pinsNode = netNode.appendList("pins");
    for (const auto& cmpSig : net->getComponentSignals()) {
      const QString cmpName = *cmpSig->getComponentInstance().getName();
      for (const auto& pad : cmpSig->getRegisteredFootprintPads()) {
        const QString padId = pad->getLibPadUuid().toStr().replace("-", "");
        pinsNode.ensureLineBreak();
        pinsNode.appendChild(SExpression::createToken(cmpName + "-" + padId));
      }
    }
    pinsNode.ensureLineBreak();
    netNode.ensureLineBreak();
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genWiring(
    const QList<std::tuple<Point, QString, std::size_t>>& vias) const {
  std::unique_ptr<SExpression> root = SExpression::createList("wiring");
  for (const auto& segment : mBoard.getNetSegments()) {
    for (const auto& trace : segment->getNetLines()) {
      root->ensureLineBreak();
      auto& wireNode = root->appendList("wire");
      wireNode.appendChild(toPath(
          trace->getLayer().getId(), positiveToUnsigned(trace->getWidth()),
          Path::line(trace->getStartPoint().getPosition(),
                     trace->getEndPoint().getPosition()),
          false));
      wireNode.appendChild("net", *segment->getNetSignal()->getName());
      wireNode.appendChild("type", SExpression::createToken("protect"));
    }
  }
  for (const auto& via : vias) {
    root->ensureLineBreak();
    auto& viaNode = root->appendList("via");
    viaNode.appendChild("via-" + QString::number(std::get<2>(via)));
    viaNode.appendChild(toToken(std::get<0>(via).getX()));
    viaNode.appendChild(toToken(std::get<0>(via).getY()));
    viaNode.appendChild("net", std::get<1>(via));
    viaNode.appendChild("type", SExpression::createToken("protect"));
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genWiringPadStack(
    const BI_Via& via) const {
  std::unique_ptr<SExpression> root = SExpression::createList("padstack");
  root->appendChild(QString("via-"));
  for (const Layer* layer : mBoard.getCopperLayers()) {
    if (via.getVia().isOnLayer(*layer)) {
      root->ensureLineBreak();
      auto& node = root->appendList("shape");
      node.appendChild(toCircle(layer->getId(), via.getSize()));
    }
  }
  root->ensureLineBreak();
  root->appendChild("attach", SExpression::createToken("off"));
  root->ensureLineBreak();
  return root;
}

template <typename THole>
std::unique_ptr<SExpression> BoardSpecctraExport::toKeepout(
    const THole& hole) const {
  std::unique_ptr<SExpression> root = SExpression::createList("keepout");
  const PositiveLength width(
      hole.getDiameter() +
      mBoard.getDrcSettings().getMinCopperNpthClearance() * 2);
  if (hole.isSlot()) {
    root->appendChild(
        toPath("signal", positiveToUnsigned(width), *hole.getPath(), true));
  } else {
    root->appendChild(toCircle("signal", width,
                               hole.getPath()->getVertices().first().getPos()));
  }
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toKeepout(
    const Path& cutout) const {
  const Length offset =
      *mBoard.getDrcSettings().getMinCopperBoardClearance() + maxArcTolerance();
  ClipperLib::Paths paths{ClipperHelpers::convert(cutout, maxArcTolerance())};
  ClipperHelpers::offset(paths, offset, maxArcTolerance());
  if (paths.size() != 1) throw LogicError(__FILE__, __LINE__);

  std::unique_ptr<SExpression> root = SExpression::createList("keepout");
  root->appendChild(toPolygon("signal", UnsignedLength(0),
                              ClipperHelpers::convert(paths.at(0)), true));
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toPolygon(
    const QString& layer, const UnsignedLength& width, const Path& path,
    bool multiline) const {
  std::unique_ptr<SExpression> root = toPath(layer, width, path, multiline);
  root->setName("polygon");
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toPath(
    const QString& layer, const UnsignedLength& width, const Path& path,
    bool multiline) const {
  std::unique_ptr<SExpression> root = SExpression::createList("path");
  root->appendChild(SExpression::createToken(layer));
  root->appendChild(toToken(*width));
  const Path flattenedPath = path.flattenedArcs(PositiveLength(5000));
  foreach (const auto& vertex, flattenedPath.getVertices()) {
    if (multiline) root->ensureLineBreak();
    root->appendChild(toToken(vertex.getPos().getX()));
    root->appendChild(toToken(vertex.getPos().getY()));
  }
  if (multiline) root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toCircle(
    const QString& layer, const PositiveLength& diameter,
    const Point& pos) const {
  std::unique_ptr<SExpression> root = SExpression::createList("circle");
  root->appendChild(SExpression::createToken(layer));
  root->appendChild(toToken(*diameter));
  if (!pos.isOrigin()) {
    root->appendChild(toToken(pos.getX()));
    root->appendChild(toToken(pos.getY()));
  }
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toToken(
    const Length& length) const {
  return SExpression::createToken(length.toMmString());
}

std::size_t BoardSpecctraExport::addToPadStacks(
    std::vector<std::unique_ptr<SExpression>>& padStacks,
    std::unique_ptr<SExpression> padStack) {
  for (std::size_t i = 0; i < padStacks.size(); ++i) {
    if (*padStacks.at(i) == *padStack) {
      return i;
    }
  }
  padStacks.emplace_back(std::move(padStack));
  return padStacks.size() - 1;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
