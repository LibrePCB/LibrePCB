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

#ifndef LIBREPCB_WORKSPACE_RECENTPROJECTSMODEL_H
#define LIBREPCB_WORKSPACE_RECENTPROJECTSMODEL_H

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
namespace app {

/*******************************************************************************
 *  Class RecentProjectsModel
 ******************************************************************************/

/**
 * @brief The RecentProjectsModel class
 */
class RecentProjectsModel : public QObject,
                            public slint::Model<ui::FolderTreeItem> {
  Q_OBJECT

public:
  // Constructors / Destructor
  RecentProjectsModel() = delete;
  RecentProjectsModel(const RecentProjectsModel& other) = delete;
  explicit RecentProjectsModel(Workspace& ws,
                               QObject* parent = nullptr) noexcept;
  virtual ~RecentProjectsModel() noexcept;

  // General Methods
  void push(const FilePath& fp) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::FolderTreeItem> row_data(std::size_t i) const override;

  // Operator Overloadings
  RecentProjectsModel& operator=(const RecentProjectsModel& rhs) = delete;

private:
  void load() noexcept;
  void save() noexcept;
  void refreshItems() noexcept;

  const Workspace& mWorkspace;
  const FilePath mFilePath;
  QList<FilePath> mPaths;
  std::vector<ui::FolderTreeItem> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
