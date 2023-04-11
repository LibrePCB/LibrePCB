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
#include "../../export/graphicspainter.h"
#include "../../library/sym/symbol.h"
#include "../../workspace/theme.h"
#include "../circuit/netsignal.h"
#include "items/si_netlabel.h"
#include "items/si_netline.h"
#include "items/si_netpoint.h"
#include "items/si_netsegment.h"
#include "items/si_polygon.h"
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
#include "items/si_text.h"
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

SchematicPainter::SchematicPainter(const Schematic& schematic,
                                   bool thumbnail) noexcept
  : mDefaultFont(Application::getDefaultSansSerifFont()),
    mNetLabelFont(Application::getDefaultMonospaceFont()) {
  mNetLabelFont.setPixelSize(4);
  foreach (const SI_Symbol* symbol, schematic.getSymbols()) {
    Symbol sym;
    sym.transform = Transform(*symbol);
    foreach (const SI_SymbolPin* pin, symbol->getPins()) {
      sym.pins.append(Pin{
          pin->getLibPin().getPosition(),
          pin->getLibPin().getRotation(),
          pin->getLibPin().getLength(),
          pin->getText(),
          pin->getLibPin().getNamePosition(),
          pin->getLibPin().getNameRotation(),
          pin->getLibPin().getNameHeight(),
          pin->getLibPin().getNameAlignment(),
      });
      if (pin->isVisibleJunction() && (!thumbnail)) {
        mJunctions.append(pin->getPosition());
      }
    }
    for (const Polygon& polygon : symbol->getLibSymbol().getPolygons()) {
      sym.polygons.append(polygon);
    }
    for (const Circle& circle : symbol->getLibSymbol().getCircles()) {
      sym.circles.append(circle);
    }
    if (!thumbnail) {
      for (const SI_Text* text : symbol->getTexts()) {
        Text copy(text->getTextObj());
        copy.setText(text->getText());  // Memorize substituted text.
        mTexts.append(copy);
      }
    }
    mSymbols.append(sym);
  }
  foreach (const SI_Polygon* polygon, schematic.getPolygons()) {
    mPolygons.append(polygon->getPolygon());
  }
  if (!thumbnail) {
    foreach (const SI_Text* text, schematic.getTexts()) {
      Text copy(text->getTextObj());
      copy.setText(text->getText());  // Memorize substituted text.
      mTexts.append(copy);
    }
  }
  foreach (const SI_NetSegment* segment, schematic.getNetSegments()) {
    if (!thumbnail) {
      for (const SI_NetLabel* netlabel : segment->getNetLabels()) {
        mNetLabels.append(
            Label{netlabel->getPosition(), netlabel->getRotation(),
                  netlabel->getMirrored(),
                  *netlabel->getNetSignalOfNetSegment().getName()});
      }
      for (const SI_NetPoint* netpoint : segment->getNetPoints()) {
        if (netpoint->isVisibleJunction()) {
          mJunctions.append(netpoint->getPosition());
        }
      }
    }
    for (const SI_NetLine* netline : segment->getNetLines()) {
      mNetLines.append(Line{netline->getStartPoint().getPosition(),
                            netline->getEndPoint().getPosition(),
                            netline->getWidth()});
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
    // Draw grab areas first to make them appearing behind every other graphics
    // item. Otherwise they might completely cover (hide) other items.
    for (bool grabArea : {true, false}) {
      // Draw Symbol Polygons.
      foreach (const Polygon& polygon, symbol.polygons) {
        if (polygon.isGrabArea() != grabArea) continue;
        const QString color = polygon.getLayer().getThemeColor();
        p.drawPolygon(symbol.transform.map(polygon.getPath()),
                      *polygon.getLineWidth(), settings.getColor(color),
                      settings.getFillColor(color, polygon.isFilled(),
                                            polygon.isGrabArea()));
      }

      // Draw Symbol Circles.
      foreach (const Circle& circle, symbol.circles) {
        if (circle.isGrabArea() != grabArea) continue;
        const QString color = circle.getLayer().getThemeColor();
        p.drawCircle(symbol.transform.map(circle.getCenter()),
                     *circle.getDiameter(), *circle.getLineWidth(),
                     settings.getColor(color),
                     settings.getFillColor(color, circle.isFilled(),
                                           circle.isGrabArea()));
      }
    }

    // Draw Symbol Pins.
    foreach (const Pin& pin, symbol.pins) {
      p.drawSymbolPin(symbol.transform.map(pin.position),
                      symbol.transform.map(pin.rotation), *pin.length,
                      settings.getColor(Theme::Color::sSchematicPinLines),
                      QColor());
      Alignment nameAlignment = pin.nameAlignment;
      if (symbol.transform.getMirrored()) {
        nameAlignment.mirrorV();
      }
      p.drawText(symbol.transform.map(pin.position +
                                      pin.namePosition.rotated(pin.rotation)),
                 symbol.transform.map(pin.rotation + pin.nameRotation),
                 *pin.nameHeight, nameAlignment, pin.name, mDefaultFont,
                 settings.getColor(Theme::Color::sSchematicPinNames), true,
                 false);
    }
  }

  // Draw Polygons.
  foreach (const Polygon& polygon, mPolygons) {
    const QString color = polygon.getLayer().getThemeColor();
    p.drawPolygon(
        polygon.getPath(), *polygon.getLineWidth(), settings.getColor(color),
        settings.getFillColor(color, polygon.isFilled(), polygon.isGrabArea()));
  }

  // Draw Texts.
  foreach (const Text& text, mTexts) {
    const QString color = text.getLayer().getThemeColor();
    p.drawText(text.getPosition(), text.getRotation(), *text.getHeight(),
               text.getAlign(), text.getText(), mDefaultFont,
               settings.getColor(color), true, false);
  }

  // Draw Net Lines.
  foreach (const Line& netline, mNetLines) {
    p.drawLine(netline.startPosition, netline.endPosition, *netline.width,
               settings.getColor(Theme::Color::sSchematicWires));
  }

  // Draw Junctions.
  foreach (const Point& pos, mJunctions) {
    p.drawNetJunction(pos, settings.getColor(Theme::Color::sSchematicWires));
  }

  // Draw Net Labels.
  foreach (const Label& netlabel, mNetLabels) {
    p.drawNetLabel(netlabel.position, netlabel.rotation, netlabel.mirrored,
                   netlabel.text, mNetLabelFont,
                   settings.getColor(Theme::Color::sSchematicNetLabels));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
