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
#include "recentprojectsmodel.h"

#include "settings/workspacesettings.h"
#include "workspace.h"

#include <librepcb/common/fileio/fileutils.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RecentProjectsModel::RecentProjectsModel(const Workspace& workspace) noexcept
  : QAbstractListModel(nullptr), mWorkspace(workspace) {
  try {
    mFilePath = mWorkspace.getMetadataPath().getPathTo("recent_projects.lp");
    if (mFilePath.isExistingFile()) {
      SExpression root =
          SExpression::parse(FileUtils::readFile(mFilePath), mFilePath);
      const QList<SExpression>& childs = root.getChildren("project");
      foreach (const SExpression& child, childs) {
        QString  path    = child.getValueOfFirstChild<QString>(true);
        FilePath absPath = FilePath::fromRelative(mWorkspace.getPath(), path);
        mAllProjects.append(absPath);
      }
      updateVisibleProjects();
    }
  } catch (Exception& e) {
    qWarning() << "Could not read recent projects file:" << e.getMsg();
  }
}

RecentProjectsModel::~RecentProjectsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RecentProjectsModel::setLastRecentProject(
    const FilePath& filepath) noexcept {
  if ((mAllProjects.count() > 0) && (mAllProjects.first() == filepath)) {
    // the filename is already on top of the list, so nothing to do here...
    return;
  }

  // first remove it from the list, then add it to the top of the list
  mAllProjects.removeAll(filepath);
  mAllProjects.prepend(filepath);
  updateVisibleProjects();
  save();
}

void RecentProjectsModel::updateVisibleProjects() noexcept {
  beginResetModel();
  mVisibleProjects.clear();
  foreach (const FilePath& fp, mAllProjects) {
    if ((!mVisibleProjects.contains(fp)) && (mVisibleProjects.count() < 5) &&
        fp.isExistingFile()) {
      // show maximum 5 existing projects
      mVisibleProjects.append(fp);
    }
  }
  endResetModel();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RecentProjectsModel::save() noexcept {
  try {
    // save the new list in the workspace
    SExpression root = SExpression::createList("librepcb_recent_projects");
    foreach (const FilePath& filepath, mAllProjects) {
      root.appendChild("project", filepath.toRelative(mWorkspace.getPath()),
                       true);
    }
    FileUtils::writeFile(mFilePath, root.toByteArray());  // can throw
  } catch (Exception& e) {
    qWarning() << "Could not save recent projects file:" << e.getMsg();
  }
}

int RecentProjectsModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;
  else
    return mVisibleProjects.count();
}

QVariant RecentProjectsModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  switch (role) {
    case Qt::DisplayRole: {
      return mVisibleProjects.value(index.row()).getFilename();
    }

    case Qt::StatusTipRole:
    case Qt::UserRole:
      return mVisibleProjects.value(index.row()).toNative();

    case Qt::DecorationRole:
      return QIcon(":/img/actions/recent.png");

    default:
      return QVariant();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
