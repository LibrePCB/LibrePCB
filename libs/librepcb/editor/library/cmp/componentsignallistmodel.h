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

#ifndef LIBREPCB_EDITOR_COMPONENTSIGNALLISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTSIGNALLISTMODEL_H

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
 *  Class ComponentSignalListModel
 ******************************************************************************/

/**
 * @brief The ComponentSignalListModel class
 */
class ComponentSignalListModel : public QObject,
                                 public slint::Model<ui::ComponentSignalData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // ComponentSignalListModel() = delete;
  ComponentSignalListModel(const ComponentSignalListModel& other) = delete;
  explicit ComponentSignalListModel(QObject* parent = nullptr) noexcept;
  virtual ~ComponentSignalListModel() noexcept;

  // General Methods
  void setSignalList(ComponentSignalList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  bool add(const QStringList& names) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ComponentSignalData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::ComponentSignalData& data) noexcept override;

  // Operator Overloadings
  ComponentSignalListModel& operator=(const ComponentSignalListModel& rhs) =
      delete;

private:
  ui::ComponentSignalData createItem(const ComponentSignal& sig) noexcept;
  void signalListEdited(const ComponentSignalList& list, int index,
                        const std::shared_ptr<const ComponentSignal>& signal,
                        ComponentSignalList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  static void throwDuplicateNameError(const QString& name);
  static QString cleanForcedNetName(const QString& name) noexcept;

private:
  ComponentSignalList* mSignalList;
  UndoStack* mUndoStack;

  QList<ui::ComponentSignalData> mItems;

  // Slots
  ComponentSignalList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
