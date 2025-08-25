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
#include "../../export/graphicsexportsettings.h"
#include "../../export/graphicspainter.h"
#include "../../library/sym/symbol.h"
#include "../../workspace/theme.h"
#include "../circuit/netsignal.h"
#include "items/si_image.h"
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
                                   QStringList* errors, bool thumbnail) noexcept
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
          pin->getName(),
          pin->getNumbersTruncated(),
          pin->getLibPin().getNamePosition(),
          pin->getLibPin().getNameRotation(),
          pin->getLibPin().getNameHeight(),
          pin->getLibPin().getNameAlignment(),
          pin->getNumbersPosition(),
          pin->getNumbersAlignment(),
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
    for (const Image& image : symbol->getLibSymbol().getImages()) {
      try {
        if (!sym.imageFiles.contains(*image.getFileName())) {
          const QByteArray content = symbol->getLibSymbol().getDirectory().read(
              *image.getFileName());  // can throw
          QString error = "Unknown error.";
          if (auto pix =
                  Image::tryLoad(content, image.getFileExtension(), &error)) {
            sym.imageFiles.insert(*image.getFileName(), *pix);
          } else {
            throw RuntimeError(__FILE__, __LINE__,
                               QString("Failed to load image '%1': %2")
                                   .arg(*image.getFileName(), error));
          }
        }
        sym.images.append(image);
      } catch (const Exception& e) {
        if (errors) {
          errors->append(e.getMsg());
        }
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
  foreach (const SI_Image* image, schematic.getImages()) {
    try {
      const Image& img = *image->getImage();
      if (!mImageFiles.contains(*img.getFileName())) {
        const QByteArray content =
            schematic.getDirectory().read(*img.getFileName());  // can throw
        QString error = "Unknown error.";
        if (auto pix =
                Image::tryLoad(content, img.getFileExtension(), &error)) {
          mImageFiles.insert(*img.getFileName(), *pix);
        } else {
          throw RuntimeError(__FILE__, __LINE__,
                             QString("Failed to load image '%1': %2")
                                 .arg(*img.getFileName(), error));
        }
      }
      mImages.append(img);
    } catch (const Exception& e) {
      if (errors) {
        errors->append(e.getMsg());
      }
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
      mNetLines.append(Line{netline->getP1().getPosition(),
                            netline->getP2().getPosition(),
                            netline->getWidth()});
    }
  }
}

SchematicPainter::~SchematicPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicPainter::paint(
    QPainter& painter, const GraphicsExportSettings& settings) const noexcept {
  GraphicsPainter p(painter);
  p.setMinLineWidth(settings.getMinLineWidth());

  // Draw Symbols.
  foreach (const Symbol& symbol, mSymbols) {
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
    auto drawShapes = [&symbol, &settings, &p, &doDraw](ShapeType type) {
      // Draw Polygons.
      foreach (const Polygon& polygon, symbol.polygons) {
        if (doDraw(type, polygon.isGrabArea(), polygon.isFilled())) {
          const QString color = polygon.getLayer().getThemeColor();
          p.drawPolygon(symbol.transform.map(polygon.getPath()),
                        *polygon.getLineWidth(), settings.getColor(color),
                        settings.getFillColor(color, polygon.isFilled(),
                                              polygon.isGrabArea()));
        }
      }

      // Draw Circles.
      foreach (const Circle& circle, symbol.circles) {
        if (doDraw(type, circle.isGrabArea(), circle.isFilled())) {
          const QString color = circle.getLayer().getThemeColor();
          p.drawCircle(symbol.transform.map(circle.getCenter()),
                       *circle.getDiameter(), *circle.getLineWidth(),
                       settings.getColor(color),
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
    // Note that if the symbol is mirrored, we do not mirror the image but
    // just fit it into its mirrored bounding box because most images are
    // not mirrorable as they might contain text. Most symbol images will
    // probably be company logos (or other logos) anyway which are not
    // intended to be mirrored.
    foreach (const Image& image, symbol.images) {
      Point pos = symbol.transform.map(image.getPosition());
      Angle rot = symbol.transform.mapNonMirrorable(image.getRotation());
      if (symbol.transform.getMirrored()) {
        pos +=
            Point(-image.getWidth(), 0)
                .rotated(-image.getRotation() + symbol.transform.getRotation());
        rot += Angle::deg180();
      }
      p.drawImage(pos, rot,
                  settings.convertImageColors(
                      symbol.imageFiles.value(*image.getFileName())),
                  image.getWidth(), image.getHeight(), image.getBorderWidth(),
                  settings.getColor(Theme::Color::sSchematicImageBorders));
    }

    // Draw line shapes (no fill, no grab area).
    drawShapes(ShapeType::Others);

    // Draw Symbol Pins.
    foreach (const Pin& pin, symbol.pins) {
      p.drawSymbolPin(
          symbol.transform.map(pin.position),
          symbol.transform.mapNonMirrorable(pin.rotation), *pin.length,
          settings.getColor(Theme::Color::sSchematicPinLines), QColor());
      Alignment nameAlignment = pin.nameAlignment;
      if (symbol.transform.getMirrored()) {
        nameAlignment.mirrorV();
      }
      p.drawText(
          symbol.transform.map(pin.position +
                               pin.namePosition.rotated(pin.rotation)),
          symbol.transform.mapNonMirrorable(pin.rotation + pin.nameRotation),
          *pin.nameHeight, nameAlignment, pin.name, mDefaultFont,
          settings.getColor(Theme::Color::sSchematicPinNames), true, false,
          true);
      const Angle numberRot = symbol.transform.mapNonMirrorable(pin.rotation);
      p.drawText(symbol.transform.map(
                     pin.position + pin.numbersPosition.rotated(pin.rotation)),
                 numberRot, *SymbolPin::getNumbersHeight(),
                 pin.numbersAlignment, pin.numbers, mDefaultFont,
                 settings.getColor(Theme::Color::sSchematicPinNumbers), true,
                 false, false);
    }
  }

  // Draw Images.
  foreach (const Image& image, mImages) {
    p.drawImage(
        image.getPosition(), image.getRotation(),
        settings.convertImageColors(mImageFiles.value(*image.getFileName())),
        image.getWidth(), image.getHeight(), image.getBorderWidth(),
        settings.getColor(Theme::Color::sSchematicImageBorders));
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
               settings.getColor(color), true, false, false);
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
