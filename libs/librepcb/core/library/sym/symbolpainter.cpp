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
#include "symbolpainter.h"

#include "../../application.h"
#include "../../export/graphicsexportsettings.h"
#include "../../graphics/graphicslayer.h"
#include "../../graphics/graphicspainter.h"
#include "symbol.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolPainter::SymbolPainter(const Symbol& symbol) noexcept {
  for (const SymbolPin& pin : symbol.getPins()) {
    mPins.append(pin);
  }
  for (const Polygon& polygon : symbol.getPolygons()) {
    mPolygons.append(polygon);
  }
  for (const Circle& circle : symbol.getCircles()) {
    mCircles.append(circle);
  }
  for (const Text& text : symbol.getTexts()) {
    mTexts.append(text);
  }
}

SymbolPainter::~SymbolPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolPainter::paint(QPainter& painter,
                          const GraphicsExportSettings& settings) const
    noexcept {
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());

  // Draw Polygons.
  foreach (const Polygon& polygon, mPolygons) {
    p.drawPolygon(
        polygon.getPath(), *polygon.getLineWidth(),
        settings.getColor(*polygon.getLayerName()),
        settings.getFillColor(*polygon.getLayerName(), polygon.isFilled(),
                              polygon.isGrabArea()));
  }

  // Draw Circles.
  foreach (const Circle& circle, mCircles) {
    p.drawCircle(circle.getCenter(), *circle.getDiameter(),
                 *circle.getLineWidth(),
                 settings.getColor(*circle.getLayerName()),
                 settings.getFillColor(*circle.getLayerName(),
                                       circle.isFilled(), circle.isGrabArea()));
  }

  // Draw Texts.
  foreach (const Text& text, mTexts) {
    QFont font = qApp->getDefaultSansSerifFont();
    font.setPixelSize(qCeil(text.getHeight()->toPx()));
    p.drawText(text.getPosition(), text.getRotation(), *text.getHeight(),
               text.getAlign(), text.getText(), font,
               settings.getColor(*text.getLayerName()), false);
  }

  // Draw Pins.
  foreach (const SymbolPin& pin, mPins) {
    QFont font = qApp->getDefaultSansSerifFont();
    font.setPixelSize(5);
    p.drawSymbolPin(pin.getPosition(), pin.getRotation(), *pin.getLength(),
                    *pin.getName(), font,
                    settings.getColor(GraphicsLayer::sSymbolOutlines), QColor(),
                    settings.getColor(GraphicsLayer::sSymbolPinNames));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
