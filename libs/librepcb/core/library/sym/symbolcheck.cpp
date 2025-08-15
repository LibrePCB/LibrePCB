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
#include "symbolcheck.h"

#include "symbol.h"
#include "symbolcheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolCheck::SymbolCheck(const Symbol& symbol) noexcept
  : LibraryElementCheck(symbol), mSymbol(symbol) {
}

SymbolCheck::~SymbolCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

RuleCheckMessageList SymbolCheck::runChecks() const {
  RuleCheckMessageList msgs = LibraryElementCheck::runChecks();
  checkInvalidImageFiles(msgs);
  checkDuplicatePinNames(msgs);
  checkPinNamesInversionSign(msgs);
  checkOffTheGridPins(msgs);
  checkOverlappingPins(msgs);
  checkMissingTexts(msgs);
  checkWrongTextLayers(msgs);
  checkOriginInCenter(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void SymbolCheck::checkInvalidImageFiles(MsgList& msgs) const {
  using Error = MsgInvalidImageFile::Error;
  typedef std::optional<std::pair<Error, QString>> Result;

  auto getError = [this](const Image& image) {
    try {
      if (!mSymbol.getDirectory().fileExists(*image.getFileName())) {
        return std::make_optional(
            std::make_pair(Error::FileMissing, QString()));
      }
      const QByteArray content =
          mSymbol.getDirectory().read(*image.getFileName());  // can throw
      QString error = "Unknown error.";
      if (Image::tryLoad(content, image.getFileExtension(), &error)) {
        return Result();  // Success.
      } else if (!Image::getSupportedExtensions().contains(
                     image.getFileExtension())) {
        return std::make_optional(
            std::make_pair(Error::UnsupportedFormat, error));
      } else {
        return std::make_optional(std::make_pair(Error::ImageLoadError, error));
      }
    } catch (const Exception& e) {
      return std::make_optional(
          std::make_pair(Error::FileReadError, e.getMsg()));
    }
  };

  // Emit the warning only once per filename.
  QMap<QString, Result> errors;
  for (const Image& image : mSymbol.getImages()) {
    if (!errors.contains(*image.getFileName())) {
      errors.insert(*image.getFileName(), getError(image));
    }
  }
  for (auto it = errors.begin(); it != errors.end(); it++) {
    if (it.value()) {
      msgs.append(std::make_shared<MsgInvalidImageFile>(
          it.key(), it.value()->first, it.value()->second));
    }
  }
}

void SymbolCheck::checkDuplicatePinNames(MsgList& msgs) const {
  QSet<CircuitIdentifier> pinNames;
  for (const SymbolPin& pin : mSymbol.getPins()) {
    if (pinNames.contains(pin.getName())) {
      msgs.append(std::make_shared<MsgDuplicatePinName>(pin));
    } else {
      pinNames.insert(pin.getName());
    }
  }
}

void SymbolCheck::checkPinNamesInversionSign(MsgList& msgs) const {
  for (auto it = mSymbol.getPins().begin(); it != mSymbol.getPins().end();
       ++it) {
    const QString name = *it->getName();
    if (name.startsWith("/") ||
        ((name.count() >= 2) && name.startsWith("n") && name.at(1).isUpper())) {
      msgs.append(
          std::make_shared<MsgNonFunctionalSymbolPinInversionSign>(it.ptr()));
    }
  }
}

void SymbolCheck::checkOffTheGridPins(MsgList& msgs) const {
  for (auto it = mSymbol.getPins().begin(); it != mSymbol.getPins().end();
       ++it) {
    PositiveLength grid(2540000);
    if (((*it).getPosition() % (*grid)) != Point(0, 0)) {
      msgs.append(std::make_shared<MsgSymbolPinNotOnGrid>(it.ptr(), grid));
    }
  }
}

void SymbolCheck::checkOverlappingPins(MsgList& msgs) const {
  QHash<Point, QVector<std::shared_ptr<const SymbolPin>>> pinPositions;
  for (auto it = mSymbol.getPins().begin(); it != mSymbol.getPins().end();
       ++it) {
    pinPositions[(*it).getPosition()].append(it.ptr());
  }
  foreach (const auto& pins, pinPositions) {
    if (pins.count() > 1) {
      msgs.append(std::make_shared<MsgOverlappingSymbolPins>(pins));
    }
  }
}

void SymbolCheck::checkMissingTexts(MsgList& msgs) const {
  QHash<QString, QVector<std::shared_ptr<const Text>>> texts;
  for (auto it = mSymbol.getTexts().begin(); it != mSymbol.getTexts().end();
       ++it) {
    texts[(*it).getText()].append(it.ptr());
  }
  if (texts.value("{{NAME}}").isEmpty()) {
    msgs.append(std::make_shared<MsgMissingSymbolName>());
  }
  if (texts.value("{{VALUE}}").isEmpty()) {
    msgs.append(std::make_shared<MsgMissingSymbolValue>());
  }
}

void SymbolCheck::checkWrongTextLayers(MsgList& msgs) const {
  QHash<QString, const Layer*> textLayers = {
      std::make_pair("{{NAME}}", &Layer::symbolNames()),
      std::make_pair("{{VALUE}}", &Layer::symbolValues()),
  };
  for (auto it = mSymbol.getTexts().begin(); it != mSymbol.getTexts().end();
       ++it) {
    const Layer* expectedLayer = textLayers.value((*it).getText());
    if (expectedLayer && (&(*it).getLayer() != expectedLayer)) {
      msgs.append(
          std::make_shared<MsgWrongSymbolTextLayer>(it.ptr(), *expectedLayer));
    }
  }
}

void SymbolCheck::checkOriginInCenter(MsgList& msgs) const {
  // Suppress this warning for symbols which have no pins and no grab areas.
  // This avoids false-positives on very special symbols like schematic frames.
  bool isNormalSymbol = !mSymbol.getPins().isEmpty();
  if (!isNormalSymbol) {
    for (const Circle& c : mSymbol.getCircles()) {
      if ((c.getLayer() == Layer::symbolOutlines()) && (c.isGrabArea())) {
        isNormalSymbol = true;
      }
    }
    for (const Polygon& p : mSymbol.getPolygons()) {
      if ((p.getLayer() == Layer::symbolOutlines()) && (p.isGrabArea())) {
        isNormalSymbol = true;
      }
    }
  }
  if (!isNormalSymbol) {
    return;
  }

  // Determine bounding area of grab area polygons, as they are the best
  // indicator for the symbol body.
  QSet<Length> x, y;
  for (const Polygon& p : mSymbol.getPolygons()) {
    if ((p.getLayer() == Layer::symbolOutlines()) && (!p.isFilled()) &&
        p.isGrabArea() && p.getPath().isClosed() && (!p.getPath().isCurved())) {
      for (const Vertex& v : p.getPath().getVertices()) {
        x.insert(v.getPos().getX());
        y.insert(v.getPos().getY());
      }
    }
  }

  // Only if we didn't find a symbol body, take more objects into account.
  if (x.isEmpty() || y.isEmpty()) {
    for (const SymbolPin& pin : mSymbol.getPins()) {
      x.insert(pin.getPosition().getX());
      y.insert(pin.getPosition().getY());
    }
    for (const Circle& circle : mSymbol.getCircles()) {
      if (circle.getLayer() == Layer::symbolOutlines()) {
        const Length r = circle.getDiameter() / 2;
        x.insert(circle.getCenter().getX() - r);
        x.insert(circle.getCenter().getX() + r);
        y.insert(circle.getCenter().getY() - r);
        y.insert(circle.getCenter().getY() + r);
      }
    }
    for (const Polygon& p : mSymbol.getPolygons()) {
      if (p.getLayer() == Layer::symbolOutlines()) {
        for (const Vertex& v : p.getPath().getVertices()) {
          x.insert(v.getPos().getX());
          y.insert(v.getPos().getY());
        }
      }
    }
  }

  // If there is no boundary, abort.
  if (x.isEmpty() || y.isEmpty()) {
    return;
  }

  // Calculate and check center.
  const Length minX = *std::min_element(x.begin(), x.end());
  const Length maxX = *std::max_element(x.begin(), x.end());
  const Length minY = *std::min_element(y.begin(), y.end());
  const Length maxY = *std::max_element(y.begin(), y.end());
  const Point center((minX + maxX) / 2, (minY + maxY) / 2);
  const Length tolerance = Length(2540000 - 1);  // Not ideal, but good enough?
  if (std::max(center.getX().abs(), center.getY().abs()) > tolerance) {
    msgs.append(std::make_shared<MsgSymbolOriginNotInCenter>(center));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
