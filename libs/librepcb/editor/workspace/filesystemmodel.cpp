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

#include "../utils/slinthelpers.h"
#include "quickaccessmodel.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/workspace/workspace.h>

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

FileSystemModel::FileSystemModel(const Workspace& ws, const FilePath& root,
                                 const QString& settingsPrefix,
                                 QuickAccessModel* quickAccessModel,
                                 QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mRoot(root),
    mSettingsPrefix(settingsPrefix),
    mQuickAccess(quickAccessModel) {
  if (mQuickAccess) {
    connect(this, &FileSystemModel::pinningRequested, mQuickAccess,
            &QuickAccessModel::setFavoriteProject, Qt::QueuedConnection);
    connect(mQuickAccess, &QuickAccessModel::favoriteProjectChanged, this,
            &FileSystemModel::favoriteProjectChanged, Qt::QueuedConnection);
  }
  connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this,
          &FileSystemModel::directoryChanged);

  // Run actions asynchronously to avoid complex nested function calls.
  connect(this, &FileSystemModel::actionTriggered, this,
          &FileSystemModel::handleAction, Qt::QueuedConnection);

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

std::optional<ui::TreeViewItemData> FileSystemModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void FileSystemModel::set_row_data(std::size_t i,
                                   const ui::TreeViewItemData& data) noexcept {
  const FilePath fp(s2q(data.user_data));
  if ((i < mItems.size()) && fp.isValid()) {
    if ((!mItems.at(i).expanded) && data.expanded) {
      expandDir(fp, i + 1, data.level + 1);
    } else if (mItems.at(i).expanded && (!data.expanded)) {
      collapseDir(fp, i + 1, data.level + 1);
    }
    if (mItems.at(i).supports_pinning && (mItems.at(i).pinned != data.pinned)) {
      emit pinningRequested(fp, data.pinned);
    }
    if (data.action != ui::Action::None) {
      emit actionTriggered(fp, data.action);
    }
    mItems.at(i) = data;
    mItems.at(i).action = ui::Action::None;
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
    const bool isProjectFile =
        (itemFp.getSuffix() == "lpp") || (itemFp.getSuffix() == "lppz");
    const bool isPinnable = isProjectFile && mQuickAccess;
    const bool isProjectFolder = Project::isProjectDirectory(itemFp);
    const bool isProjectFileOrFolder =
        isProjectFolder || Project::isFilePathInsideProjectDirectory(itemFp);

    QIcon icon;
    if (isProjectFile) {
      // TODO: For *.lppz, use zipped project icon.
      icon = QIcon(":/img/app/librepcb.png");
    } else if (isProjectFolder) {
      icon = QIcon(":/img/places/project_folder.png");
    } else if (itemFp.isExistingDir()) {
      icon = QIcon(":/img/places/folder.png");
    } else if (itemFp.isExistingFile()) {
      icon = QIcon(":/img/places/file.png");
    }

    mItems.insert(
        mItems.begin() + index,
        ui::TreeViewItemData{
            level,  // Level
            q2s(icon.pixmap(32)),  // Icon
            q2s(info.fileName()),  // Text
            slint::SharedString(),  // Hint
            q2s(itemFp.toStr()),  // User data
            isProjectFileOrFolder,
            // Is project file or folder
            info.isDir(),  // Has children
            expand,  // Expanded
            isPinnable,  // Supports pinning
            isPinnable && mQuickAccess->isFavoriteProject(itemFp),  // Pinned
            ui::Action::None,  // Action
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
    expandDir(FilePath(s2q(mItems.at(i).user_data)), i + 1,
              mItems.at(i).level + 1);
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

  if (fp == mRoot) {
    collapseDir(mRoot, 0, 0);
    expandDir(mRoot, 0, 0);
  } else {
    for (std::size_t i = 0; i < mItems.size(); ++i) {
      if (FilePath(s2q(mItems.at(i).user_data)) == fp) {
        if (mItems.at(i).expanded) {
          collapseDir(fp, i + 1, mItems.at(i).level + 1);
          expandDir(fp, i + 1, mItems.at(i).level + 1);
        }
        break;
      }
    }
  }
}

void FileSystemModel::favoriteProjectChanged(const FilePath& fp,
                                             bool favorite) noexcept {
  for (std::size_t i = 0; i < mItems.size(); ++i) {
    if (mItems.at(i).supports_pinning &&
        (FilePath(s2q(mItems.at(i).user_data)) == fp) &&
        (mItems.at(i).pinned != favorite)) {
      mItems.at(i).pinned = favorite;
      row_changed(i);
      break;
    }
  }
}

void FileSystemModel::handleAction(const FilePath& fp, ui::Action a) noexcept {
  if (a == ui::Action::Default) {
    emit openFileTriggered(fp);
  } else if (a == ui::Action::FolderNew) {
    QString n = QInputDialog::getText(qApp->activeWindow(), tr("New Folder"),
                                      tr("Name:"));
    n = FilePath::cleanFileName(n, FilePath::CleanFileNameOption::Default);
    if (!n.isEmpty()) {
      QDir(fp.toStr()).mkdir(n);
    }
  } else if (a == ui::Action::ProjectNew) {
    emit newProjectTriggered(fp);
  } else if (a == ui::Action::Delete) {
    removeFileOrDirectory(fp);
  } else {
    qWarning() << "Unhandled action in FileSystemModel:" << static_cast<int>(a);
  }
}

void FileSystemModel::removeFileOrDirectory(const FilePath& fp) noexcept {
  if ((!fp.isValid()) || (!fp.isLocatedInDir(mRoot))) {
    return;
  }

  const QMessageBox::StandardButton btn = QMessageBox::question(
      qApp->activeWindow(), tr("Remove"),
      tr("Are you really sure to remove following file or directory?\n\n"
         "%1\n\nWarning: This cannot be undone!")
          .arg(fp.toNative()));
  if (btn != QMessageBox::Yes) {
    return;
  }

  try {
    if (fp.isExistingDir()) {
      FileUtils::removeDirRecursively(fp);
    } else {
      FileUtils::removeFile(fp);
    }
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
