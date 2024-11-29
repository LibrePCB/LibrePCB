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

#include "../apptoolbox.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/workspace/workspace.h>

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

FavoriteProjectsModel::FavoriteProjectsModel(Workspace& ws,
                                             QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mFilePath(ws.getDataPath().getPathTo("favorite_projects.lp")) {
  load();
  refreshItems();
}

FavoriteProjectsModel::~FavoriteProjectsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void FavoriteProjectsModel::add(const FilePath& fp) noexcept {
  if (!mPaths.contains(fp)) {
    mPaths.append(fp);
    refreshItems();
    save();
  }
}

void FavoriteProjectsModel::remove(const FilePath& fp) noexcept {
  if (mPaths.removeAll(fp) > 0) {
    refreshItems();
    save();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t FavoriteProjectsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::FolderTreeItem> FavoriteProjectsModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void FavoriteProjectsModel::load() noexcept {
  try {
    if (mFilePath.isExistingFile()) {
      const std::unique_ptr<const SExpression> root =
          SExpression::parse(FileUtils::readFile(mFilePath), mFilePath);
      foreach (const SExpression* child, root->getChildren("project")) {
        const QString relPath = child->getChild("@0").getValue();
        mPaths.append(FilePath::fromRelative(mWorkspace.getPath(), relPath));
      }
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to read favorite projects file:" << e.getMsg();
  }
}

void FavoriteProjectsModel::save() noexcept {
  try {
    std::unique_ptr<SExpression> root =
        SExpression::createList("librepcb_favorite_projects");
    foreach (const FilePath& filepath, mPaths) {
      root->ensureLineBreak();
      root->appendChild("project", filepath.toRelative(mWorkspace.getPath()));
    }
    root->ensureLineBreak();
    FileUtils::writeFile(mFilePath, root->toByteArray());  // can throw
  } catch (const Exception& e) {
    qWarning() << "Failed to save favorite projects file:" << e.getMsg();
  }
}

void FavoriteProjectsModel::refreshItems() noexcept {
  mItems.clear();
  QSet<FilePath> set;
  for (const FilePath& fp : mPaths) {
    if ((!set.contains(fp)) && fp.isExistingFile()) {
      mItems.push_back(ui::FolderTreeItem{
          0,
          q2s(QPixmap(":/img/logo/48x48.png")),
          q2s(fp.getFilename()),
          q2s(fp.toStr()),
          false,
      });
      set.insert(fp);
    }
  }
  reset();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
