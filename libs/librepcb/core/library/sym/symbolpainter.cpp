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

SymbolPainter::SymbolPainter(const Symbol& symbol, QStringList* errors) noexcept
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
  for (const Image& image : symbol.getImages()) {
    try {
      if (!mImageFiles.contains(*image.getFileName())) {
        const QByteArray content =
            symbol.getDirectory().read(*image.getFileName());  // can throw
        QString error = "Unknown error.";
        if (auto pix =
                Image::tryLoad(content, image.getFileExtension(), &error)) {
          mImageFiles.insert(*image.getFileName(), *pix);
        } else {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Failed to load image '%1': %2")
                                 .arg(*image.getFileName(), error));
        }
      }
      mImages.append(image);
    } catch (const Exception& e) {
      if (errors) {
        errors->append(e.getMsg());
      }
    }
  }
}

SymbolPainter::~SymbolPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SymbolPainter::paint(
    QPainter& painter, const GraphicsExportSettings& settings) const noexcept {
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());

  // Helper to draw grab areas first to make them appearing behind every other
  // graphics item. Otherwise they might completely cover (hide) other items.
  // Images shall be drawn in front of filled polygons/circles, but behind
  // non-filled polygons/circles.
  enum class ShapeType { GrabArea, Filled, Others };
  auto doDraw = [](ShapeType type, bool grabArea, bool filled) {
    if (type == ShapeType::GrabArea) {
      return grabArea && (!filled);
    } else if (type == ShapeType::Filled) {
      return filled;
    } else {
      return (!grabArea) && (!filled);
    }
  };
  auto drawShapes = [this, &settings, &p, &doDraw](ShapeType type) {
    // Draw Polygons.
    foreach (const Polygon& polygon, mPolygons) {
      if (doDraw(type, polygon.isGrabArea(), polygon.isFilled())) {
        const QString color = polygon.getLayer().getThemeColor();
        p.drawPolygon(polygon.getPath(), *polygon.getLineWidth(),
                      settings.getColor(color),
                      settings.getFillColor(color, polygon.isFilled(),
                                            polygon.isGrabArea()));
      }
    }

    // Draw Circles.
    foreach (const Circle& circle, mCircles) {
      if (doDraw(type, circle.isGrabArea(), circle.isFilled())) {
        const QString color = circle.getLayer().getThemeColor();
        p.drawCircle(circle.getCenter(), *circle.getDiameter(),
                     *circle.getLineWidth(), settings.getColor(color),
                     settings.getFillColor(color, circle.isFilled(),
                                           circle.isGrabArea()));
      }
    }
  };

  // Draw grab-area shapes.
  drawShapes(ShapeType::GrabArea);

  // Draw filled shaped.
  drawShapes(ShapeType::Filled);

  // Draw images.
  foreach (const Image& image, mImages) {
    p.drawImage(
        image.getPosition(), image.getRotation(),
        settings.convertImageColors(mImageFiles.value(*image.getFileName())),
        image.getWidth(), image.getHeight(), image.getBorderWidth(),
        settings.getColor(Theme::Color::sSchematicImageBorders));
  }

  // Draw line shapes (no fill, no grab area).
  drawShapes(ShapeType::Others);

  // Draw Texts.
  foreach (const Text& text, mTexts) {
    const QString color = text.getLayer().getThemeColor();
    p.drawText(text.getPosition(), text.getRotation(), *text.getHeight(),
               text.getAlign(), text.getText(), mDefaultFont,
               settings.getColor(color), true, false, false);
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
        settings.getColor(Theme::Color::sSchematicPinNames), true, false, true);
    const bool flipped = Toolbox::isTextUpsideDown(pin.getRotation());
    p.drawText(pin.getPosition() +
                   pin.getNumbersPosition(flipped).rotated(pin.getRotation()),
               pin.getRotation(), *SymbolPin::getNumbersHeight(),
               pin.getNumbersAlignment(flipped), "1…", mDefaultFont,
               settings.getColor(Theme::Color::sSchematicPinNumbers), true,
               false, false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
