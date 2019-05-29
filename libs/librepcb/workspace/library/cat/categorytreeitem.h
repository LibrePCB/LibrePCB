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

#ifndef LIBREPCB_LIBRARY_CATEGORYTREEITEM_H
#define LIBREPCB_LIBRARY_CATEGORYTREEITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
class ComponentCategory;
class PackageCategory;
}  // namespace library

namespace workspace {

class WorkspaceLibraryDb;

/*******************************************************************************
 *  Class CategoryTreeFilter
 ******************************************************************************/

struct CategoryTreeFilter {
  enum Flag {
    SYMBOLS    = 1 << 0,  ///< Show items containing symbols
    PACKAGES   = 1 << 1,  ///< Show items containing packages
    COMPONENTS = 1 << 2,  ///< Show items containing components
    DEVICES    = 1 << 3,  ///< Show items containing devices
    ALL        = 1 << 4,  ///< Show all items, even empty ones
  };
  Q_DECLARE_FLAGS(Flags, Flag)
};

/*******************************************************************************
 *  Class CategoryTreeItem
 ******************************************************************************/

/**
 * @brief The CategoryTreeItem class
 */
template <typename ElementType>
class CategoryTreeItem final {
public:
  // Constructors / Destructor
  CategoryTreeItem()                              = delete;
  CategoryTreeItem(const CategoryTreeItem& other) = delete;
  CategoryTreeItem(const WorkspaceLibraryDb& library,
                   const QStringList localeOrder, CategoryTreeItem* parent,
                   const tl::optional<Uuid>& uuid,
                   CategoryTreeFilter::Flags filter) noexcept;
  ~CategoryTreeItem() noexcept;

  // Getters
  const tl::optional<Uuid>& getUuid() const noexcept { return mUuid; }
  unsigned int              getDepth() const noexcept { return mDepth; }
  int                       getColumnCount() const noexcept { return 1; }
  CategoryTreeItem*         getParent() const noexcept { return mParent; }
  CategoryTreeItem*         getChild(int index) const noexcept {
    return mChilds.value(index).data();
  }
  int      getChildCount() const noexcept { return mChilds.count(); }
  int      getChildNumber() const noexcept;
  QVariant data(int role) const noexcept;
  bool     isVisible() const noexcept { return mIsVisible; }

  // Operator Overloadings
  CategoryTreeItem& operator=(const CategoryTreeItem& rhs) = delete;

private:
  // Types
  using ChildType = QSharedPointer<CategoryTreeItem<ElementType>>;

  // Methods
  FilePath   getLatestCategory(const WorkspaceLibraryDb& lib) const;
  QSet<Uuid> getCategoryChilds(const WorkspaceLibraryDb& lib) const;
  bool       matchesFilter(const WorkspaceLibraryDb& lib,
                           CategoryTreeFilter::Flags filter) const;

  // Attributes
  CategoryTreeItem*  mParent;
  tl::optional<Uuid> mUuid;
  QString            mName;
  QString            mDescription;
  unsigned int       mDepth;  ///< this is to avoid endless recursion in the
                              ///< parent-child relationship
  QString          mExceptionMessage;
  bool             mIsVisible;
  QList<ChildType> mChilds;
};

typedef CategoryTreeItem<library::ComponentCategory> ComponentCategoryTreeItem;
typedef CategoryTreeItem<library::PackageCategory>   PackageCategoryTreeItem;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::workspace::CategoryTreeFilter::Flags)

#endif  // LIBREPCB_LIBRARY_CATEGORYTREEITEM_H
