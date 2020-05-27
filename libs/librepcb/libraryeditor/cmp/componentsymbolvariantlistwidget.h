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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTLISTWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTLISTWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "if_componentsymbolvarianteditorprovider.h"

#include <librepcb/library/cmp/componentsymbolvariant.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;
class EditableTableWidget;

namespace library {
namespace editor {

class ComponentSymbolVariantListModel;

/*******************************************************************************
 *  Class ComponentSymbolVariantListWidget
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantListWidget class
 */
class ComponentSymbolVariantListWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit ComponentSymbolVariantListWidget(QWidget* parent = nullptr) noexcept;
  ComponentSymbolVariantListWidget(
      const ComponentSymbolVariantListWidget& other) = delete;
  ~ComponentSymbolVariantListWidget() noexcept;

  // Setters
  void setReferences(
      UndoStack* undoStack, ComponentSymbolVariantList* list,
      IF_ComponentSymbolVariantEditorProvider* editorProvider) noexcept;

  // Operator Overloadings
  ComponentSymbolVariantListWidget& operator       =(
      const ComponentSymbolVariantListWidget& rhs) = delete;

private:  // Methods
  void btnEditClicked(const QVariant& data) noexcept;
  void viewDoubleClicked(const QModelIndex& index) noexcept;
  void editVariant(const Uuid& uuid) noexcept;

private:  // Data
  QScopedPointer<ComponentSymbolVariantListModel> mModel;
  QScopedPointer<EditableTableWidget>             mView;
  ComponentSymbolVariantList*                     mSymbolVariantList;
  UndoStack*                                      mUndoStack;
  IF_ComponentSymbolVariantEditorProvider*        mEditorProvider;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTLISTWIDGET_H
