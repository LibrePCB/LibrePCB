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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class PackagePadListEditorWidget
 ******************************************************************************/

/**
 * @brief The PackagePadListEditorWidget class
 */
class PackagePadListEditorWidget final : public QWidget {
  Q_OBJECT

private:  // Types
  enum Column { COLUMN_NAME = 0, COLUMN_BUTTONS, _COLUMN_COUNT };

public:
  // Constructors / Destructor
  explicit PackagePadListEditorWidget(QWidget* parent = nullptr) noexcept;
  PackagePadListEditorWidget(const PackagePadListEditorWidget& other) = delete;
  ~PackagePadListEditorWidget() noexcept;

  // Setters
  void setReferences(PackagePadList& list, UndoStack* stack) noexcept;

  // Operator Overloadings
  PackagePadListEditorWidget& operator=(const PackagePadListEditorWidget& rhs) =
      delete;

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void tableCellChanged(int row, int column) noexcept;
  void btnAddRemoveClicked() noexcept;

private:  // Methods
  void padListEdited(const PackagePadList& list, int index,
                     const std::shared_ptr<const PackagePad>& pad,
                     PackagePadList::Event                    event) noexcept;
  void updateTable(const tl::optional<Uuid>& selected = tl::nullopt) noexcept;
  void setTableRowContent(int row, const tl::optional<Uuid>& uuid,
                          const QString& name) noexcept;
  void addPad(const QString& name) noexcept;
  void removePad(const Uuid& uuid) noexcept;
  CircuitIdentifier  setName(const Uuid& uuid, const QString& name) noexcept;
  int                getRowOfTableCellWidget(QObject* obj) const noexcept;
  tl::optional<Uuid> getUuidOfRow(int row) const noexcept;
  CircuitIdentifier  validateNameOrThrow(const QString& name) const;
  void               executeCommand(UndoCommand* cmd);
  QString            getNextPadNameProposal() const noexcept;

  // row index <-> signal index conversion methods
  int  newPadRow() const noexcept { return mPadList->count(); }
  int  indexToRow(int index) const noexcept { return index; }
  int  rowToIndex(int row) const noexcept { return row; }
  bool isExistingPadRow(int row) const noexcept {
    return row >= 0 && row < mPadList->count();
  }
  bool isNewPadRow(int row) const noexcept { return row == newPadRow(); }

private:  // Data
  QTableWidget*      mTable;
  PackagePadList*    mPadList;
  UndoStack*         mUndoStack;
  tl::optional<Uuid> mSelectedPad;

  // Slots
  PackagePadList::OnEditedSlot mPadListEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTEDITORWIDGET_H
