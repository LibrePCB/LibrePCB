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
#include "symbolclipboarddata.h"

#include <librepcb/core/application.h>
#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/textgraphicsitem.h>
#include <librepcb/core/library/sym/symbolpingraphicsitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolClipboardData::SymbolClipboardData(const Uuid& symbolUuid,
                                         const Point& cursorPos) noexcept
  : mSymbolUuid(symbolUuid), mCursorPos(cursorPos) {
}

SymbolClipboardData::SymbolClipboardData(const SExpression& node,
                                         const Version& fileFormat)
  : mSymbolUuid(deserialize<Uuid>(node.getChild("symbol/@0"), fileFormat)),
    mCursorPos(node.getChild("cursor_position"), fileFormat),
    mPins(node, fileFormat),
    mPolygons(node, fileFormat),
    mCircles(node, fileFormat),
    mTexts(node, fileFormat) {
}

SymbolClipboardData::~SymbolClipboardData() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<QMimeData> SymbolClipboardData::toMimeData(
    const IF_GraphicsLayerProvider& lp) {
  SExpression sexpr =
      serializeToDomElement("librepcb_clipboard_symbol");  // can throw

  std::unique_ptr<QMimeData> data(new QMimeData());
  data->setImageData(generatePixmap(lp));
  data->setData(getMimeType(), sexpr.toByteArray());
  return data;
}

std::unique_ptr<SymbolClipboardData> SymbolClipboardData::fromMimeData(
    const QMimeData* mime) {
  QByteArray content = mime ? mime->data(getMimeType()) : QByteArray();
  if (!content.isNull()) {
    SExpression root = SExpression::parse(content, FilePath());
    return std::unique_ptr<SymbolClipboardData>(new SymbolClipboardData(
        root, qApp->getFileFormatVersion()));  // can throw
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolClipboardData::serialize(SExpression& root) const {
  root.appendChild(mCursorPos.serializeToDomElement("cursor_position"), true);
  root.appendChild("symbol", mSymbolUuid, true);
  mPins.serialize(root);
  mPolygons.serialize(root);
  mCircles.serialize(root);
  mTexts.serialize(root);
}

QPixmap SymbolClipboardData::generatePixmap(
    const IF_GraphicsLayerProvider& lp) noexcept {
  GraphicsScene scene;
  QVector<std::shared_ptr<QGraphicsItem>> items;
  for (SymbolPin& pin : mPins) {
    items.append(std::make_shared<SymbolPinGraphicsItem>(pin, lp));
  }
  for (Polygon& polygon : mPolygons) {
    items.append(std::make_shared<PolygonGraphicsItem>(polygon, lp));
  }
  for (Circle& circle : mCircles) {
    items.append(std::make_shared<CircleGraphicsItem>(circle, lp));
  }
  for (Text& text : mTexts) {
    items.append(std::make_shared<TextGraphicsItem>(text, lp));
  }
  foreach (const auto& item, items) { scene.addItem(*item); }
  return scene.toPixmap(300);
}

QString SymbolClipboardData::getMimeType() noexcept {
  return QString("application/x-librepcb-clipboard.symbol; version=%1")
      .arg(qApp->applicationVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
