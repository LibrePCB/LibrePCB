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

#ifndef LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTMODEL_H
#define LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/pkg/footprint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class FootprintListModel
 ******************************************************************************/

/**
 * @brief The FootprintListModel class
 */
class FootprintListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column { COLUMN_NAME, COLUMN_ACTIONS, _COLUMN_COUNT };

  // Constructors / Destructor
  FootprintListModel() = delete;
  FootprintListModel(const FootprintListModel& other) noexcept;
  explicit FootprintListModel(QObject* parent = nullptr) noexcept;
  ~FootprintListModel() noexcept;

  // Setters
  void setFootprintList(FootprintList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void addFootprint(const QVariant& editData) noexcept;
  void copyFootprint(const QVariant& editData) noexcept;
  void removeFootprint(const QVariant& editData) noexcept;
  void moveFootprintUp(const QVariant& editData) noexcept;
  void moveFootprintDown(const QVariant& editData) noexcept;

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
  FootprintListModel& operator=(const FootprintListModel& rhs) noexcept;

private:
  void footprintListEdited(const FootprintList& list, int index,
                           const std::shared_ptr<const Footprint>& footprint,
                           FootprintList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  ElementName validateNameOrThrow(const QString& name) const;

private:  // Data
  FootprintList* mFootprintList;
  UndoStack* mUndoStack;
  QString mNewName;

  // Slots
  FootprintList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
