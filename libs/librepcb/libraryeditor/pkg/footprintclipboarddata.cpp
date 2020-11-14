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
#include "footprintclipboarddata.h"

#include <librepcb/common/application.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/stroketextgraphicsitem.h>
#include <librepcb/library/pkg/footprintpadgraphicsitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FootprintClipboardData::FootprintClipboardData(
    const Uuid& footprintUuid, const PackagePadList& packagePads,
    const Point& cursorPos) noexcept
  : mFootprintUuid(footprintUuid),
    mPackagePads(packagePads),
    mCursorPos(cursorPos) {
}

FootprintClipboardData::FootprintClipboardData(const SExpression& node)
  : mFootprintUuid(deserialize<Uuid>(node.getChild("footprint/@0"))),
    mPackagePads(node.getChild("package")),
    mCursorPos(node.getChild("cursor_position")),
    mFootprintPads(node),
    mPolygons(node),
    mCircles(node),
    mStrokeTexts(node),
    mHoles(node) {
}

FootprintClipboardData::~FootprintClipboardData() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<QMimeData> FootprintClipboardData::toMimeData(
    const IF_GraphicsLayerProvider& lp) {
  SExpression sexpr =
      serializeToDomElement("librepcb_clipboard_footprint");  // can throw

  std::unique_ptr<QMimeData> data(new QMimeData());
  data->setImageData(generatePixmap(lp));
  data->setData(getMimeType(), sexpr.toByteArray());
  return data;
}

std::unique_ptr<FootprintClipboardData> FootprintClipboardData::fromMimeData(
    const QMimeData* mime) {
  QByteArray content = mime ? mime->data(getMimeType()) : QByteArray();
  if (!content.isNull()) {
    SExpression root = SExpression::parse(content, FilePath());
    return std::unique_ptr<FootprintClipboardData>(
        new FootprintClipboardData(root));  // can throw
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintClipboardData::serialize(SExpression& root) const {
  root.appendChild(mCursorPos.serializeToDomElement("cursor_position"), true);
  root.appendChild("footprint", mFootprintUuid, true);
  SExpression& packageRoot = root.appendList("package", true);
  mPackagePads.serialize(packageRoot);
  mFootprintPads.serialize(root);
  mPolygons.serialize(root);
  mCircles.serialize(root);
  mStrokeTexts.serialize(root);
  mHoles.serialize(root);
}

QPixmap FootprintClipboardData::generatePixmap(
    const IF_GraphicsLayerProvider& lp) noexcept {
  GraphicsScene scene;
  QVector<std::shared_ptr<QGraphicsItem>> items;
  for (FootprintPad& pad : mFootprintPads) {
    items.append(
        std::make_shared<FootprintPadGraphicsItem>(pad, lp, &mPackagePads));
  }
  for (Polygon& polygon : mPolygons) {
    items.append(std::make_shared<PolygonGraphicsItem>(polygon, lp));
  }
  for (Circle& circle : mCircles) {
    items.append(std::make_shared<CircleGraphicsItem>(circle, lp));
  }
  for (StrokeText& text : mStrokeTexts) {
    text.setFont(&qApp->getDefaultStrokeFont());
    items.append(std::make_shared<StrokeTextGraphicsItem>(text, lp));
  }
  for (Hole& hole : mHoles) {
    items.append(std::make_shared<HoleGraphicsItem>(hole, lp));
  }
  foreach (const auto& item, items) { scene.addItem(*item); }
  return scene.toPixmap(300, Qt::black);
}

QString FootprintClipboardData::getMimeType() noexcept {
  return QString("application/x-librepcb-clipboard.footprint; version=%1")
      .arg(qApp->applicationVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
