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

#ifndef LIBREPCB_EDITOR_CATEGORYTREELABELTEXTBUILDER_H
#define LIBREPCB_EDITOR_CATEGORYTREELABELTEXTBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "categorytreebuilder.h"

#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class WorkspaceLibraryDb;

namespace editor {

/*******************************************************************************
 *  Class CategoryTreeLabelTextBuilder
 ******************************************************************************/

/**
 * @brief The CategoryTreeLabelTextBuilder class
 */
template <typename ElementType>
class CategoryTreeLabelTextBuilder final {
  Q_DECLARE_TR_FUNCTIONS(CategoryTreeLabelTextBuilder)

public:
  // Constructors / Destructor
  CategoryTreeLabelTextBuilder() = delete;
  CategoryTreeLabelTextBuilder(const CategoryTreeLabelTextBuilder& other) =
      delete;
  CategoryTreeLabelTextBuilder(const WorkspaceLibraryDb& db,
                               const QStringList& localeOrder,
                               bool nulloptIsRootCategory,
                               QLabel& label) noexcept;
  ~CategoryTreeLabelTextBuilder() noexcept;

  // Setters
  void setOneLine(bool oneLine) noexcept { mOneLine = oneLine; }
  void setPleaseChooseIfEmpty(bool choose) noexcept { mChooseIfEmpty = choose; }
  void setText(const QString& text) noexcept;
  void setErrorText(const QString& error) noexcept;

  // General Methods
  bool updateText(const std::optional<Uuid>& category) noexcept;

  // Operator Overloadings
  CategoryTreeLabelTextBuilder& operator=(
      const CategoryTreeLabelTextBuilder& rhs) = delete;

private:  // Methods
  void setText(const QStringList& lines) noexcept;

private:  // Data
  CategoryTreeBuilder<ElementType> mBuilder;
  QLabel& mLabel;
  bool mOneLine;
  bool mChooseIfEmpty;
};

typedef CategoryTreeLabelTextBuilder<ComponentCategory>
    ComponentCategoryTreeLabelTextBuilder;
typedef CategoryTreeLabelTextBuilder<PackageCategory>
    PackageCategoryTreeLabelTextBuilder;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
