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
  std::map<QString, ComponentData> components;
  std::vector<std::unique_ptr<SExpression>> padStacks;
  for (const BI_Device* dev : mBoard.getDeviceInstances()) {
    const QString imageId = dev->getLibPackage().getUuid().toStr() +
        "::" + dev->getLibFootprint().getUuid().toStr();
    auto it = components.find(imageId);
    if (it == components.end()) {
      ComponentData data{
          toImage(imageId, dev->getLibFootprint(), padStacks),
          {},
      };
      it = components.insert(std::pair(imageId, std::move(data))).first;
    }
    it->second.placements.push_back(toPlacement(*dev));
  }
  for (std::size_t i = 0; i < padStacks.size(); ++i) {
    padStacks.at(i)->getChild(0).setValue(QString::number(i));
  }

  std::unique_ptr<SExpression> root = SExpression::createList("pcb");
  root->appendChild(*mBoard.getProject().getName() + " - " + *mBoard.getName());
  root->ensureLineBreak();
  root->appendChild(genParser());
  root->ensureLineBreak();
  root->appendChild(genResolution());
  root->ensureLineBreak();
  root->appendChild(genStructure());
  root->ensureLineBreak();
  root->appendChild(genPlacement(components));
  root->ensureLineBreak();
  root->appendChild(genLibrary(components, padStacks));
  root->ensureLineBreak();
  root->appendChild(genNetwork());
  root->ensureLineBreak();
  root->appendChild(genWiring());
  root->ensureLineBreak();
  return root->toByteArray(false);
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
  root->appendChild(SExpression::createToken("um"));
  root->appendChild(SExpression::createToken("10"));
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genStructure() const {
  std::unique_ptr<SExpression> root = SExpression::createList("structure");
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
  root->ensureLineBreak();
  {
    auto& boundaryNode = root->appendList("boundary");
    boundaryNode.ensureLineBreak();
    foreach (const auto& polygon, mBoard.getPolygons()) {
      if (polygon->getData().getLayer() == Layer::boardOutlines()) {
        boundaryNode.appendChild(
            toPath("pcb", UnsignedLength(0),
                   polygon->getData().getPath().toClosedPath(), true));
        boundaryNode.ensureLineBreak();
      }
    }
  }
  root->ensureLineBreak();
  root->appendChild("via", SExpression::createToken("0"));
  root->ensureLineBreak();
  {
    auto& ruleNode = root->appendList("rule");
    ruleNode.ensureLineBreak();
    ruleNode.appendChild("width",
                         toToken(*mBoard.getDrcSettings().getMinCopperWidth()));
    ruleNode.ensureLineBreak();
    ruleNode.appendChild(
        "clearance",
        toToken(*mBoard.getDrcSettings().getMinCopperCopperClearance()));
    ruleNode.ensureLineBreak();
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genPlacement(
    std::map<QString, ComponentData>& components) const {
  std::unique_ptr<SExpression> root = SExpression::createList("placement");
  for (auto& it : components) {
    root->ensureLineBreak();
    auto& componentNode = root->appendList("component");
    componentNode.appendChild(it.first);
    for (auto& ptr : it.second.placements) {
      componentNode.ensureLineBreak();
      componentNode.appendChild(std::move(ptr));
    }
    it.second.placements.clear();  // Avoid nullpointers.
    componentNode.ensureLineBreak();
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::genLibrary(
    std::map<QString, ComponentData>& components,
    std::vector<std::unique_ptr<SExpression>>& padStacks) const {
  std::unique_ptr<SExpression> root = SExpression::createList("library");
  for (auto& it : components) {
    root->ensureLineBreak();
    root->appendChild(std::move(it.second.image));
  }
  components.clear();  // Avoid nullpointers.
  for (auto& ptr : padStacks) {
    root->ensureLineBreak();
    root->appendChild(std::move(ptr));
  }
  padStacks.clear();  // Avoid nullpointers.
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

std::unique_ptr<SExpression> BoardSpecctraExport::genWiring() const {
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
    for (const auto& via : segment->getVias()) {
      root->ensureLineBreak();
      auto& viaNode = root->appendList("via");
      viaNode.appendChild(SExpression::createToken("0"));
      viaNode.appendChild(toToken(via->getPosition().getX()));
      viaNode.appendChild(toToken(via->getPosition().getY()));
      viaNode.appendChild("net", *segment->getNetSignal()->getName());
      viaNode.appendChild("type", SExpression::createToken("protect"));
    }
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toPlacement(
    const BI_Device& dev) const {
  std::unique_ptr<SExpression> root = SExpression::createList("place");
  root->appendChild(*dev.getComponentInstance().getName());
  root->appendChild(toToken(dev.getPosition().getX()));
  root->appendChild(toToken(dev.getPosition().getY()));
  if (!dev.getMirrored()) {
    root->appendChild(SExpression::createToken("front"));
  } else {
    root->appendChild(SExpression::createToken("back"));
  }
  root->appendChild(SExpression::createToken(dev.getRotation().toDegString()));
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toImage(
    const QString& id, const Footprint& footprint,
    std::vector<std::unique_ptr<SExpression>>& padStacks) const {
  auto addToPadStack = [&padStacks](std::unique_ptr<SExpression> padStack) {
    for (std::size_t i = 0; i < padStacks.size(); ++i) {
      if (*padStacks.at(i) == *padStack) {
        return i;
      }
    }
    padStacks.emplace_back(std::move(padStack));
    return padStacks.size() - 1;
  };

  std::unique_ptr<SExpression> root = SExpression::createList("image");
  root->appendChild(id);
  for (const auto& polygon : footprint.getPolygons()) {
    root->ensureLineBreak();
    auto& outlineNode = root->appendList("outline");
    Path path = polygon.getPath();
    if (polygon.getLayer().getPolygonsRepresentAreas()) {
      path.close();
    }
    outlineNode.appendChild(
        toPath("signal", polygon.getLineWidth(), path, true));
    outlineNode.ensureLineBreak();
  }
  for (const auto& pad : footprint.getPads()) {
    const std::size_t pinId = addToPadStack(toPadStack(pad));
    root->ensureLineBreak();
    auto& pinNode = root->appendList("pin");
    pinNode.appendChild(SExpression::createToken(QString::number(pinId)));
    pinNode.appendChild(pad.getUuid().toStr().replace("-", ""));
    pinNode.appendChild(toToken(pad.getPosition().getX()));
    pinNode.appendChild(toToken(pad.getPosition().getY()));
  }
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toPath(
    const QString& layer, const UnsignedLength& width, const Path& path,
    bool multiline) const {
  std::unique_ptr<SExpression> root = SExpression::createList("path");
  root->appendChild(SExpression::createToken(layer));
  root->appendChild(toToken(*width));
  foreach (const auto& vertex, path.getVertices()) {
    if (multiline) root->ensureLineBreak();
    root->appendChild(toToken(vertex.getPos().getX()));
    root->appendChild(toToken(vertex.getPos().getY()));
  }
  if (multiline) root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toPadStack(
    const BI_FootprintPad& pad) const {
  std::unique_ptr<SExpression> root = SExpression::createList("padstack");
  root->appendChild(QString("placeholder"));  // Will be replaced later.
  root->ensureLineBreak();
  const auto& geometries = pad.getGeometries();
  for (auto it = geometries.begin(); it != geometries.end(); it++) {
    auto& node = root->appendList("shape");
    auto& child = node.appendList("circle");
    child.appendChild(SExpression::createToken(Layer::topCopper().getId()));
    child.appendChild(SExpression::createToken("1600"));
  }
  root->ensureLineBreak();
  root->appendChild("attach", SExpression::createToken("off"));
  root->ensureLineBreak();
  return root;
}

std::unique_ptr<SExpression> BoardSpecctraExport::toToken(
    const Length& length) const {
  return SExpression::createToken(QString::number(length.toNm() / 109));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
