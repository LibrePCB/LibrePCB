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

#include <librepcb/core/application.h>
#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/graphics/holegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/stroketextgraphicsitem.h>
#include <librepcb/core/library/pkg/footprintpadgraphicsitem.h>

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

FootprintClipboardData::FootprintClipboardData(const SExpression& node,
                                               const Version& fileFormat)
  : mFootprintUuid(
        deserialize<Uuid>(node.getChild("footprint/@0"), fileFormat)),
    mPackagePads(node.getChild("package"), fileFormat),
    mCursorPos(node.getChild("cursor_position"), fileFormat),
    mFootprintPads(node, fileFormat),
    mPolygons(node, fileFormat),
    mCircles(node, fileFormat),
    mStrokeTexts(node, fileFormat),
    mHoles(node, fileFormat) {
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
    return std::unique_ptr<FootprintClipboardData>(new FootprintClipboardData(
        root, qApp->getFileFormatVersion()));  // can throw
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FootprintClipboardData::serialize(SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild(mCursorPos.serializeToDomElement("cursor_position"));
  root.ensureLineBreak();
  root.appendChild("footprint", mFootprintUuid);
  root.ensureLineBreak();
  SExpression& packageRoot = root.appendList("package");
  mPackagePads.serialize(packageRoot);
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
}

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
}  // namespace librepcb
