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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/componentsymbolvariantitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CenteredCheckBox;
class IF_GraphicsLayerProvider;

namespace workspace {
class Workspace;
}

namespace library {
namespace editor {

/*******************************************************************************
 *  Class ComponentSymbolVariantItemListEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantItemListEditorWidget class
 */
class ComponentSymbolVariantItemListEditorWidget final : public QWidget {
  Q_OBJECT

private:  // Types
  enum Column {
    COLUMN_NUMBER = 0,
    COLUMN_SYMBOL,
    COLUMN_SUFFIX,
    COLUMN_ISREQUIRED,
    COLUMN_POS_X,
    COLUMN_POS_Y,
    COLUMN_ROTATION,
    COLUMN_BUTTONS,
    _COLUMN_COUNT
  };

public:
  // Constructors / Destructor
  explicit ComponentSymbolVariantItemListEditorWidget(
      QWidget* parent = nullptr) noexcept;
  ComponentSymbolVariantItemListEditorWidget(
      const ComponentSymbolVariantItemListEditorWidget& other) = delete;
  ~ComponentSymbolVariantItemListEditorWidget() noexcept;

  // Setters
  void setVariant(const workspace::Workspace&     ws,
                  const IF_GraphicsLayerProvider& layerProvider,
                  ComponentSymbolVariantItemList& items) noexcept;

  // Operator Overloadings
  ComponentSymbolVariantItemListEditorWidget& operator       =(
      const ComponentSymbolVariantItemListEditorWidget& rhs) = delete;

signals:
  void edited();

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void tableCellChanged(int row, int column) noexcept;
  void isRequiredChanged(bool checked) noexcept;
  void btnChooseSymbolClicked() noexcept;
  void btnAddRemoveClicked() noexcept;
  void btnUpClicked() noexcept;
  void btnDownClicked() noexcept;

private:  // Methods
  void updateTable(tl::optional<Uuid> selected = tl::nullopt) noexcept;
  void setTableRowContent(int row, int number, const tl::optional<Uuid>& uuid,
                          const tl::optional<Uuid>& symbol,
                          const QString& suffix, bool required,
                          const Point& pos, const Angle& rot) noexcept;
  void addItem(const Uuid& symbol, const QString& suffix, bool required,
               const Point& pos, const Angle& rot) noexcept;
  void removeItem(const Uuid& uuid) noexcept;
  void moveItemUp(int index) noexcept;
  void moveItemDown(int index) noexcept;
  void setSymbolUuid(const Uuid& uuid, const Uuid& symbol) noexcept;
  void setIsRequired(const Uuid& uuid, bool required) noexcept;
  void setSuffix(const Uuid& uuid, const QString& suffix) noexcept;
  void setPosX(const Uuid& uuid, const Length& x) noexcept;
  void setPosY(const Uuid& uuid, const Length& y) noexcept;
  void setRotation(const Uuid& uuid, const Angle& rot) noexcept;
  int  getRowOfTableCellWidget(QObject* obj) const noexcept;
  tl::optional<Uuid> getUuidOfRow(int row) const noexcept;

  // row index <-> item index conversion methods
  int  newItemRow() const noexcept { return mItems->count(); }
  int  indexToRow(int index) const noexcept { return index; }
  int  rowToIndex(int row) const noexcept { return row; }
  bool isExistingItemRow(int row) const noexcept {
    return row >= 0 && row < mItems->count();
  }
  bool isNewItemRow(int row) const noexcept { return row == newItemRow(); }

private:  // Data
  QTableWidget*                   mTable;
  const workspace::Workspace*     mWorkspace;
  const IF_GraphicsLayerProvider* mLayerProvider;
  ComponentSymbolVariantItemList* mItems;
  tl::optional<Uuid>              mSelectedItem;
  QLabel*                         mNewSymbolLabel;
  CenteredCheckBox*               mNewRequiredCheckbox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTEDITORWIDGET_H
