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

#ifndef LIBREPCB_EDITOR_CATEGORYLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_CATEGORYLISTEDITORWIDGET_H

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

class Workspace;

namespace editor {

namespace Ui {
class CategoryListEditorWidget;
}

/*******************************************************************************
 *  Class CategoryListEditorWidget
 ******************************************************************************/

/**
 * @brief The CategoryListEditorWidget class
 */
class CategoryListEditorWidget : public QWidget {
  Q_OBJECT

public:
  enum class Categories {
    Component,
    Package,
  };

  // Constructors / Destructor
  CategoryListEditorWidget() = delete;
  explicit CategoryListEditorWidget(const Workspace& ws, Categories categories,
                                    QWidget* parent = nullptr) noexcept;
  CategoryListEditorWidget(const CategoryListEditorWidget& other) = delete;
  virtual ~CategoryListEditorWidget() noexcept;

  // Getters
  const QSet<Uuid>& getUuids() const noexcept { return mUuids; }

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setRequiresMinimumOneEntry(bool v) noexcept;
  void setUuids(const QSet<Uuid>& uuids) noexcept;

  // General Methods
  void openAddCategoryDialog() noexcept { btnAddClicked(); }

  // Operator Overloadings
  CategoryListEditorWidget& operator=(const CategoryListEditorWidget& rhs) =
      delete;

signals:
  void edited();
  void categoryAdded(const Uuid& category);
  void categoryRemoved(const Uuid& category);

private:  // Methods
  void btnAddClicked() noexcept;
  void btnRemoveClicked() noexcept;
  void addItem(const tl::optional<Uuid>& category) noexcept;
  void addItem(const tl::optional<Uuid>& category,
               const QStringList& lines) noexcept;
  void addItem(const tl::optional<Uuid>& category,
               const QString& text) noexcept;
  void updateColor() noexcept;
  QStringList buildTree(const tl::optional<Uuid>& category) const;
  tl::optional<Uuid> chooseCategoryWithDialog() noexcept;

private:  // Data
  const Workspace& mWorkspace;
  const Categories mCategories;
  QScopedPointer<Ui::CategoryListEditorWidget> mUi;
  bool mRequiresMinimumOneEntry;
  QSet<Uuid> mUuids;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
