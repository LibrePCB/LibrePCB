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

#ifndef LIBREPCB_EDITOR_PACKAGEMODELLISTMODEL_H
#define LIBREPCB_EDITOR_PACKAGEMODELLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/packagemodel.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Package;

namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class PackageModelListModel
 ******************************************************************************/

/**
 * @brief The PackageModelListModel class
 */
class PackageModelListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column { COLUMN_ENABLED, COLUMN_NAME, COLUMN_ACTIONS, _COLUMN_COUNT };

  // Constructors / Destructor
  PackageModelListModel() = delete;
  PackageModelListModel(const PackageModelListModel& other) noexcept;
  explicit PackageModelListModel(QObject* parent = nullptr) noexcept;
  ~PackageModelListModel() noexcept;

  // Setters
  void setPackage(Package* package) noexcept;
  void setFootprint(std::shared_ptr<Footprint> footprint) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void add(const QPersistentModelIndex& itemIndex) noexcept;
  void remove(const QPersistentModelIndex& itemIndex) noexcept;
  void edit(const QPersistentModelIndex& itemIndex) noexcept;
  void moveUp(const QPersistentModelIndex& itemIndex) noexcept;
  void moveDown(const QPersistentModelIndex& itemIndex) noexcept;

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
  PackageModelListModel& operator=(const PackageModelListModel& rhs) noexcept;

signals:
  void newModelAdded(int index);

private:
  void modelListEdited(const PackageModelList& list, int index,
                       const std::shared_ptr<const PackageModel>& obj,
                       PackageModelList::Event event) noexcept;
  void footprintEdited(const Footprint& obj, Footprint::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  ElementName validateNameOrThrow(const QString& name) const;
  bool chooseStepFile(QByteArray& content, FilePath* selectedFile = nullptr);

private:  // Data
  QPointer<Package> mPackage;
  std::shared_ptr<Footprint> mFootprint;
  UndoStack* mUndoStack;
  bool mNewEnabled;
  QString mNewName;

  // Slots
  PackageModelList::OnEditedSlot mOnEditedSlot;
  Footprint::OnEditedSlot mOnFootprintEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
