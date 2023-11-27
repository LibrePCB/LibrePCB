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

#ifndef LIBREPCB_EDITOR_COMPONENTPINSIGNALMAPMODEL_H
#define LIBREPCB_EDITOR_COMPONENTPINSIGNALMAPMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../modelview/comboboxdelegate.h"

#include <librepcb/core/library/cmp/componentsignal.h>
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
 *  Class ComponentPinSignalMapModel
 ******************************************************************************/

/**
 * @brief The ComponentPinSignalMapModel class
 */
class ComponentPinSignalMapModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_SYMBOL,
    COLUMN_PIN,
    COLUMN_SIGNAL,
    COLUMN_DISPLAY,
    _COLUMN_COUNT
  };

  // Constructors / Destructor
  ComponentPinSignalMapModel() = delete;
  ComponentPinSignalMapModel(const ComponentPinSignalMapModel& other) noexcept;
  explicit ComponentPinSignalMapModel(QObject* parent = nullptr) noexcept;
  ~ComponentPinSignalMapModel() noexcept;

  // Setters
  void setSymbolVariant(ComponentSymbolVariant* variant) noexcept;
  void setSignalList(const ComponentSignalList* list) noexcept;
  void setSymbolsCache(
      const std::shared_ptr<const LibraryElementCache>& cache) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // General Methods
  void autoAssignSignals() noexcept;

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
  ComponentPinSignalMapModel& operator=(
      const ComponentPinSignalMapModel& rhs) noexcept;

private:
  void symbolItemsEdited(
      const ComponentSymbolVariantItemList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariantItem>& item,
      ComponentSymbolVariantItemList::Event event) noexcept;
  void signalListEdited(const ComponentSignalList& list, int index,
                        const std::shared_ptr<const ComponentSignal>& signal,
                        ComponentSignalList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  void updateSignalComboBoxItems() noexcept;
  void getRowItem(
      int row, int& symbolItemIndex,
      std::shared_ptr<ComponentSymbolVariantItem>& symbolItem,
      std::shared_ptr<ComponentPinSignalMapItem>& mapItem) const noexcept;

private:  // Data
  ComponentSymbolVariant* mSymbolVariant;
  const ComponentSignalList* mSignals;
  std::shared_ptr<const LibraryElementCache> mSymbolsCache;
  UndoStack* mUndoStack;
  ComboBoxDelegate::Items mSignalComboBoxItems;
  ComboBoxDelegate::Items mDisplayTypeComboBoxItems;

  // Slots
  ComponentSymbolVariantItemList::OnEditedSlot mOnItemsEditedSlot;
  ComponentSignalList::OnEditedSlot mOnSignalsEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
