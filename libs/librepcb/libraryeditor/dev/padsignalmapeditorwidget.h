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

#ifndef LIBREPCB_LIBRARY_EDITOR_PADSIGNALMAPEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_PADSIGNALMAPEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/componentsignal.h>
#include <librepcb/library/dev/devicepadsignalmap.h>
#include <librepcb/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace workspace {
class Workspace;
}

namespace library {
namespace editor {

/*******************************************************************************
 *  Class PadSignalMapEditorWidget
 ******************************************************************************/

/**
 * @brief The PadSignalMapEditorWidget class
 *
 * @author ubruhin
 * @date 2017-03-25
 */
class PadSignalMapEditorWidget final : public QWidget,
                                       private DevicePadSignalMap::IF_Observer {
  Q_OBJECT

private:  // Types
  enum Column { COLUMN_PAD = 0, COLUMN_SIGNAL, _COLUMN_COUNT };

public:
  // Constructors / Destructor
  explicit PadSignalMapEditorWidget(QWidget* parent = nullptr) noexcept;
  PadSignalMapEditorWidget(const PadSignalMapEditorWidget& other) = delete;
  ~PadSignalMapEditorWidget() noexcept;

  // General Methods
  void setReferences(UndoStack* undoStack, DevicePadSignalMap* map) noexcept;
  void setPadList(const PackagePadList& list) noexcept;
  void setSignalList(const ComponentSignalList& list) noexcept;

  // Operator Overloadings
  PadSignalMapEditorWidget& operator=(const PadSignalMapEditorWidget& rhs) =
      delete;

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void componentSignalChanged(int index) noexcept;

private:  // Observer
  void listObjectAdded(
      const DevicePadSignalMap& list, int newIndex,
      const std::shared_ptr<DevicePadSignalMapItem>& ptr) noexcept override;
  void listObjectRemoved(
      const DevicePadSignalMap& list, int oldIndex,
      const std::shared_ptr<DevicePadSignalMapItem>& ptr) noexcept override;

private:  // Methods
  void updateTable() noexcept;
  void setTableRowContent(int row, const Uuid& padUuid, const QString& padName,
                          const tl::optional<Uuid>& signalUuid) noexcept;
  void setComponentSignal(const Uuid&               pad,
                          const tl::optional<Uuid>& signal) noexcept;
  tl::optional<Uuid> getPadUuidOfTableCellWidget(QObject* obj) const noexcept;
  tl::optional<Uuid> getPadUuidOfRow(int row) const noexcept;

private:  // Data
  QTableWidget*       mTable;
  UndoStack*          mUndoStack;
  DevicePadSignalMap* mPadSignalMap;
  PackagePadList      mPads;
  ComponentSignalList mSignals;
  tl::optional<Uuid>  mSelectedPad;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PADSIGNALMAPEDITORWIDGET_H
