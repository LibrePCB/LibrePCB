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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPSYMBVARPINSIGNALMAPEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPSYMBVARPINSIGNALMAPEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/componentsignal.h>
#include <librepcb/library/cmp/componentsymbolvariant.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {

class Symbol;
class ComponentSymbolVariant;

namespace editor {

/*******************************************************************************
 *  Class CompSymbVarPinSignalMapEditorWidget
 ******************************************************************************/

/**
 * @brief The CompSymbVarPinSignalMapEditorWidget class
 */
class CompSymbVarPinSignalMapEditorWidget final : public QWidget {
  Q_OBJECT

private:  // Types
  enum Column {
    COLUMN_SYMBOL = 0,
    COLUMN_PIN,
    COLUMN_SIGNAL,
    COLUMN_DISPLAYTYPE,
    _COLUMN_COUNT
  };

public:
  // Constructors / Destructor
  explicit CompSymbVarPinSignalMapEditorWidget(
      QWidget* parent = nullptr) noexcept;
  CompSymbVarPinSignalMapEditorWidget(
      const CompSymbVarPinSignalMapEditorWidget& other) = delete;
  ~CompSymbVarPinSignalMapEditorWidget() noexcept;

  // General Methods
  void setVariant(const workspace::Workspace& ws,
                  const ComponentSignalList&  sigs,
                  ComponentSymbolVariant&     variant) noexcept;
  void updateVariant() noexcept { updateTable(mSelectedItem, mSelectedPin); }

  // Operator Overloadings
  CompSymbVarPinSignalMapEditorWidget& operator       =(
      const CompSymbVarPinSignalMapEditorWidget& rhs) = delete;

signals:
  void edited();

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void componentSignalChanged(int index) noexcept;
  void displayTypeChanged(const CmpSigPinDisplayType& dt) noexcept;
  void btnAutoAssignSignalsClicked() noexcept;

private:  // Methods
  void updateTable(tl::optional<Uuid> selItem = tl::nullopt,
                   tl::optional<Uuid> selPin  = tl::nullopt) noexcept;
  void setTableRowContent(int row, const ComponentSymbolVariantItem& item,
                          const ComponentPinSignalMapItem& map, int itemNumber,
                          const Symbol* symbol) noexcept;
  void setComponentSignal(const Uuid& item, const Uuid& pin,
                          const tl::optional<Uuid>& signal) noexcept;
  void setDisplayType(const Uuid& item, const Uuid& pin,
                      const CmpSigPinDisplayType& dt) noexcept;
  int  getRowOfTableCellWidget(QObject* obj) const noexcept;
  tl::optional<Uuid> getItemUuidOfRow(int row) const noexcept;
  tl::optional<Uuid> getPinUuidOfRow(int row) const noexcept;
  int                getTotalPinCount() const noexcept;
  const QStringList& getLocaleOrder() const noexcept;

private:  // Data
  QTableWidget*               mTable;
  const workspace::Workspace* mWorkspace;
  const ComponentSignalList*  mSignalList;
  ComponentSymbolVariant*     mSymbolVariant;
  tl::optional<Uuid>          mSelectedItem;
  tl::optional<Uuid>          mSelectedPin;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPSYMBVARPINSIGNALMAPEDITORWIDGET_H
