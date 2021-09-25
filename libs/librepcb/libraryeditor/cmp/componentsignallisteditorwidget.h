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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/componentsignal.h>

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

class ComponentSignalListModel;

/*******************************************************************************
 *  Class ComponentSignalListEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentSignalListEditorWidget class
 */
class ComponentSignalListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit ComponentSignalListEditorWidget(QWidget* parent = nullptr) noexcept;
  ComponentSignalListEditorWidget(
      const ComponentSignalListEditorWidget& other) = delete;
  ~ComponentSignalListEditorWidget() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(UndoStack* undoStack, ComponentSignalList* list) noexcept;

  // Operator Overloadings
  ComponentSignalListEditorWidget& operator=(
      const ComponentSignalListEditorWidget& rhs) = delete;

private:
  QScopedPointer<ComponentSignalListModel> mModel;
  QScopedPointer<SortFilterProxyModel> mProxy;
  QScopedPointer<EditableTableWidget> mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTEDITORWIDGET_H
