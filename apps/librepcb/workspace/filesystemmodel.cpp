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
#include "quickaccessmodel.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/utils/toolbox.h>
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
                                 const QString& settingsPrefix,
                                 QuickAccessModel* quickAccessModel,
                                 QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mRoot(root),
    mSettingsPrefix(settingsPrefix),
    mQuickAccess(quickAccessModel),
    mIconProvider(new FileIconProvider()) {
  if (mQuickAccess) {
    connect(mQuickAccess, &QuickAccessModel::favoriteProjectChanged, this,
            &FileSystemModel::favoriteProjectChanged);
  }
  connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this,
          &FileSystemModel::directoryChanged);

  // Restore expanded directories.
  QSettings cs;
  for (const QString& path :
       cs.value(mSettingsPrefix % "/expanded").toStringList()) {
    const FilePath fp = mRoot.getPathTo(path);
    if (fp.isValid()) {
      mExpandedDirs.insert(fp);
    }
  }

  // Clean up non-existing expanded directories if there are many.
  if (mExpandedDirs.count() > 100) {
    qInfo()
        << "A lot of expanded directories in file system model, cleaning up...";
    mExpandedDirs.removeIf(
        [](const FilePath& fp) { return !fp.isExistingDir(); });
  }

  // Load directory.
  expandDir(root, 0, 0);
}

FileSystemModel::~FileSystemModel() noexcept {
  // Save expanded directories.
  QSettings cs;
  QStringList paths;
  for (const FilePath& fp : mExpandedDirs) {
    paths.append(fp.toRelative(mRoot));
  }
  std::sort(paths.begin(), paths.end());
  cs.setValue(mSettingsPrefix % "/expanded", paths);
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t FileSystemModel::row_count() const {
  return mItems.size();
}

std::optional<ui::FolderTreeItemData> FileSystemModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void FileSystemModel::set_row_data(
    std::size_t i, const ui::FolderTreeItemData& data) noexcept {
  if (i < mItems.size()) {
    const FilePath fp(s2q(data.path));
    if ((!mItems.at(i).expanded) && data.expanded) {
      expandDir(fp, i + 1, data.level + 1);
    } else if (mItems.at(i).expanded && (!data.expanded)) {
      collapseDir(fp, i + 1, data.level + 1);
    }
    if (mQuickAccess && mItems.at(i).supports_pinning &&
        (mItems.at(i).pinned != data.pinned)) {
      mQuickAccess->setFavoriteProject(fp, data.pinned);
    }
    mItems.at(i) = data;
    row_changed(i);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FileSystemModel::expandDir(const FilePath& fp, std::size_t index,
                                int level) noexcept {
  QVector<std::size_t> childsToBeExpanded;

  QDir dir(fp.toStr());
  dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
  dir.setSorting(QDir::Name | QDir::DirsFirst);
  foreach (const QFileInfo& info, dir.entryInfoList()) {
    const FilePath itemFp(info.absoluteFilePath());
    const bool expand = info.isDir() && mExpandedDirs.contains(itemFp);
    if (expand) {
      childsToBeExpanded.append(index);
    }
    const bool isProject =
        (itemFp.getSuffix() == "lpp") || (itemFp.getSuffix() == "lppz");
    mItems.insert(
        mItems.begin() + index,
        ui::FolderTreeItemData{
            level, q2s(mIconProvider->icon(info).pixmap(48)),
            q2s(info.fileName()), q2s(itemFp.toStr()),
            info.isDir(),  // Has children
            expand,  // Expanded
            isProject,  // Supports pinning
            mQuickAccess && mQuickAccess->isFavoriteProject(itemFp),  // Pinned
        });
    row_added(index, 1);
    ++index;
  }
  if (!mWatcher.addPath(fp.toStr())) {
    qWarning() << "Failed to watch directory:" << fp.toNative();
  }
  if (fp != mRoot) {
    mExpandedDirs.insert(fp);
  }

  // Expand children from bottom to top to keep indices valid.
  std::reverse(childsToBeExpanded.begin(), childsToBeExpanded.end());
  for (std::size_t i : childsToBeExpanded) {
    expandDir(FilePath(s2q(mItems.at(i).path)), i + 1, mItems.at(i).level + 1);
  }
}

void FileSystemModel::collapseDir(const FilePath& fp, std::size_t index,
                                  int level) noexcept {
  for (const QString& dir : mWatcher.directories()) {
    const FilePath dirFp(dir);
    if ((dirFp == fp) || dirFp.isLocatedInDir(fp)) {
      if (!mWatcher.removePath(dir)) {
        qWarning() << "Failed to unwatch directory:" << dirFp.toNative();
      }
    }
  }

  int childCount = 0;
  for (std::size_t i = index;
       (i < mItems.size()) && (mItems.at(i).level >= level); ++i) {
    ++childCount;
  }
  if (childCount > 0) {
    mItems.erase(mItems.begin() + index, mItems.begin() + index + childCount);
    row_removed(index, childCount);
  }
  if (fp != mRoot) {
    mExpandedDirs.remove(fp);
  }
}

void FileSystemModel::directoryChanged(const QString& dir) noexcept {
  const FilePath fp(dir);
  qDebug() << "Watched directory changed:" << fp.toNative();

  for (std::size_t i = 0; i < mItems.size(); ++i) {
    if (FilePath(s2q(mItems.at(i).path)) == fp) {
      if (mItems.at(i).expanded) {
        collapseDir(fp, i + 1, mItems.at(i).level + 1);
        expandDir(fp, i + 1, mItems.at(i).level + 1);
      }
      break;
    }
  }
}

void FileSystemModel::favoriteProjectChanged(const FilePath& fp,
                                             bool favorite) noexcept {
  for (std::size_t i = 0; i < mItems.size(); ++i) {
    if (mItems.at(i).supports_pinning &&
        (FilePath(s2q(mItems.at(i).path)) == fp) &&
        (mItems.at(i).pinned != favorite)) {
      mItems.at(i).pinned = favorite;
      row_changed(i);
      break;
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
