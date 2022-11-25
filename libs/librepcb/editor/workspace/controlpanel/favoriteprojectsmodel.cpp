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
#include "favoriteprojectsmodel.h"

#include <librepcb/core/fileio/fileutils.h>
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

FavoriteProjectsModel::FavoriteProjectsModel(
    const Workspace& workspace) noexcept
  : QAbstractListModel(nullptr), mWorkspace(workspace) {
  try {
    mFilePath = mWorkspace.getDataPath().getPathTo("favorite_projects.lp");
    if (mFilePath.isExistingFile()) {
      SExpression root =
          SExpression::parse(FileUtils::readFile(mFilePath), mFilePath);
      const QList<SExpression>& childs = root.getChildren("project");
      foreach (const SExpression& child, childs) {
        QString path = child.getChild("@0").getValue();
        FilePath absPath = FilePath::fromRelative(mWorkspace.getPath(), path);
        mAllProjects.append(absPath);
      }
      updateVisibleProjects();
    }
  } catch (Exception& e) {
    qWarning() << "Failed to read favorite projects file:" << e.getMsg();
  }
}

FavoriteProjectsModel::~FavoriteProjectsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool FavoriteProjectsModel::isFavoriteProject(const FilePath& filepath) const
    noexcept {
  return mAllProjects.contains(filepath);
}

void FavoriteProjectsModel::addFavoriteProject(
    const FilePath& filepath) noexcept {
  if (!mAllProjects.contains(filepath)) {
    mAllProjects.append(filepath);
    updateVisibleProjects();
    save();
  }
}

void FavoriteProjectsModel::removeFavoriteProject(
    const FilePath& filepath) noexcept {
  if (mAllProjects.removeAll(filepath) > 0) {
    updateVisibleProjects();
    save();
  }
}

void FavoriteProjectsModel::updateVisibleProjects() noexcept {
  beginResetModel();
  mVisibleProjects.clear();
  foreach (const FilePath& fp, mAllProjects) {
    if ((!mVisibleProjects.contains(fp)) && fp.isExistingFile()) {
      // show only existing projects
      mVisibleProjects.append(fp);
    }
  }
  endResetModel();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FavoriteProjectsModel::save() noexcept {
  try {
    // save the new list in the workspace
    SExpression root = SExpression::createList("librepcb_favorite_projects");
    foreach (const FilePath& filepath, mAllProjects) {
      root.ensureLineBreak();
      root.appendChild("project", filepath.toRelative(mWorkspace.getPath()));
    }
    root.ensureLineBreak();
    FileUtils::writeFile(mFilePath, root.toByteArray());  // can throw
  } catch (Exception& e) {
    qWarning() << "Failed to save favorite projects file:" << e.getMsg();
  }
}

int FavoriteProjectsModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;
  else
    return mVisibleProjects.count();
}

QVariant FavoriteProjectsModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  switch (role) {
    case Qt::DisplayRole:
      return mVisibleProjects.value(index.row()).getFilename();

    // case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::UserRole:
      return mVisibleProjects.value(index.row()).toNative();

    case Qt::DecorationRole:
      return QIcon(":/img/actions/bookmark.png");

    default:
      return QVariant();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
