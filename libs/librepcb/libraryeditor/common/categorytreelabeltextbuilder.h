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

#ifndef LIBREPCB_LIBRARY_EDITOR_CATEGORYTREELABELTEXTBUILDER_H
#define LIBREPCB_LIBRARY_EDITOR_CATEGORYTREELABELTEXTBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/uuid.h>
#include <librepcb/library/cat/componentcategory.h>
#include <librepcb/library/cat/packagecategory.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace workspace {
class WorkspaceLibraryDb;
}

namespace library {
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
  CategoryTreeLabelTextBuilder(const workspace::WorkspaceLibraryDb& db,
                               const QStringList& localeOrder,
                               QLabel& label) noexcept;
  ~CategoryTreeLabelTextBuilder() noexcept;

  // Setters
  void setHighlightLastLine(bool highlight) noexcept {
    mHighlightLastLine = highlight;
  }
  void setEndlessRecursionUuid(const Uuid& uuid) noexcept {
    mEndlessRecursionUuid = uuid;
  }
  void setOneLine(bool oneLine) noexcept { mOneLine = oneLine; }
  void setText(const QString& text) noexcept;
  void setErrorText(const QString& error) noexcept;

  // General Methods
  bool updateText(const tl::optional<Uuid>& category,
                  const QString& lastLine = QString()) noexcept;

  // Operator Overloadings
  CategoryTreeLabelTextBuilder& operator=(
      const CategoryTreeLabelTextBuilder& rhs) = delete;

private:  // Methods
  bool updateText(const QList<Uuid>& uuids, const QString& lastLine) noexcept;
  void setText(const QStringList& lines) noexcept;
  FilePath getLatestCategory(const Uuid& category) const;
  QList<Uuid> getCategoryParents(const Uuid& category) const;

private:  // Data
  const workspace::WorkspaceLibraryDb& mDb;
  const QStringList& mLocaleOrder;
  QLabel& mLabel;
  bool mHighlightLastLine;
  tl::optional<Uuid> mEndlessRecursionUuid;
  bool mOneLine;
};

typedef CategoryTreeLabelTextBuilder<library::ComponentCategory>
    ComponentCategoryTreeLabelTextBuilder;
typedef CategoryTreeLabelTextBuilder<library::PackageCategory>
    PackageCategoryTreeLabelTextBuilder;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_CATEGORYTREELABELTEXTBUILDER_H
