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

namespace library {
namespace editor {

/*******************************************************************************
 *  Class ComponentSymbolVariantListWidget
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantListWidget class
 */
class ComponentSymbolVariantListWidget final : public QWidget {
  Q_OBJECT

private:  // Types
  enum Column {
    COLUMN_NAME = 0,
    COLUMN_DESCRIPTION,
    COLUMN_NORM,
    COLUMN_SYMBOLCOUNT,
    COLUMN_BUTTONS,
    _COLUMN_COUNT
  };

public:
  // Constructors / Destructor
  explicit ComponentSymbolVariantListWidget(QWidget* parent = nullptr) noexcept;
  ComponentSymbolVariantListWidget(
      const ComponentSymbolVariantListWidget& other) = delete;
  ~ComponentSymbolVariantListWidget() noexcept;

  // Setters
  void setReferences(
      UndoStack* undoStack, ComponentSymbolVariantList* variants,
      IF_ComponentSymbolVariantEditorProvider* editorProvider) noexcept;

  // General Methods
  void addDefaultSymbolVariant();

  // Operator Overloadings
  ComponentSymbolVariantListWidget& operator       =(
      const ComponentSymbolVariantListWidget& rhs) = delete;

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void cellDoubleClicked(int row, int column) noexcept;
  void btnEditClicked() noexcept;
  void btnAddRemoveClicked() noexcept;
  void btnUpClicked() noexcept;
  void btnDownClicked() noexcept;

private:  // Methods
  void variantListEdited(
      const ComponentSymbolVariantList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariant>& variant,
      ComponentSymbolVariantList::Event                    event) noexcept;
  void               updateTable() noexcept;
  void               setTableRowContent(int row, const tl::optional<Uuid>& uuid,
                                        const QString& name, const QString& desc,
                                        const QString& norm, int symbolCount) noexcept;
  void               addVariant(const QString& name, const QString& desc,
                                const QString& norm) noexcept;
  void               removeVariant(const Uuid& uuid) noexcept;
  void               moveVariantUp(int index) noexcept;
  void               moveVariantDown(int index) noexcept;
  void               editVariant(const Uuid& uuid) noexcept;
  int                getRowOfTableCellWidget(QObject* obj) const noexcept;
  tl::optional<Uuid> getUuidOfRow(int row) const noexcept;
  bool               allReferencesValid() const noexcept {
    return mUndoStack && mVariantList && mEditorProvider;
  }

  // row index <-> symbol variant index conversion methods
  int  newVariantRow() const noexcept { return mVariantList->count(); }
  int  indexToRow(int index) const noexcept { return index; }
  int  rowToIndex(int row) const noexcept { return row; }
  bool isExistingVariantRow(int row) const noexcept {
    return row >= 0 && row < mVariantList->count();
  }
  bool isNewVariantRow(int row) const noexcept {
    return row == newVariantRow();
  }

private:  // Data
  QTableWidget*                            mTable;
  UndoStack*                               mUndoStack;
  ComponentSymbolVariantList*              mVariantList;
  IF_ComponentSymbolVariantEditorProvider* mEditorProvider;
  tl::optional<Uuid>                       mSelectedVariant;

  // Slots
  ComponentSymbolVariantList::OnEditedSlot mVariantListEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTLISTWIDGET_H
