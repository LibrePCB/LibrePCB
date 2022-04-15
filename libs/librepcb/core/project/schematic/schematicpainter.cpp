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
#include "schematicpainter.h"

#include "../../application.h"
#include "../../attribute/attributesubstitutor.h"
#include "../../export/graphicsexportsettings.h"
#include "../../graphics/graphicslayer.h"
#include "../../graphics/graphicspainter.h"
#include "../../library/sym/symbol.h"
#include "../circuit/netsignal.h"
#include "items/si_netlabel.h"
#include "items/si_netline.h"
#include "items/si_netpoint.h"
#include "items/si_netsegment.h"
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
#include "schematic.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicPainter::SchematicPainter(const Schematic& schematic) noexcept {
  foreach (const SI_Symbol* symbol, schematic.getSymbols()) {
    Symbol sym;
    sym.transform = Transform(*symbol);
    foreach (const SI_SymbolPin* pin, symbol->getPins()) {
      sym.pins.append(Pin{
          pin->getLibPin().getPosition(),
          pin->getLibPin().getRotation(),
          pin->getLibPin().getLength(),
          pin->getDisplayText(),
          pin->getLibPin().getNamePosition(),
      });
      if (pin->isVisibleJunction()) {
        mJunctions.append(pin->getPosition());
      }
    }
    for (const Polygon& polygon : symbol->getLibSymbol().getPolygons()) {
      sym.polygons.append(polygon);
    }
    for (const Circle& circle : symbol->getLibSymbol().getCircles()) {
      sym.circles.append(circle);
    }
    for (const Text& text : symbol->getLibSymbol().getTexts()) {
      Text copy(text);
      copy.setText(AttributeSubstitutor::substitute(copy.getText(), symbol));
      sym.texts.append(copy);
    }
    mSymbols.append(sym);
  }
  foreach (const SI_NetSegment* segment, schematic.getNetSegments()) {
    for (const SI_NetLabel* netlabel : segment->getNetLabels()) {
      mNetLabels.append(Label{netlabel->getPosition(), netlabel->getRotation(),
                              *netlabel->getNetSignalOfNetSegment().getName()});
    }
    for (const SI_NetLine* netline : segment->getNetLines()) {
      mNetLines.append(Line{netline->getStartPoint().getPosition(),
                            netline->getEndPoint().getPosition(),
                            netline->getWidth()});
    }
    for (const SI_NetPoint* netpoint : segment->getNetPoints()) {
      if (netpoint->isVisibleJunction()) {
        mJunctions.append(netpoint->getPosition());
      }
    }
  }
}

SchematicPainter::~SchematicPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicPainter::paint(QPainter& painter,
                             const GraphicsExportSettings& settings) const
    noexcept {
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());

  // Draw Symbols.
  foreach (const Symbol& symbol, mSymbols) {
    // Draw Symbol Polygons.
    foreach (const Polygon& polygon, symbol.polygons) {
      p.drawPolygon(
          symbol.transform.map(polygon.getPath()), *polygon.getLineWidth(),
          settings.getColor(*polygon.getLayerName()),
          settings.getFillColor(*polygon.getLayerName(), polygon.isFilled(),
                                polygon.isGrabArea()));
    }

    // Draw Symbol Circles.
    foreach (const Circle& circle, symbol.circles) {
      p.drawCircle(
          symbol.transform.map(circle.getCenter()), *circle.getDiameter(),
          *circle.getLineWidth(), settings.getColor(*circle.getLayerName()),
          settings.getFillColor(*circle.getLayerName(), circle.isFilled(),
                                circle.isGrabArea()));
    }

    // Draw Symbol Texts.
    foreach (const Text& text, symbol.texts) {
      Alignment alignment = text.getAlign();
      if (symbol.transform.getMirrored()) {
        alignment.mirrorV();
      }
      p.drawText(symbol.transform.map(text.getPosition()),
                 symbol.transform.map(text.getRotation()), *text.getHeight(),
                 alignment, text.getText(), qApp->getDefaultSansSerifFont(),
                 settings.getColor(*text.getLayerName()), false);
    }

    // Draw Symbol Pins.
    foreach (const Pin& pin, symbol.pins) {
      p.drawSymbolPin(symbol.transform.map(pin.position),
                      symbol.transform.map(pin.rotation), *pin.length,
                      settings.getColor(GraphicsLayer::sSymbolOutlines),
                      QColor());
      p.drawText(symbol.transform.map(pin.position +
                                      pin.namePosition.rotated(pin.rotation)),
                 symbol.transform.map(pin.rotation),
                 *SymbolPin::getNameHeight(),
                 Alignment(HAlign::left(), VAlign::center()), pin.name,
                 qApp->getDefaultSansSerifFont(),
                 settings.getColor(GraphicsLayer::sSymbolPinNames), false);
    }
  }

  // Draw Net Lines.
  foreach (const Line& netline, mNetLines) {
    p.drawLine(netline.startPosition, netline.endPosition, *netline.width,
               settings.getColor(GraphicsLayer::sSchematicNetLines));
  }

  // Draw Junctions.
  foreach (const Point& pos, mJunctions) {
    p.drawNetJunction(pos,
                      settings.getColor(GraphicsLayer::sSchematicNetLines));
  }

  // Draw Net Labels.
  foreach (const Label& netlabel, mNetLabels) {
    QFont font = qApp->getDefaultMonospaceFont();
    font.setPixelSize(4);
    p.drawNetLabel(netlabel.position, netlabel.rotation, false, netlabel.text,
                   font, settings.getColor(GraphicsLayer::sSchematicNetLabels));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
