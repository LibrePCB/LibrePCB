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

#ifndef LIBREPCB_EDITOR_QUICKACCESSMODEL_H
#define LIBREPCB_EDITOR_QUICKACCESSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

/*******************************************************************************
 *  Class QuickAccessModel
 ******************************************************************************/

/**
 * @brief The QuickAccessModel class
 */
class QuickAccessModel : public QObject,
                         public slint::Model<ui::TreeViewItemData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  QuickAccessModel() = delete;
  QuickAccessModel(const QuickAccessModel& other) = delete;
  explicit QuickAccessModel(Workspace& ws, QObject* parent = nullptr) noexcept;
  virtual ~QuickAccessModel() noexcept;

  // General Methods
  void pushRecentProject(const FilePath& fp) noexcept;
  void discardRecentProject(const FilePath& fp) noexcept;
  void setFavoriteProject(const FilePath& fp, bool favorite) noexcept;
  bool isFavoriteProject(const FilePath& fp) const noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::TreeViewItemData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::TreeViewItemData& data) noexcept override;

  // Operator Overloadings
  QuickAccessModel& operator=(const QuickAccessModel& rhs) = delete;

signals:
  void actionTriggered(const FilePath& fp, ui::Action a);  // Internal signal.
  void favoriteProjectChanged(const FilePath& fp, bool favorite);
  void openFileTriggered(const FilePath& fp);

private:
  void load() noexcept;
  void saveRecentProjects() noexcept;
  void saveFavoriteProjects() noexcept;
  void refreshItems() noexcept;
  void setWatchedProjects(const QSet<FilePath>& projects) noexcept;
  void handleAction(const FilePath& fp, ui::Action a) noexcept;

  const Workspace& mWorkspace;
  const FilePath mRecentProjectsFp;
  const FilePath mFavoriteProjectsFp;
  const slint::Image mIcon;
  QList<FilePath> mRecentProjects;
  QList<FilePath> mFavoriteProjects;
  std::vector<ui::TreeViewItemData> mItems;
  QFileSystemWatcher mWatcher;
  QTimer mWatcherTimer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
