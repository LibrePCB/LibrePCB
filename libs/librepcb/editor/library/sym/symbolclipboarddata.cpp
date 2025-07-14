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
#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/graphicsscene.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/textgraphicsitem.h"
#include "symbolpingraphicsitem.h"

#include <librepcb/core/application.h>

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

std::unique_ptr<QMimeData> SymbolClipboardData::toMimeData() {
  std::unique_ptr<SExpression> root =
      SExpression::createList("librepcb_clipboard_symbol");
  root->ensureLineBreak();
  mCursorPos.serialize(root->appendList("cursor_position"));
  root->ensureLineBreak();
  root->appendChild("symbol", mSymbolUuid);
  root->ensureLineBreak();
  mPins.serialize(*root);
  root->ensureLineBreak();
  mPolygons.serialize(*root);
  root->ensureLineBreak();
  mCircles.serialize(*root);
  root->ensureLineBreak();
  mTexts.serialize(*root);
  root->ensureLineBreak();

  const QByteArray sexpr = root->toByteArray();
  std::unique_ptr<QMimeData> data(new QMimeData());
  data->setImageData(generatePixmap());
  data->setData(getMimeType(), sexpr);
  // Note: At least on one system the clipboard didn't work if no text was
  // set, so let's also copy the SExpression as text as a workaround. This
  // might be useful anyway, e.g. for debugging purposes.
  data->setText(QString::fromUtf8(sexpr));
  return data;
}

std::unique_ptr<SymbolClipboardData> SymbolClipboardData::fromMimeData(
    const QMimeData* mime) {
  QByteArray content = mime ? mime->data(getMimeType()) : QByteArray();
  if (!content.isNull()) {
    const std::unique_ptr<const SExpression> root =
        SExpression::parse(content, FilePath());
    return std::unique_ptr<SymbolClipboardData>(
        new SymbolClipboardData(*root));  // can throw
  } else {
    return nullptr;
  }
}

bool SymbolClipboardData::isValid(const QMimeData* mime) noexcept {
  return mime && (!mime->data(getMimeType()).isNull());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QPixmap SymbolClipboardData::generatePixmap() noexcept {
  // Maybe it's good that we don't pass the workspace here, to get
  // workspace-independent pixmaps?
  std::unique_ptr<GraphicsLayerList> layers =
      GraphicsLayerList::libraryLayers(nullptr);

  GraphicsScene scene;
  QVector<std::shared_ptr<QGraphicsItem>> items;
  for (auto ptr : mPins.values()) {
    items.append(std::make_shared<SymbolPinGraphicsItem>(ptr, *layers));
  }
  for (Polygon& polygon : mPolygons) {
    items.append(std::make_shared<PolygonGraphicsItem>(polygon, *layers));
  }
  for (Circle& circle : mCircles) {
    items.append(std::make_shared<CircleGraphicsItem>(circle, *layers));
  }
  for (Text& text : mTexts) {
    items.append(std::make_shared<TextGraphicsItem>(text, *layers));
  }
  foreach (const auto& item, items) {
    scene.addItem(*item);
  }
  return scene.toPixmap(300);
}

QString SymbolClipboardData::getMimeType() noexcept {
  return QString("application/x-librepcb-clipboard.symbol; version=%1")
      .arg(Application::getVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
