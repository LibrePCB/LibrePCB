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

#ifndef LIBREPCB_EDITOR_CATEGORYCHOOSERDIALOG_H
#define LIBREPCB_EDITOR_CATEGORYCHOOSERDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../workspace/categorytreemodel.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

namespace Ui {
class CategoryChooserDialog;
}

/*******************************************************************************
 *  Class CategoryChooserDialog
 ******************************************************************************/

/**
 * @brief The CategoryChooserDialog class
 */
class CategoryChooserDialog final : public QDialog {
  Q_OBJECT

public:
  using Filter = CategoryTreeModel::Filter;
  using Filters = CategoryTreeModel::Filters;

  // Constructors / Destructor
  CategoryChooserDialog() = delete;
  CategoryChooserDialog(const CategoryChooserDialog& other) = delete;
  explicit CategoryChooserDialog(const Workspace& ws, Filters filters,
                                 QWidget* parent = 0) noexcept;
  ~CategoryChooserDialog() noexcept;

  // Getters
  tl::optional<Uuid> getSelectedCategoryUuid() const noexcept;

  // Operator Overloadings
  CategoryChooserDialog& operator=(const CategoryChooserDialog& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::CategoryChooserDialog> mUi;
  QScopedPointer<CategoryTreeModel> mModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
