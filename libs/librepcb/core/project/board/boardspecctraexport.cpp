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

#include "../../serialization/sexpression.h"
#include "../../types/layer.h"
#include "board.h"
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
  std::unique_ptr<SExpression> root = SExpression::createList("pcb");
  root->appendChild(SExpression::createToken(mBoard.getUuid().toStr()));
  root->ensureLineBreak();
  {
    auto& node = root->appendList("parser");
    node.ensureLineBreak();
    node.appendChild("string_quote", SExpression::createToken("\""));
    node.ensureLineBreak();
    node.appendChild("space_in_quoted_tokens", SExpression::createToken("on"));
    node.ensureLineBreak();
    node.appendChild("host_cad", qApp->applicationName());
    node.ensureLineBreak();
    node.appendChild("host_version", qApp->applicationVersion());
    node.ensureLineBreak();
  }
  root->ensureLineBreak();
  {
    auto& node = root->appendList("resolution");
    node.appendChild(SExpression::createToken("um"));
    node.appendChild(SExpression::createToken("10"));
  }
  root->ensureLineBreak();
  root->appendChild("unit", SExpression::createToken("mm"));
  root->ensureLineBreak();
  {
    auto& node = root->appendList("structure");
    for (int i = 0; i < (mBoard.getInnerLayerCount() + 2); ++i) {
      node.ensureLineBreak();
      auto& layerNode = node.appendList("layer");
      if (i == 0) {
        layerNode.appendChild(SExpression::createToken("top"));
      } else if (i <= mBoard.getInnerLayerCount()) {
        layerNode.appendChild(SExpression::createToken(QString("in%1").arg(i)));
      } else {
        layerNode.appendChild(SExpression::createToken("bottom"));
      }
      layerNode.ensureLineBreak();
      layerNode.appendChild("type", SExpression::createToken("signal"));
      layerNode.ensureLineBreak();
      auto& propertyNode = layerNode.appendList("property");
      propertyNode.ensureLineBreak();
      propertyNode.appendChild("index",
                               SExpression::createToken(QString::number(i)));
      propertyNode.ensureLineBreak();
      layerNode.ensureLineBreak();
    }
    node.ensureLineBreak();
    {
      auto& boundaryNode = node.appendList("boundary");
      boundaryNode.ensureLineBreak();
      foreach (const auto& polygon, mBoard.getPolygons()) {
        if (polygon->getData().getLayer() == Layer::boardOutlines()) {
          auto& pathNode = boundaryNode.appendList("path");
          pathNode.appendChild(SExpression::createToken("pcb"));
          pathNode.appendChild(SExpression::createToken("0"));
          foreach (const auto& vertex,
                   polygon->getData().getPath().toClosedPath().getVertices()) {
            pathNode.appendChild(SExpression::createToken(
                QString::number(vertex.getPos().getX().toNm() / 100)));
            pathNode.appendChild(SExpression::createToken(
                QString::number(vertex.getPos().getY().toNm() / 100)));
          }
          boundaryNode.ensureLineBreak();
        }
      }
    }
    node.ensureLineBreak();
  }
  root->ensureLineBreak();
  { auto& node = root->appendList("placement"); }
  root->ensureLineBreak();
  { auto& node = root->appendList("library"); }
  root->ensureLineBreak();
  { auto& node = root->appendList("network"); }
  root->ensureLineBreak();
  { auto& node = root->appendList("wiring"); }
  root->ensureLineBreak();
  return root->toByteArray(false);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
