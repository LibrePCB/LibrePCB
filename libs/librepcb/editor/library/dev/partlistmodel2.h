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

#ifndef LIBREPCB_EDITOR_PARTLISTMODEL2_H
#define LIBREPCB_EDITOR_PARTLISTMODEL2_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/dev/part.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class PartListModel2
 ******************************************************************************/

/**
 * @brief The PartListModel2 class
 */
class PartListModel2 : public QObject, public slint::Model<ui::DevicePartData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // PartListModel2() = delete;
  PartListModel2(const PartListModel2& other) = delete;
  explicit PartListModel2(QObject* parent = nullptr) noexcept;
  virtual ~PartListModel2() noexcept;

  // General Methods
  void setList(PartList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  // void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::DevicePartData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::DevicePartData& data) noexcept override;

  // Operator Overloadings
  PartListModel2& operator=(const PartListModel2& rhs) = delete;

private:
  ui::DevicePartData createItem(const Part& item) noexcept;
  void listEdited(const PartList& list, int index,
                  const std::shared_ptr<const Part>& item,
                  PartList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:
  PartList* mList;
  UndoStack* mUndoStack;

  QList<ui::DevicePartData> mItems;

  // Slots
  PartList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
