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

#ifndef LIBREPCB_EDITOR_DEVICEPINOUTMODEL_H
#define LIBREPCB_EDITOR_DEVICEPINOUTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/dev/devicepadsignalmap.h>
#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class DevicePinoutModel
 ******************************************************************************/

/**
 * @brief The DevicePinoutModel class
 */
class DevicePinoutModel : public QObject,
                          public slint::Model<ui::DevicePinoutData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // DevicePinoutModel() = delete;
  DevicePinoutModel(const DevicePinoutModel& other) = delete;
  explicit DevicePinoutModel(QObject* parent = nullptr) noexcept;
  virtual ~DevicePinoutModel() noexcept;

  // General Methods
  void setList(DevicePadSignalMap* list) noexcept;
  void setSignals(const ComponentSignalList* sigs) noexcept;
  void setPads(const PackagePadList* pads) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  // void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::DevicePinoutData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::DevicePinoutData& data) noexcept override;

  // Operator Overloadings
  DevicePinoutModel& operator=(const DevicePinoutModel& rhs) = delete;

private:
  ui::DevicePinoutData createItem(const DevicePadSignalMapItem& item) noexcept;
  void listEdited(const DevicePadSignalMap& list, int index,
                  const std::shared_ptr<const DevicePadSignalMapItem>& item,
                  DevicePadSignalMap::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:
  DevicePadSignalMap* mList;
  const ComponentSignalList* mSignals;
  const PackagePadList* mPads;
  UndoStack* mUndoStack;

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
