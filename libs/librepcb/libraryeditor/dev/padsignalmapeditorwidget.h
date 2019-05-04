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
class SortFilterProxyModel;

namespace workspace {
class Workspace;
}

namespace library {

class DevicePadSignalMapModel;

namespace editor {

/*******************************************************************************
 *  Class PadSignalMapEditorWidget
 ******************************************************************************/

/**
 * @brief The PadSignalMapEditorWidget class
 */
class PadSignalMapEditorWidget final : public QWidget {
  Q_OBJECT

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

private:
  QScopedPointer<DevicePadSignalMapModel> mModel;
  QScopedPointer<SortFilterProxyModel>    mProxy;
  QScopedPointer<QTableView>              mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PADSIGNALMAPEDITORWIDGET_H
