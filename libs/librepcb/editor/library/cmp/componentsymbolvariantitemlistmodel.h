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

#ifndef LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/cmp/componentsymbolvariant.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class LibraryElementCache;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentSymbolVariantItemListModel
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantItemListModel class
 */
class ComponentSymbolVariantItemListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_NUMBER,
    COLUMN_SYMBOL,
    COLUMN_SUFFIX,
    COLUMN_ISREQUIRED,
    COLUMN_X,
    COLUMN_Y,
    COLUMN_ROTATION,
    COLUMN_ACTIONS,
    _COLUMN_COUNT
  };

  // Constructors / Destructor
  ComponentSymbolVariantItemListModel() = delete;
  ComponentSymbolVariantItemListModel(
      const ComponentSymbolVariantItemListModel& other) noexcept;
  explicit ComponentSymbolVariantItemListModel(
      QObject* parent = nullptr) noexcept;
  ~ComponentSymbolVariantItemListModel() noexcept;

  // Setters
  void setItemList(ComponentSymbolVariantItemList* list) noexcept;
  void setSymbolsCache(
      const std::shared_ptr<const LibraryElementCache>& cache) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void addItem(const QVariant& editData) noexcept;
  void removeItem(const QVariant& editData) noexcept;
  void moveItemUp(const QVariant& editData) noexcept;
  void moveItemDown(const QVariant& editData) noexcept;
  void changeSymbol(const QVariant& editData, const Uuid& symbol) noexcept;

  // Inherited from QAbstractItemModel
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;

  // Operator Overloadings
  ComponentSymbolVariantItemListModel& operator=(
      const ComponentSymbolVariantItemListModel& rhs) noexcept;

private:
  void itemListEdited(
      const ComponentSymbolVariantItemList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariantItem>& item,
      ComponentSymbolVariantItemList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:  // Data
  ComponentSymbolVariantItemList* mItemList;
  std::shared_ptr<const LibraryElementCache> mSymbolsCache;
  UndoStack* mUndoStack;
  tl::optional<Uuid> mNewSymbolUuid;
  QString mNewSuffix;
  bool mNewIsRequired;
  Point mNewPosition;
  Angle mNewRotation;

  // Slots
  ComponentSymbolVariantItemList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
