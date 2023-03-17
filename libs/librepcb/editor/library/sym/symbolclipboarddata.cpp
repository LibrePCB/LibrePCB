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

#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/graphicsscene.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/textgraphicsitem.h"
#include "symbolpingraphicsitem.h"

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

SymbolClipboardData::SymbolClipboardData(const SExpression& node)
  : mSymbolUuid(deserialize<Uuid>(node.getChild("symbol/@0"))),
    mCursorPos(node.getChild("cursor_position")),
    mPins(node),
    mPolygons(node),
    mCircles(node),
    mTexts(node) {
}

SymbolClipboardData::~SymbolClipboardData() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<QMimeData> SymbolClipboardData::toMimeData(
    const IF_GraphicsLayerProvider& lp) {
  SExpression root = SExpression::createList("librepcb_clipboard_symbol");
  root.ensureLineBreak();
  mCursorPos.serialize(root.appendList("cursor_position"));
  root.ensureLineBreak();
  root.appendChild("symbol", mSymbolUuid);
  root.ensureLineBreak();
  mPins.serialize(root);
  root.ensureLineBreak();
  mPolygons.serialize(root);
  root.ensureLineBreak();
  mCircles.serialize(root);
  root.ensureLineBreak();
  mTexts.serialize(root);
  root.ensureLineBreak();

  std::unique_ptr<QMimeData> data(new QMimeData());
  data->setImageData(generatePixmap(lp));
  data->setData(getMimeType(), root.toByteArray());
  return data;
}

std::unique_ptr<SymbolClipboardData> SymbolClipboardData::fromMimeData(
    const QMimeData* mime) {
  QByteArray content = mime ? mime->data(getMimeType()) : QByteArray();
  if (!content.isNull()) {
    SExpression root = SExpression::parse(content, FilePath());
    return std::unique_ptr<SymbolClipboardData>(
        new SymbolClipboardData(root));  // can throw
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QPixmap SymbolClipboardData::generatePixmap(
    const IF_GraphicsLayerProvider& lp) noexcept {
  GraphicsScene scene;
  QVector<std::shared_ptr<QGraphicsItem>> items;
  for (auto ptr : mPins.values()) {
    items.append(std::make_shared<SymbolPinGraphicsItem>(ptr, lp));
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
