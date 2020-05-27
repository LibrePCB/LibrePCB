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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;
class EditableTableWidget;
class SortFilterProxyModel;

namespace library {
namespace editor {

class PackagePadListModel;

/*******************************************************************************
 *  Class PackagePadListEditorWidget
 ******************************************************************************/

/**
 * @brief The PackagePadListEditorWidget class
 */
class PackagePadListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PackagePadListEditorWidget(QWidget* parent = nullptr) noexcept;
  PackagePadListEditorWidget(const PackagePadListEditorWidget& other) = delete;
  ~PackagePadListEditorWidget() noexcept;

  // Setters
  void setReferences(PackagePadList& list, UndoStack* stack) noexcept;

  // Operator Overloadings
  PackagePadListEditorWidget& operator=(const PackagePadListEditorWidget& rhs) =
      delete;

private:
  QScopedPointer<PackagePadListModel>  mModel;
  QScopedPointer<SortFilterProxyModel> mProxy;
  QScopedPointer<EditableTableWidget>  mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTEDITORWIDGET_H
