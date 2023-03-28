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

#include "../../graphics/circlegraphicsitem.h"
#include "../../graphics/graphicsscene.h"
#include "../../graphics/holegraphicsitem.h"
#include "../../graphics/polygongraphicsitem.h"
#include "../../graphics/stroketextgraphicsitem.h"
#include "footprintpadgraphicsitem.h"

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
  SExpression root = SExpression::createList("librepcb_clipboard_footprint");
  root.ensureLineBreak();
  mCursorPos.serialize(root.appendList("cursor_position"));
  root.ensureLineBreak();
  root.appendChild("footprint", mFootprintUuid);
  root.ensureLineBreak();
  {
    SExpression& node = root.appendList("package");
    mPackagePads.serialize(node);
  }
  root.ensureLineBreak();
  mFootprintPads.serialize(root);
  root.ensureLineBreak();
  mPolygons.serialize(root);
  root.ensureLineBreak();
  mCircles.serialize(root);
  root.ensureLineBreak();
  mStrokeTexts.serialize(root);
  root.ensureLineBreak();
  mHoles.serialize(root);
  root.ensureLineBreak();

  std::unique_ptr<QMimeData> data(new QMimeData());
  data->setImageData(generatePixmap(lp));
  data->setData(getMimeType(), root.toByteArray());
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

QPixmap FootprintClipboardData::generatePixmap(
    const IF_GraphicsLayerProvider& lp) noexcept {
  GraphicsScene scene;
  QVector<std::shared_ptr<QGraphicsItem>> items;
  for (auto pad : mFootprintPads.values()) {
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
    items.append(std::make_shared<StrokeTextGraphicsItem>(
        text, lp, Application::getDefaultStrokeFont()));
  }
  for (Hole& hole : mHoles) {
    items.append(std::make_shared<HoleGraphicsItem>(hole, lp, false));
  }
  foreach (const auto& item, items) { scene.addItem(*item); }
  return scene.toPixmap(300, Qt::black);
}

QString FootprintClipboardData::getMimeType() noexcept {
  return QString("application/x-librepcb-clipboard.footprint; version=%1")
      .arg(Application::getVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
