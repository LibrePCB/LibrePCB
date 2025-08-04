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

#ifndef LIBREPCB_EDITOR_PARTLISTMODEL_H
#define LIBREPCB_EDITOR_PARTLISTMODEL_H

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

class PartEditor;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class PartListModel
 ******************************************************************************/

/**
 * @brief The PartListModel class
 */
class PartListModel final : public QObject, public slint::Model<ui::PartData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // PartListModel() = delete;
  PartListModel(const PartListModel& other) = delete;
  explicit PartListModel(QObject* parent = nullptr) noexcept;
  ~PartListModel() noexcept;

  // General Methods
  void setDefaultManufacturer(const SimpleString& mfr) noexcept;
  void setReferences(PartList* list, UndoStack* stack) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::PartData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i, const ui::PartData& data) noexcept override;

  // Operator Overloadings
  PartListModel& operator=(const PartListModel& rhs) = delete;

private:
  void trigger(int index, std::shared_ptr<Part> obj, ui::PartAction a) noexcept;
  void listEdited(const PartList& list, int index,
                  const std::shared_ptr<const Part>& item,
                  PartList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:
  PartList* mList;
  QPointer<UndoStack> mUndoStack;
  std::shared_ptr<Part> mNewPart;

  QList<std::shared_ptr<PartEditor>> mItems;

  // Slots
  PartList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
