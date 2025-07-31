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

#ifndef LIBREPCB_EDITOR_COMPONENTSIGNALNAMELISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTSIGNALNAMELISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/cmp/componentsignal.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentSignalNameListModel
 ******************************************************************************/

/**
 * @brief The ComponentSignalNameListModel class
 */
class ComponentSignalNameListModel final
  : public QObject,
    public slint::Model<slint::SharedString> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // ComponentSignalNameListModel() = delete;
  ComponentSignalNameListModel(const ComponentSignalNameListModel& other) =
      delete;
  explicit ComponentSignalNameListModel(QObject* parent = nullptr) noexcept;
  ~ComponentSignalNameListModel() noexcept;

  // General Methods
  void setReferences(ComponentSignalList* list, UndoStack* stack) noexcept;
  std::optional<Uuid> getUuid(std::size_t i) const noexcept;
  int getIndexOf(const std::optional<Uuid>& sig) const noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<slint::SharedString> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i, const slint::SharedString& data) override;

  // Operator Overloadings
  ComponentSignalNameListModel& operator=(
      const ComponentSignalNameListModel& rhs) = delete;

signals:
  void modified();

private:
  void updateItems() noexcept;
  void listEdited(const ComponentSignalList& list, int index,
                  const std::shared_ptr<const ComponentSignal>& item,
                  ComponentSignalList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:
  ComponentSignalList* mList;
  QPointer<UndoStack> mUndoStack;

  QList<std::shared_ptr<ComponentSignal>> mSignalsSorted;

  // Slots
  ComponentSignalList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
