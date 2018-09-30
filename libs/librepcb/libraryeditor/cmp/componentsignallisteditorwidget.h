/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/componentsignal.h>

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
 *  Class ComponentSignalListEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentSignalListEditorWidget class
 *
 * @author ubruhin
 * @date 2017-03-12
 */
class ComponentSignalListEditorWidget final
  : public QWidget,
    private ComponentSignalList::IF_Observer {
  Q_OBJECT

private:  // Types
  enum Column {
    COLUMN_NAME = 0,
    // COLUMN_ROLE,
    COLUMN_ISREQUIRED,
    // COLUMN_ISNEGATED,
    // COLUMN_ISCLOCK,
    COLUMN_FORCEDNETNAME,
    COLUMN_BUTTONS,
    _COLUMN_COUNT
  };

public:
  // Constructors / Destructor
  explicit ComponentSignalListEditorWidget(QWidget* parent = nullptr) noexcept;
  ComponentSignalListEditorWidget(
      const ComponentSignalListEditorWidget& other) = delete;
  ~ComponentSignalListEditorWidget() noexcept;

  // Setters
  void setReferences(UndoStack* undoStack, ComponentSignalList* list) noexcept;

  // Operator Overloadings
  ComponentSignalListEditorWidget& operator       =(
      const ComponentSignalListEditorWidget& rhs) = delete;

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void tableCellChanged(int row, int column) noexcept;
  // void signalRoleChanged(const SignalRole& role) noexcept;
  void isRequiredChanged(bool checked) noexcept;
  // void isNegatedChanged(bool checked) noexcept;
  // void isClockChanged(bool checked) noexcept;
  void btnAddRemoveClicked() noexcept;

private:  // Observer
  void listObjectAdded(
      const ComponentSignalList& list, int newIndex,
      const std::shared_ptr<ComponentSignal>& ptr) noexcept override;
  void listObjectRemoved(
      const ComponentSignalList& list, int oldIndex,
      const std::shared_ptr<ComponentSignal>& ptr) noexcept override;

private:  // Methods
  void updateTable() noexcept;
  void setTableRowContent(int row, const tl::optional<Uuid>& uuid,
                          const QString&                   name,
                          /*const SignalRole& role,*/ bool required,
                          /*bool negated,
bool clock,*/ const QString& forcedNetName) noexcept;
  void addSignal(
      const QString& name, /*const SignalRole& role,*/ bool required,
      /*bool negated, bool clock,*/ const QString& forcedNetName) noexcept;
  void removeSignal(const Uuid& uuid) noexcept;
  bool setName(const Uuid& uuid, const QString& name) noexcept;
  // void setRole(const Uuid& uuid, const SignalRole& role) noexcept;
  void setIsRequired(const Uuid& uuid, bool required) noexcept;
  // void setIsNegated(const Uuid& uuid, bool negated) noexcept;
  // void setIsClock(const Uuid& uuid, bool clock) noexcept;
  void setForcedNetName(const Uuid& uuid, const QString& netname) noexcept;
  int  getRowOfTableCellWidget(QObject* obj) const noexcept;
  tl::optional<Uuid> getUuidOfRow(int row) const noexcept;
  CircuitIdentifier  validateNameOrThrow(const QString& name) const;
  static QString     cleanName(const QString& name) noexcept;
  static QString     cleanForcedNetName(const QString& name) noexcept;

  // row index <-> signal index conversion methods
  int  newSignalRow() const noexcept { return mSignalList->count(); }
  int  indexToRow(int index) const noexcept { return index; }
  int  rowToIndex(int row) const noexcept { return row; }
  bool isExistingSignalRow(int row) const noexcept {
    return row >= 0 && row < mSignalList->count();
  }
  bool isNewSignalRow(int row) const noexcept { return row == newSignalRow(); }

private:  // Data
  QTableWidget*        mTable;
  UndoStack*           mUndoStack;
  ComponentSignalList* mSignalList;
  tl::optional<Uuid>   mSelectedSignal;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTEDITORWIDGET_H
