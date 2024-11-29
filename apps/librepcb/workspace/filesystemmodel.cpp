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
#include "filesystemmodel.h"

#include "../apptoolbox.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/editor/workspace/controlpanel/fileiconprovider.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

FileSystemModel::FileSystemModel(const Workspace& ws, const FilePath& root,
                                 QObject* parent) noexcept
  : QObject(parent), mWorkspace(ws), mRoot(root) {
  refresh();
}

FileSystemModel::~FileSystemModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t FileSystemModel::row_count() const {
  return mItems.size();
}

std::optional<ui::FolderTreeItem> FileSystemModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileSystemModel::refresh() noexcept {
  mItems.clear();
  FileIconProvider ip;
  scanDir(mRoot.toStr(), 0, ip);
  reset();
}

void FileSystemModel::scanDir(const QString& fp, int level,
                              const FileIconProvider& ip) noexcept {
  QDir dir(fp);
  dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
  dir.setSorting(QDir::Name | QDir::DirsFirst);
  foreach (const QFileInfo& info, dir.entryInfoList()) {
    mItems.push_back(ui::FolderTreeItem{
        level,
        q2s(ip.icon(info).pixmap(48)),
        q2s(info.fileName()),
        q2s(info.filePath()),
        info.isDir(),
    });
    if (info.isDir() && (level < 1)) {
      scanDir(info.filePath(), level + 1, ip);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
