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
#include "../../export/graphicspainter.h"
#include "../../workspace/theme.h"
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

SymbolPainter::SymbolPainter(const Symbol& symbol) noexcept
  : mDefaultFont(Application::getDefaultSansSerifFont()) {
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

  // Draw grab areas first to make them appearing behind every other graphics
  // item. Otherwise they might completely cover (hide) other items.
  for (bool grabArea : {true, false}) {
    // Draw Polygons.
    foreach (const Polygon& polygon, mPolygons) {
      if (polygon.isGrabArea() != grabArea) continue;
      const QString color = polygon.getLayer().getThemeColor();
      p.drawPolygon(polygon.getPath(), *polygon.getLineWidth(),
                    settings.getColor(color),
                    settings.getFillColor(color, polygon.isFilled(),
                                          polygon.isGrabArea()));
    }

    // Draw Circles.
    foreach (const Circle& circle, mCircles) {
      if (circle.isGrabArea() != grabArea) continue;
      const QString color = circle.getLayer().getThemeColor();
      p.drawCircle(
          circle.getCenter(), *circle.getDiameter(), *circle.getLineWidth(),
          settings.getColor(color),
          settings.getFillColor(color, circle.isFilled(), circle.isGrabArea()));
    }
  }

  // Draw Texts.
  foreach (const Text& text, mTexts) {
    const QString color = text.getLayer().getThemeColor();
    p.drawText(text.getPosition(), text.getRotation(), *text.getHeight(),
               text.getAlign(), text.getText(), mDefaultFont,
               settings.getColor(color), true, false);
  }

  // Draw Pins.
  foreach (const SymbolPin& pin, mPins) {
    p.drawSymbolPin(pin.getPosition(), pin.getRotation(), *pin.getLength(),
                    settings.getColor(Theme::Color::sSchematicPinLines),
                    QColor());
    p.drawText(
        pin.getPosition() + pin.getNamePosition().rotated(pin.getRotation()),
        pin.getRotation() + pin.getNameRotation(), *pin.getNameHeight(),
        pin.getNameAlignment(), *pin.getName(), mDefaultFont,
        settings.getColor(Theme::Color::sSchematicPinNames), true, false);
    const bool flipped = Toolbox::isTextUpsideDown(pin.getRotation(), false);
    p.drawText(pin.getPosition() +
                   pin.getNumbersPosition(flipped).rotated(pin.getRotation()),
               pin.getRotation(), *SymbolPin::getNumbersHeight(),
               pin.getNumbersAlignment(flipped), "1…", mDefaultFont,
               settings.getColor(Theme::Color::sSchematicPinNumbers), true,
               false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
