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

#ifndef LIBREPCB_WORKSPACE_QUICKACCESSMODEL_H
#define LIBREPCB_WORKSPACE_QUICKACCESSMODEL_H

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
 *  Class QuickAccessModel
 ******************************************************************************/

/**
 * @brief The QuickAccessModel class
 */
class QuickAccessModel : public QObject,
                         public slint::Model<ui::QuickAccessItemData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  QuickAccessModel() = delete;
  QuickAccessModel(const QuickAccessModel& other) = delete;
  explicit QuickAccessModel(Workspace& ws, QObject* parent = nullptr) noexcept;
  virtual ~QuickAccessModel() noexcept;

  // General Methods
  void pushRecentProject(const FilePath& fp) noexcept;
  void setFavoriteProject(const FilePath& fp, bool favorite) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::QuickAccessItemData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::QuickAccessItemData& data) noexcept override;

  // Operator Overloadings
  QuickAccessModel& operator=(const QuickAccessModel& rhs) = delete;

private:
  void load() noexcept;
  void saveRecentProjects() noexcept;
  void saveFavoriteProjects() noexcept;
  void refreshItems() noexcept;

  const Workspace& mWorkspace;
  const FilePath mRecentProjectsFp;
  const FilePath mFavoriteProjectsFp;
  QList<FilePath> mRecentProjects;
  QList<FilePath> mFavoriteProjects;
  std::vector<ui::QuickAccessItemData> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
