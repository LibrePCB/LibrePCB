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

#ifndef LIBREPCB_EDITOR_CATEGORYTREEMODEL_H
#define LIBREPCB_EDITOR_CATEGORYTREEMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class WorkspaceLibraryDb;
class WorkspaceSettings;

namespace editor {

/*******************************************************************************
 *  Class CategoryTreeModel
 ******************************************************************************/

/**
 * @brief The CategoryTreeModel class
 */
class CategoryTreeModel : public QObject,
                          public slint::Model<ui::TreeViewItemData> {
  Q_OBJECT

public:
  // Types
  enum class Filter {
    /// Show all component categories, even empty ones
    CmpCat = 1 << 0,
    /// Show all package categories, even empty ones
    PkgCat = 1 << 4,
  };
  Q_DECLARE_FLAGS(Filters, Filter)

  // Constructors / Destructor
  CategoryTreeModel() = delete;
  CategoryTreeModel(const CategoryTreeModel& other) = delete;
  explicit CategoryTreeModel(
      const WorkspaceLibraryDb& db, const WorkspaceSettings& ws,
      Filters filters, const std::optional<Uuid>& hiddenCategory = std::nullopt,
      QObject* parent = nullptr) noexcept;
  virtual ~CategoryTreeModel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::TreeViewItemData> row_data(std::size_t i) const override;

  // Operator Overloadings
  CategoryTreeModel& operator=(const CategoryTreeModel& rhs) = delete;

private:  // Methods
  void refresh() noexcept;
  template <typename T>
  void loadChilds(const std::optional<Uuid>& parent, int level);

private:  // Data
  const WorkspaceLibraryDb& mDb;
  const WorkspaceSettings& mSettings;
  const Filters mFilters;
  const std::optional<Uuid> mHiddenCategory;
  const slint::Image mIcon;
  std::vector<ui::TreeViewItemData> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
