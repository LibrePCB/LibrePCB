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

#include "../utils/slinthelpers.h"

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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

QuickAccessModel::QuickAccessModel(Workspace& ws, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mRecentProjectsFp(ws.getDataPath().getPathTo("recent_projects.lp")),
    mFavoriteProjectsFp(ws.getDataPath().getPathTo("favorite_projects.lp")),
    mIcon(q2s(QIcon(":/img/app/librepcb.svg").pixmap(32))) {
  load();
  refreshItems();

  // Run actions asynchronously to avoid complex nested function calls.
  connect(this, &QuickAccessModel::actionTriggered, this,
          &QuickAccessModel::handleAction, Qt::QueuedConnection);

  // Refresh items when any directory of favorite/recent projects has been
  // modified. However, delay the update a bit to avoid unnecessary operations
  // when there are many file system changes within a short time period.
  mWatcherTimer.setSingleShot(true);
  connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this,
          [this]() { mWatcherTimer.start(500); });
  connect(
      &mWatcherTimer, &QTimer::timeout, this,
      [this]() {
        qDebug() << "Quick access directories modified, updating items...";
        refreshItems();
      },
      Qt::QueuedConnection);
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

void QuickAccessModel::discardRecentProject(const FilePath& fp) noexcept {
  if (mRecentProjects.removeAll(fp) > 0) {
    refreshItems();
    saveRecentProjects();
  }
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

std::optional<ui::TreeViewItemData> QuickAccessModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void QuickAccessModel::set_row_data(std::size_t i,
                                    const ui::TreeViewItemData& data) noexcept {
  Q_UNUSED(i);
  const FilePath fp(s2q(data.user_data));
  if (fp.isValid()) {
    setFavoriteProject(fp, data.pinned);
    if (data.action != ui::Action::None) {
      emit actionTriggered(fp, data.action);
    }
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
  QSet<FilePath> listedPaths;
  std::vector<ui::TreeViewItemData> items;
  for (const FilePath& fp : mRecentProjects + mFavoriteProjects) {
    const bool favorite = mFavoriteProjects.contains(fp);
    if (((listedPaths.count() < 5) || (favorite)) &&
        (!listedPaths.contains(fp)) && fp.isExistingFile()) {
      items.push_back(ui::TreeViewItemData{
          0,  // Level
          mIcon,  // Icon
          q2s(fp.getFilename()),  // Text
          q2s(fp.toNative()),  // Hint
          q2s(fp.toStr()),  // User data
          true,  // Is project file or folder
          false,  // Has children
          false,  // Expanded
          true,  // Supports pinning
          favorite,  // Pinned
          ui::Action::None,  // Action
      });
      listedPaths.insert(fp);
    }
  }
  setWatchedProjects(listedPaths);

  const std::size_t oldCount = mItems.size();
  if (items.size() < oldCount) {
    notify_row_removed(items.size(), oldCount - items.size());
  }
  mItems = items;
  if (items.size() > oldCount) {
    notify_row_added(oldCount, items.size() - oldCount);
  }
  for (std::size_t i = 0; i < std::min(items.size(), oldCount); ++i) {
    notify_row_changed(i);
  }
}

static void addToWatchedDirs(const FilePath& fp, QSet<FilePath>& out,
                             int limit = 10) noexcept {
  if (fp.isValid()) {
    out.insert(fp);
    if (limit > 0) {
      addToWatchedDirs(fp.getParentDir(), out, limit - 1);
    }
  }
}

void QuickAccessModel::setWatchedProjects(
    const QSet<FilePath>& projects) noexcept {
  QSet<FilePath> toBeWatched;
  for (const FilePath& fp : projects) {
    addToWatchedDirs(fp.getParentDir(), toBeWatched);
  }

  QSet<FilePath> watched;
  for (const QString& dir : mWatcher.directories()) {
    watched.insert(FilePath(dir));
  }

  for (const FilePath& fp : watched - toBeWatched) {
    mWatcher.removePath(fp.toStr());
  }
  for (const FilePath& fp : toBeWatched - watched) {
    if (!mWatcher.addPath(fp.toStr())) {
      qWarning() << "Failed to watch file:" << fp.toNative();
    }
  }
}

void QuickAccessModel::handleAction(const FilePath& fp, ui::Action a) noexcept {
  if (a == ui::Action::Default) {
    emit openFileTriggered(fp);
  } else if (a == ui::Action::Delete) {
    setFavoriteProject(fp, false);
    discardRecentProject(fp);
  } else {
    qWarning() << "Unhandled action in QuickAccessModel:"
               << static_cast<int>(a);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
