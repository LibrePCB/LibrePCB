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

#ifndef LIBREPCB_EDITOR_DEVICEPINOUTLISTMODEL_H
#define LIBREPCB_EDITOR_DEVICEPINOUTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/dev/devicepadsignalmap.h>
#include <librepcb/core/library/pkg/packagepad.h>

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
 *  Class DevicePinoutListModel
 ******************************************************************************/

/**
 * @brief The DevicePinoutListModel class
 */
class DevicePinoutListModel final : public QObject,
                                    public slint::Model<ui::DevicePinoutData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // DevicePinoutListModel() = delete;
  DevicePinoutListModel(const DevicePinoutListModel& other) = delete;
  explicit DevicePinoutListModel(QObject* parent = nullptr) noexcept;
  ~DevicePinoutListModel() noexcept;

  // General Methods
  void setReferences(DevicePadSignalMap* list, const PackagePadList* pads,
                     const std::shared_ptr<ComponentSignalNameListModel>& sigs,
                     UndoStack* stack) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::DevicePinoutData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::DevicePinoutData& data) noexcept override;

  // Operator Overloadings
  DevicePinoutListModel& operator=(const DevicePinoutListModel& rhs) = delete;

private:
  ui::DevicePinoutData createItem(const DevicePadSignalMapItem& obj) noexcept;
  void refresh() noexcept;
  void listEdited(const DevicePadSignalMap& list, int index,
                  const std::shared_ptr<const DevicePadSignalMapItem>& item,
                  DevicePadSignalMap::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:
  DevicePadSignalMap* mList;
  const PackagePadList* mPads;
  std::shared_ptr<ComponentSignalNameListModel> mSignals;
  QPointer<UndoStack> mUndoStack;

  QList<ui::DevicePinoutData> mItems;

  // Slots
  DevicePadSignalMap::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
