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
#include "boardclipboarddata.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/circuit/netsignal.h>

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

BoardClipboardData::BoardClipboardData(const Uuid& boardUuid,
                                       const Point& cursorPos) noexcept
  : mFileSystem(TransactionalFileSystem::openRW(FilePath::getRandomTempPath())),
    mBoardUuid(boardUuid),
    mCursorPos(cursorPos),
    mDevices(),
    mNetSegments(),
    mPlanes(),
    mPolygons(),
    mStrokeTexts(),
    mHoles(),
    mPadPositions() {
}

BoardClipboardData::BoardClipboardData(const QByteArray& mimeData)
  : BoardClipboardData(Uuid::createRandom(), Point()) {
  mFileSystem->loadFromZip(mimeData);  // can throw

  SExpression root =
      SExpression::parse(mFileSystem->read("board.lp"), FilePath());
  mBoardUuid = deserialize<Uuid>(root.getChild("board/@0"));
  mCursorPos = Point(root.getChild("cursor_position"));
  mDevices.loadFromSExpression(root);
  mNetSegments.loadFromSExpression(root);
  mPlanes.loadFromSExpression(root);
  mPolygons.loadFromSExpression(root);

  foreach (const SExpression* child, root.getChildren("stroke_text")) {
    mStrokeTexts.append(BoardStrokeTextData(*child));
  }

  foreach (const SExpression* child, root.getChildren("hole")) {
    mHoles.append(BoardHoleData(*child));
  }

  foreach (const SExpression* child, root.getChildren("pad_position")) {
    mPadPositions.insert(
        std::make_pair(deserialize<Uuid>(child->getChild("device/@0")),
                       deserialize<Uuid>(child->getChild("pad/@0"))),
        Point(child->getChild("position")));
  }
}

BoardClipboardData::~BoardClipboardData() noexcept {
  // Clean up the temporary directory, but destroy the TransactionalFileSystem
  // object first since it has a lock on the directory.
  FilePath fp = mFileSystem->getAbsPath();
  mFileSystem.reset();
  QDir(fp.toStr()).removeRecursively();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BoardClipboardData::isEmpty() const noexcept {
  return mDevices.isEmpty() && mNetSegments.isEmpty() && mPlanes.isEmpty() &&
      mPolygons.isEmpty() && mStrokeTexts.isEmpty() && mHoles.isEmpty();
}

std::unique_ptr<TransactionalDirectory> BoardClipboardData::getDirectory(
    const QString& path) noexcept {
  return std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem, path));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<QMimeData> BoardClipboardData::toMimeData() const {
  SExpression root = SExpression::createList("librepcb_clipboard_board");
  root.ensureLineBreak();
  mCursorPos.serialize(root.appendList("cursor_position"));
  root.ensureLineBreak();
  root.appendChild("board", mBoardUuid);
  root.ensureLineBreak();
  mDevices.serialize(root);
  root.ensureLineBreak();
  mNetSegments.serialize(root);
  root.ensureLineBreak();
  mPlanes.serialize(root);
  root.ensureLineBreak();
  mPolygons.serialize(root);
  for (const BoardStrokeTextData& data : mStrokeTexts) {
    root.ensureLineBreak();
    data.serialize(root.appendList("stroke_text"));
  }
  root.ensureLineBreak();
  for (const BoardHoleData& data : mHoles) {
    root.ensureLineBreak();
    data.serialize(root.appendList("hole"));
  }
  root.ensureLineBreak();
  for (auto it = mPadPositions.begin(); it != mPadPositions.end(); ++it) {
    SExpression child = SExpression::createList("pad_position");
    child.appendChild("device", it.key().first);
    child.appendChild("pad", it.key().second);
    it.value().serialize(child.appendList("position"));
    root.ensureLineBreak();
    root.appendChild(child);
  }
  root.ensureLineBreak();
  mFileSystem->write("board.lp", root.toByteArray());

  QByteArray zip = mFileSystem->exportToZip();

  std::unique_ptr<QMimeData> data(new QMimeData());
  data->setData(getMimeType(), zip);
  data->setData("application/zip", zip);
  data->setText(root.toByteArray());  // TODO: Remove this
  return data;
}

std::unique_ptr<BoardClipboardData> BoardClipboardData::fromMimeData(
    const QMimeData* mime) {
  QByteArray content = mime ? mime->data(getMimeType()) : QByteArray();
  if (!content.isNull()) {
    return std::unique_ptr<BoardClipboardData>(
        new BoardClipboardData(content));  // can throw
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString BoardClipboardData::getMimeType() noexcept {
  return QString("application/x-librepcb-clipboard.board; version=%1")
      .arg(Application::getVersion());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
