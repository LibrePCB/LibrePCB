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
#include "../../utils/toolbox.h"
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
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_pad.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"
#include "items/bi_zone.h"

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
 *  General Methods
 ******************************************************************************/

QByteArray BoardSpecctraExport::generate() const {
  // Add all via pad stacks.
  std::vector<std::unique_ptr<SExpression>> fptPadStacks;
  std::vector<std::unique_ptr<SExpression>> viaPadStacks;
  for (const auto& segment : mBoard.getNetSegments()) {
    for (const auto& via : segment->getVias()) {
      addToPadStacks(viaPadStacks, genWiringPadStack(*via));
    }
  }
  // Sort for a more natural order of vias.
  QCollator collator;
  collator.setLocale(QLocale::c());  // Important for locale-independent output.
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  std::sort(viaPadStacks.begin(), viaPadStacks.end(),
            [&collator](const std::unique_ptr<SExpression>& a,
                        const std::unique_ptr<SExpression>& b) {
              return collator(a->getChild(0).getValue(),
                              b->getChild(0).getValue());
            });

  // Project name must not contain spaces since quotation is not activated
  // until the "parser" node appears.
  QString name = *mBoard.getProject().getName();
  if (mBoard.getProject().getBoards().count() > 1) {
    name += " " % *mBoard.getName();
  }
  name = Toolbox::cleanUserInputString(name,
                                       QRegularExpression("[^-a-zA-Z0-9_+.]"),
                                       true, false, false, "-", -1);
  if (name.isEmpty()) name = "unnamed";

  // Build the file.
  std::unique_ptr<SExpression> root = SExpression::createList("pcb");
  root->appendChild(SExpression::createToken(name));
  root->ensureLineBreak();
  root->appendChild(genParser());
  root->ensureLineBreak();
  root->appendChild(genResolution());
  root->ensureLineBreak();
  root->appendChild("unit", SExpression::createToken("mm"));
  root->ensureLineBreak();
  root->appendChild(genStructure(viaPadStacks));
  root->ensureLineBreak();
  root->appendChild(genPlacement());
  root->ensureLineBreak();
  root->appendChild(genLibrary(fptPadStacks, viaPadStacks));
  root->ensureLineBreak();
  root->appendChild(genNetwork());
  root->ensureLineBreak();
  root->appendChild(genWiring());
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
    std::vector<std::unique_ptr<SExpression>>& viaPadStacks) const {
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
      root->appendChild(
          toKeepout("cutout:" + polygon->getData().getUuid().toStr(),
                    polygon->getData().getPath(), {}));
    }
  }
  for (const auto& hole : mBoard.getHoles()) {
    root->ensureLineBreak();
    root->appendChild(toKeepout("hole:" + hole->getData().getUuid().toStr(),
                                hole->getData()));
  }
  foreach (const auto& zone, mBoard.getZones()) {
    if (zone->getData().getRules().testFlag(Zone::Rule::NoCopper)) {
      root->ensureLineBreak();
      root->appendChild(toKeepout("zone:" + zone->getData().getUuid().toStr(),
                                  zone->getData().getOutline(),
                                  zone->getData().getLayers()));
    }
  }

  // Vias.
  root->ensureLineBreak();
  {
    auto& node = root->appendList("via");
    for (const auto& padStack : viaPadStacks) {
      node.ensureLineBreak();
      node.appendChild(padStack->getChild(0).getValue());
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

  // Helper.
  auto addComponent = [&](const QString& imageId, const QString& designator,
                          const Point& pos, const Angle& rotation,
                          bool mirrored) {
    root->ensureLineBreak();
    auto& componentNode = root->appendList("component");
    componentNode.appendChild(imageId);
    componentNode.ensureLineBreak();
    auto& placeNode = componentNode.appendList("place");
    placeNode.appendChild(designator);
    placeNode.appendChild(toToken(pos.getX()));
    placeNode.appendChild(toToken(pos.getY()));
    if (!mirrored) {
      placeNode.appendChild(SExpression::createToken("front"));
    } else {
      placeNode.appendChild(SExpression::createToken("back"));
    }
    placeNode.appendChild(SExpression::createToken(rotation.toDegString()));
    componentNode.ensureLineBreak();
  };

  // Dummy placement for board (which may contain pads).
  addComponent("BOARD", "BOARD", Point(0, 0), Angle::deg0(), false);

  // Real devices.
  for (const BI_Device* dev : mBoard.getDeviceInstances()) {
    const QString imageId = *dev->getComponentInstance().getName() % ":" %
        *dev->getLibPackage().getNames().getDefaultValue();
    addComponent(imageId, *dev->getComponentInstance().getName(),
                 dev->getPosition(), dev->getRotation(), dev->getMirrored());
  }

  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibrary(
    std::vector<std::unique_ptr<SExpression>>& fptPadStacks,
    std::vector<std::unique_ptr<SExpression>>& viaPadStacks) const {
  std::unique_ptr<SExpression> root = SExpression::createList("library");
  root->ensureLineBreak();
  root->appendChild(genLibraryImage(fptPadStacks));
  for (const BI_Device* dev : mBoard.getDeviceInstances()) {
    root->ensureLineBreak();
    root->appendChild(genLibraryImage(*dev, fptPadStacks));
  }
  root->ensureLineBreak();
  for (std::size_t i = 0; i < fptPadStacks.size(); ++i) {
    fptPadStacks[i]->getChild(0).setValue(
        fptPadStacks[i]->getChild(0).getValue() + QString::number(i));
    root->ensureLineBreak();
    root->appendChild(std::move(fptPadStacks[i]));
  }
  for (std::size_t i = 0; i < viaPadStacks.size(); ++i) {
    root->ensureLineBreak();
    root->appendChild(std::move(viaPadStacks[i]));
  }
  fptPadStacks.clear();
  viaPadStacks.clear();
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibraryImage(
    std::vector<std::unique_ptr<SExpression>>& fptPadStacks) const {
  const QString id = "BOARD";

  std::unique_ptr<SExpression> root = SExpression::createList("image");
  root->appendChild(id);
  for (const auto& segment : mBoard.getNetSegments()) {
    for (const auto& pad : segment->getPads()) {
      const QString id = "pad-" +
          QString::number(addToPadStacks(fptPadStacks,
                                         genLibraryPadStack(*pad)));
      root->ensureLineBreak();
      auto& pinNode = root->appendList("pin");
      pinNode.appendChild(id);
      pinNode.appendChild(
          "rotate",
          SExpression::createToken(
              pad->getProperties().getRotation().toDegString()));
      pinNode.appendChild(
          QString(segment->getUuid().toStr() + ":" + pad->getUuid().toStr())
              .replace("-", ""));
      pinNode.appendChild(toToken(pad->getProperties().getPosition().getX()));
      pinNode.appendChild(toToken(pad->getProperties().getPosition().getY()));
    }
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibraryImage(
    const BI_Device& dev,
    std::vector<std::unique_ptr<SExpression>>& fptPadStacks) const {
  const QString id = *dev.getComponentInstance().getName() % ":" %
      *dev.getLibPackage().getNames().getDefaultValue();

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
      root->appendChild(toKeepout(*dev.getComponentInstance().getName() + ":" +
                                      "cutout:" + polygon.getUuid().toStr(),
                                  polygon.getPath(), {}));
    }
  }
  for (const auto& pad : dev.getPads()) {
    const QString id = "pad-" +
        QString::number(addToPadStacks(fptPadStacks, genLibraryPadStack(*pad)));
    root->ensureLineBreak();
    auto& pinNode = root->appendList("pin");
    pinNode.appendChild(id);
    pinNode.appendChild("rotate",
                        SExpression::createToken(
                            pad->getProperties().getRotation().toDegString()));
    pinNode.appendChild(pad->getUuid().toStr().replace("-", ""));
    pinNode.appendChild(toToken(pad->getProperties().getPosition().getX()));
    pinNode.appendChild(toToken(pad->getProperties().getPosition().getY()));
  }
  for (const auto& hole : dev.getLibFootprint().getHoles()) {
    root->ensureLineBreak();
    root->appendChild(toKeepout(*dev.getComponentInstance().getName() + ":" +
                                    "hole:" + hole.getUuid().toStr(),
                                hole));
  }
  for (const auto& zone : dev.getLibFootprint().getZones()) {
    if (zone.getRules().testFlag(Zone::Rule::NoCopper)) {
      root->ensureLineBreak();
      QSet<const Layer*> layers;
      if (zone.getLayers().testFlag(Zone::Layer::Top)) {
        layers.insert(&Layer::topCopper());
      }
      if (zone.getLayers().testFlag(Zone::Layer::Inner)) {
        for (int i = 1; i <= mBoard.getInnerLayerCount(); ++i) {
          layers.insert(Layer::innerCopper(i));
        }
      }
      if (zone.getLayers().testFlag(Zone::Layer::Bottom)) {
        layers.insert(&Layer::botCopper());
      }
      root->appendChild(toKeepout(*dev.getComponentInstance().getName() + ":" +
                                      "zone:" + zone.getUuid().toStr(),
                                  zone.getOutline(), layers));
    }
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibraryPadStack(
    const BI_Pad& pad) const {
  std::unique_ptr<SExpression> root = SExpression::createList("padstack");
  root->appendChild(QString("pad-"));
  root->ensureLineBreak();
  const Transform transform(pad);

  // Determine pad shape.
  typedef std::pair<const Layer*, QList<PadGeometry>> LayerGeometry;
  QList<LayerGeometry> geometries;
  const Layer& solderLayer = pad.getSolderLayer();
  if (pad.getProperties().isTht()) {
    // Always use full THT pad annular rings because automatic annulars depend
    // on whether a trace is connected or not. But connections might be made
    // in an external software, so we don't know which pads will be connected.
    // It's not a nice solution, but safer than exporting too small annular
    // rings. Probably this could be improved with 'reduced_shape_descriptor'?
    const QList<PadGeometry> shapes = pad.getGeometries().value(&solderLayer);
    for (const Layer* layer : mBoard.getCopperLayers()) {
      geometries.append(std::make_pair(layer, shapes));
    }
    // Sort by layer for reproducibility.
    std::sort(geometries.begin(), geometries.end(),
              [](const LayerGeometry& a, const LayerGeometry& b) {
                return a.first->getCopperNumber() < b.first->getCopperNumber();
              });
  } else {
    const QList<PadGeometry> shapes = pad.getGeometries().value(&solderLayer);
    geometries.append(std::make_pair(&transform.map(solderLayer), shapes));
  }

  // Convert pad geometries to Specctra.
  for (const LayerGeometry& pair : geometries) {
    for (const PadGeometry& geometry : pair.second) {
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
        child.appendChild(SExpression::createToken(pair.first->getId()));
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
        node.appendChild(toCircle(pair.first->getId(), PositiveLength(w)));
      } else if ((s == PadGeometry::Shape::Stroke) &&
                 (geometry.getPath().getVertices().count() == 1) && (w > 0)) {
        // Circular pad.
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toCircle(pair.first->getId(), PositiveLength(w)));
      } else if (((s == PadGeometry::Shape::RoundedRect) ||
                  (s == PadGeometry::Shape::RoundedOctagon)) &&
                 (w > h) && (h > 0) && (r >= (h / 2))) {
        // Oblong pad (horizontal).
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toPath(
            pair.first->getId(), UnsignedLength(h),
            Path::line(Point(-(w - h) / 2, 0), Point((w - h) / 2, 0)), false));
      } else if (((s == PadGeometry::Shape::RoundedRect) ||
                  (s == PadGeometry::Shape::RoundedOctagon)) &&
                 (h > w) && (w > 0) && (r >= (w / 2))) {
        // Oblong pad (vertical).
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toPath(
            pair.first->getId(), UnsignedLength(w),
            Path::line(Point(0, (h - w) / 2), Point(0, (h - w) / 2)), false));
      } else if ((s == PadGeometry::Shape::Stroke) &&
                 (geometry.getPath().getVertices().count() > 1) && (w > 0)) {
        // Circular stroke pad.
        root->ensureLineBreak();
        auto& node = root->appendList("shape");
        node.appendChild(toPath(pair.first->getId(), UnsignedLength(w),
                                geometry.getPath(), false));
      } else {
        // Fallback: Arbitrary pads as polygons.
        for (const Path& p : geometry.toOutlines()) {
          root->ensureLineBreak();
          auto& node = root->appendList("shape");
          node.appendChild(
              toPolygon(pair.first->getId(), UnsignedLength(0), p, true));
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

  // Helper.
  auto addNet = [&](const QString& name, const QStringList& pads) {
    root->ensureLineBreak();
    auto& netNode = root->appendList("net");
    netNode.appendChild(name);
    if (!pads.isEmpty()) {
      netNode.ensureLineBreak();
      auto& pinsNode = netNode.appendList("pins");
      for (const QString& pad : pads) {
        pinsNode.ensureLineBreak();
        pinsNode.appendChild(SExpression::createToken(pad));
      }
      pinsNode.ensureLineBreak();
      netNode.ensureLineBreak();
    }
  };

  for (const auto& net : mBoard.getProject().getCircuit().getNetSignals()) {
    QStringList pads;
    for (const auto& cmpSig : net->getComponentSignals()) {
      const QString cmpName = *cmpSig->getComponentInstance().getName();
      for (const auto& pad : cmpSig->getRegisteredFootprintPads()) {
        if (&pad->getBoard() == &mBoard) {
          const QString padId = pad->getUuid().toStr().replace("-", "");
          pads.append(cmpName + "-" + padId);
        }
      }
    }
    for (const auto& segment : net->getBoardNetSegments()) {
      if (&segment->getBoard() == &mBoard) {
        for (const auto& pad : segment->getPads()) {
          const QString padId =
              QString(segment->getUuid().toStr() + ":" + pad->getUuid().toStr())
                  .replace("-", "");
          pads.append("BOARD-" + padId);
        }
      }
    }
    addNet(*net->getName(), pads);
  }

  // For net segments without a net, add a separate dummy net for each of them.
  for (const auto& segment : mBoard.getNetSegments()) {
    if (!segment->getNetSignal()) {
      QStringList pads;
      for (const auto& pad : segment->getPads()) {
        const QString padId =
            QString(segment->getUuid().toStr() + ":" + pad->getUuid().toStr())
                .replace("-", "");
        pads.append("BOARD-" + padId);
      }
      addNet(getNetName(*segment), pads);
    }
  }

  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genWiring() const {
  // Not sure if required, but let's export all wires first, then all vias.
  std::unique_ptr<SExpression> root = SExpression::createList("wiring");
  for (const auto& segment : mBoard.getNetSegments()) {
    for (const auto& trace : segment->getNetLines()) {
      root->ensureLineBreak();
      auto& wireNode = root->appendList("wire");
      wireNode.appendChild(toPath(trace->getLayer().getId(),
                                  positiveToUnsigned(trace->getWidth()),
                                  Path::line(trace->getP1().getPosition(),
                                             trace->getP2().getPosition()),
                                  false));
      wireNode.appendChild("net", getNetName(*segment));
      wireNode.appendChild("type", SExpression::createToken("route"));
    }
  }
  for (const auto& segment : mBoard.getNetSegments()) {
    for (const auto& via : segment->getVias()) {
      root->ensureLineBreak();
      auto& viaNode = root->appendList("via");
      viaNode.appendChild(getWiringPadStackId(*via));
      viaNode.appendChild(toToken(via->getPosition().getX()));
      viaNode.appendChild(toToken(via->getPosition().getY()));
      viaNode.appendChild("net", getNetName(*segment));
      viaNode.appendChild("type", SExpression::createToken("route"));
    }
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genWiringPadStack(
    const BI_Via& via) const {
  std::unique_ptr<SExpression> root = SExpression::createList("padstack");
  root->appendChild(getWiringPadStackId(via));
  QList<const Layer*> layers = Toolbox::sortedQSet(
      mBoard.getCopperLayers(), [](const Layer* a, const Layer* b) {
        return a->getCopperNumber() < b->getCopperNumber();
      });
  for (const Layer* layer : layers) {
    if (via.getVia().isOnLayer(*layer)) {
      root->ensureLineBreak();
      auto& node = root->appendList("shape");
      node.appendChild(toCircle(layer->getId(), via.getActualSize()));
    }
  }
  root->ensureLineBreak();
  root->appendChild("attach", SExpression::createToken("off"));
  root->ensureLineBreak();
  return root;
}

QString BoardSpecctraExport::getWiringPadStackId(const BI_Via& via) const {
  auto getLayerName = [](const Layer& l) {
    if (l == Layer::topCopper()) {
      return QString("top");
    } else if (l == Layer::botCopper()) {
      return QString("bot");
    } else {
      return QString("in%1").arg(l.getCopperNumber());
    }
  };

  // Note: Keep in sync with CmdBoardSpecctraImport::extractViaDrillDiameter(),
  // CmdBoardSpecctraImport::extractViaSize() and
  // CmdBoardSpecctraImport::extractViaExposureConfig().
  QString s = "via";
  if (auto drill = via.getDrillDiameter()) {
    s += "-" % (*drill)->toMmString();
  } else {
    s += "-" % via.getActualDrillDiameter()->toMmString() % ":auto";
  }
  if (auto size = via.getSize()) {
    s += "-" % (*size)->toMmString();
  } else {
    s += "-" % via.getActualSize()->toMmString() % ":auto";
  }
  if (via.getVia().isThrough()) {
    s += "-tht";
  } else {
    s += QString("-%1:%2").arg(
        getLayerName(via.getVia().getStartLayer())
            .arg(getLayerName(via.getVia().getEndLayer())));
  }
  if (auto offset = via.getVia().getExposureConfig().getOffset()) {
    s += QString("-exposed:%1").arg(offset->toMmString());
  } else if (via.getVia().getExposureConfig().isEnabled()) {
    s += "-exposed";
  }
  return s;
}

template <typename THole>
std::unique_ptr<SExpression> BoardSpecctraExport::toKeepout(
    const QString& id, const THole& hole) const {
  std::unique_ptr<SExpression> root = SExpression::createList("keepout");
  root->appendChild(id);
  root->ensureLineBreak();
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
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toKeepout(
    const QString& id, const Path& path,
    const QSet<const Layer*>& layers) const {
  const Length offset =
      *mBoard.getDrcSettings().getMinCopperBoardClearance() + maxArcTolerance();
  ClipperLib::Paths paths{ClipperHelpers::convert(path, maxArcTolerance())};
  ClipperHelpers::offset(paths, offset, maxArcTolerance());
  if (paths.size() != 1) throw LogicError(__FILE__, __LINE__);

  // Sort layers for reproducability.
  QList<const Layer*> layersSorted =
      Toolbox::sortedQSet(layers, [](const Layer* a, const Layer* b) {
        return a->getCopperNumber() < b->getCopperNumber();
      });
  // If all layers are selected, use a simplified version.
  if (Toolbox::toSet(layersSorted) == mBoard.getCopperLayers()) {
    layersSorted.clear();
  }
  if (layersSorted.isEmpty()) {
    layersSorted.append(nullptr);  // All layers.
  }

  std::unique_ptr<SExpression> root = SExpression::createList("keepout");
  root->appendChild(id);
  for (const Layer* layer : layersSorted) {
    root->ensureLineBreak();
    root->appendChild(toPolygon(layer ? layer->getId() : "signal",
                                UnsignedLength(0),
                                ClipperHelpers::convert(paths.at(0)), true));
  }
  root->ensureLineBreak();
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

QString BoardSpecctraExport::getNetName(const BI_NetSegment& ns) noexcept {
  if (auto sig = ns.getNetSignal()) {
    return *sig->getName();
  } else {
    // IMPORTANT: Keep this in sync with the Specctra import!
    return "~anonymous~" % ns.getUuid().toStr();
  }
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
