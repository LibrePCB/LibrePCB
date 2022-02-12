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

#ifndef LIBREPCB_EDITOR_CATEGORYTREEBUILDER_H
#define LIBREPCB_EDITOR_CATEGORYTREEBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class Uuid;
class WorkspaceLibraryDb;

namespace editor {

/*******************************************************************************
 *  Class CategoryTreeBuilder
 ******************************************************************************/

/**
 * @brief Helper class to extract a category tree from
 *        ::librepcb::WorkspaceLibraryDb
 */
template <typename ElementType>
class CategoryTreeBuilder final {
  Q_DECLARE_TR_FUNCTIONS(CategoryTreeBuilder)

public:
  // Constructors / Destructor
  CategoryTreeBuilder() = delete;
  CategoryTreeBuilder(const CategoryTreeBuilder& other) = delete;
  CategoryTreeBuilder(const WorkspaceLibraryDb& db,
                      const QStringList& localeOrder,
                      bool nulloptIsRootCategory) noexcept;
  ~CategoryTreeBuilder() noexcept;

  // General Methods

  /**
   * @brief Build the parents tree for a specific category
   *
   * @param category  The category to get the tree from. If tl::nullopt,
   *                  it is assumed to represent the root category.
   * @param success   If not sullptr, this is set to whether the tree was
   *                  successfully built or not.
   * @return All category names. The top level category comes first (root
   *         category if `nulloptIsRootCategory=true` passed to the
   *         constructor), then down the tree, with the passed category as
   *         the last element. In case of invalid categories, the returned list
   *         is either empty or contains error messages.
   * @throw In case of database errors.
   */
  QStringList buildTree(const tl::optional<Uuid>& category,
                        bool* success = nullptr) const;

  // Operator Overloadings
  CategoryTreeBuilder& operator=(const CategoryTreeBuilder& rhs) = delete;

private:  // Methods
  bool getParentNames(const tl::optional<Uuid>& category, QStringList& names,
                      QSet<FilePath>& filePaths) const;

private:  // Data
  const WorkspaceLibraryDb& mDb;
  const QStringList& mLocaleOrder;
  const bool mNulloptIsRootCategory;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
