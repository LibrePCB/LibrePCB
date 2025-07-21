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

#ifndef LIBREPCB_EDITOR_COMPONENTPINOUTLISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTPINOUTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/cmp/componentpinsignalmap.h>
#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class ComponentSignalNameListModel;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentPinoutListModel
 ******************************************************************************/

/**
 * @brief The ComponentPinoutListModel class
 */
class ComponentPinoutListModel final
  : public QObject,
    public slint::Model<ui::ComponentPinoutData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // ComponentPinoutListModel() = delete;
  ComponentPinoutListModel(const ComponentPinoutListModel& other) = delete;
  explicit ComponentPinoutListModel(QObject* parent = nullptr) noexcept;
  ~ComponentPinoutListModel() noexcept;

  // General Methods
  void setReferences(ComponentPinSignalMap* list, const SymbolPinList* pins,
                     const std::shared_ptr<ComponentSignalNameListModel>& sigs,
                     UndoStack* stack) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ComponentPinoutData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::ComponentPinoutData& data) noexcept override;

  // Operator Overloadings
  ComponentPinoutListModel& operator=(const ComponentPinoutListModel& rhs) =
      delete;

private:
  ui::ComponentPinoutData createItem(
      const ComponentPinSignalMapItem& obj) noexcept;
  void refresh() noexcept;
  void listEdited(const ComponentPinSignalMap& list, int index,
                  const std::shared_ptr<const ComponentPinSignalMapItem>& item,
                  ComponentPinSignalMap::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:
  ComponentPinSignalMap* mList;
  std::shared_ptr<ComponentSignalNameListModel> mSignals;
  const SymbolPinList* mPins;
  QPointer<UndoStack> mUndoStack;

  QList<ui::ComponentPinoutData> mItems;

  // Slots
  ComponentPinSignalMap::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
