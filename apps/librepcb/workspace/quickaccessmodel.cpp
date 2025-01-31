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
#include "quickaccessmodel.h"

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

QuickAccessModel::QuickAccessModel(Workspace& ws, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mRecentProjectsFp(ws.getDataPath().getPathTo("recent_projects.lp")),
    mFavoriteProjectsFp(ws.getDataPath().getPathTo("favorite_projects.lp")) {
  load();
  refreshItems();
}

QuickAccessModel::~QuickAccessModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void QuickAccessModel::pushRecentProject(const FilePath& fp) noexcept {
  if ((mRecentProjects.count() > 0) && (mRecentProjects.first() == fp)) {
    // The filename is already on top of the list, so nothing to do here...
    return;
  }

  // First remove it from the list, then add it to the top of the list.
  mRecentProjects.removeAll(fp);
  mRecentProjects.prepend(fp);
  refreshItems();
  saveRecentProjects();
}

void QuickAccessModel::setFavoriteProject(const FilePath& fp,
                                          bool favorite) noexcept {
  if ((favorite) && (!mFavoriteProjects.contains(fp))) {
    mFavoriteProjects.append(fp);
    refreshItems();
    saveFavoriteProjects();
    emit favoriteProjectChanged(fp, favorite);
  } else if ((!favorite) && (mFavoriteProjects.removeAll(fp) > 0)) {
    refreshItems();
    saveFavoriteProjects();
    emit favoriteProjectChanged(fp, favorite);
  }
}

bool QuickAccessModel::isFavoriteProject(const FilePath& fp) const noexcept {
  return mFavoriteProjects.contains(fp);
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t QuickAccessModel::row_count() const {
  return mItems.size();
}

std::optional<ui::QuickAccessItemData> QuickAccessModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void QuickAccessModel::set_row_data(
    std::size_t i, const ui::QuickAccessItemData& data) noexcept {
  Q_UNUSED(i);
  const FilePath fp(s2q(data.path));
  if (fp.isValid()) {
    setFavoriteProject(fp, data.pinned);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void QuickAccessModel::load() noexcept {
  try {
    if (mRecentProjectsFp.isExistingFile()) {
      const std::unique_ptr<const SExpression> root = SExpression::parse(
          FileUtils::readFile(mRecentProjectsFp), mRecentProjectsFp);
      foreach (const SExpression* child, root->getChildren("project")) {
        const QString relPath = child->getChild("@0").getValue();
        mRecentProjects.append(
            FilePath::fromRelative(mWorkspace.getPath(), relPath));
      }
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to read recent projects file:" << e.getMsg();
  }

  try {
    if (mFavoriteProjectsFp.isExistingFile()) {
      const std::unique_ptr<const SExpression> root = SExpression::parse(
          FileUtils::readFile(mFavoriteProjectsFp), mFavoriteProjectsFp);
      foreach (const SExpression* child, root->getChildren("project")) {
        const QString relPath = child->getChild("@0").getValue();
        mFavoriteProjects.append(
            FilePath::fromRelative(mWorkspace.getPath(), relPath));
      }
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to read favorite projects file:" << e.getMsg();
  }
}

void QuickAccessModel::saveRecentProjects() noexcept {
  try {
    std::unique_ptr<SExpression> root =
        SExpression::createList("librepcb_recent_projects");
    foreach (const FilePath& filepath, mRecentProjects) {
      root->ensureLineBreak();
      root->appendChild("project", filepath.toRelative(mWorkspace.getPath()));
    }
    root->ensureLineBreak();
    FileUtils::writeFile(mRecentProjectsFp, root->toByteArray());  // can throw
  } catch (const Exception& e) {
    qWarning() << "Failed to save recent projects file:" << e.getMsg();
  }
}

void QuickAccessModel::saveFavoriteProjects() noexcept {
  try {
    std::unique_ptr<SExpression> root =
        SExpression::createList("librepcb_favorite_projects");
    foreach (const FilePath& filepath, mFavoriteProjects) {
      root->ensureLineBreak();
      root->appendChild("project", filepath.toRelative(mWorkspace.getPath()));
    }
    root->ensureLineBreak();
    FileUtils::writeFile(mFavoriteProjectsFp,
                         root->toByteArray());  // can throw
  } catch (const Exception& e) {
    qWarning() << "Failed to save favorite projects file:" << e.getMsg();
  }
}

void QuickAccessModel::refreshItems() noexcept {
  mItems.clear();
  QSet<FilePath> set;
  for (const FilePath& fp : mRecentProjects + mFavoriteProjects) {
    const bool favorite = mFavoriteProjects.contains(fp);
    if (((set.count() < 5) || (favorite)) && (!set.contains(fp)) &&
        fp.isExistingFile()) {
      mItems.push_back(ui::QuickAccessItemData{
          q2s(fp.getFilename()),
          q2s(fp.toStr()),
          favorite,
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
