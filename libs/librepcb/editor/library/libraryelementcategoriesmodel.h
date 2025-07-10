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

#ifndef LIBREPCB_EDITOR_LIBRARYELEMENTCATEGORIESMODEL_H
#define LIBREPCB_EDITOR_LIBRARYELEMENTCATEGORIESMODEL_H

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

class Workspace;

namespace editor {

/*******************************************************************************
 *  Class LibraryElementCategoriesModel
 ******************************************************************************/

/**
 * @brief The LibraryElementCategoriesModel class
 */
class LibraryElementCategoriesModel final
  : public QObject,
    public slint::Model<ui::LibraryElementCategoryData> {
  Q_OBJECT

public:
  // Types
  enum class Type {
    ComponentCategory,
    PackageCategory,
  };

  // Constructors / Destructor
  LibraryElementCategoriesModel() = delete;
  LibraryElementCategoriesModel(const LibraryElementCategoriesModel& other) =
      delete;
  explicit LibraryElementCategoriesModel(const Workspace& ws, Type type,
                                         QObject* parent = nullptr) noexcept;
  virtual ~LibraryElementCategoriesModel() noexcept;

  // General Methods
  const QSet<Uuid>& getCategories() const noexcept { return mCategories; }
  void setCategories(const QSet<Uuid>& categories) noexcept;
  void add(const Uuid& category) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::LibraryElementCategoryData> row_data(
      std::size_t i) const override;
  void set_row_data(
      std::size_t i,
      const ui::LibraryElementCategoryData& data) noexcept override;

  // Operator Overloadings
  LibraryElementCategoriesModel& operator=(
      const LibraryElementCategoriesModel& rhs) = delete;

signals:
  void modified(const QSet<Uuid>& categories);

private:
  void refresh() noexcept;
  template <typename T>
  void loadItems();

  const Workspace& mWs;
  const Type mType;
  QSet<Uuid> mCategories;
  std::vector<ui::LibraryElementCategoryData> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
