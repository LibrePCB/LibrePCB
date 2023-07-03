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

#ifndef UNITTESTS_EDITOR_EDITABLETABLEWIDGETRECEIVER_H
#define UNITTESTS_EDITOR_EDITABLETABLEWIDGETRECEIVER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  EditableTableWidgetReceiver Class
 ******************************************************************************/

class EditableTableWidgetReceiver : public QObject {
  Q_OBJECT

public:
  QPersistentModelIndex mAddIndex;
  QPersistentModelIndex mRemoveIndex;
  QPersistentModelIndex mCopyIndex;
  QPersistentModelIndex mEditIndex;
  QPersistentModelIndex mMoveUpIndex;
  QPersistentModelIndex mMoveDownIndex;
  QPersistentModelIndex mBrowseIndex;

  void btnAddClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mAddIndex = itemIndex;
  }
  void btnRemoveClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mRemoveIndex = itemIndex;
  }
  void btnCopyClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mCopyIndex = itemIndex;
  }
  void btnEditClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mEditIndex = itemIndex;
  }
  void btnMoveUpClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mMoveUpIndex = itemIndex;
  }
  void btnMoveDownClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mMoveDownIndex = itemIndex;
  }
  void btnBrowseClicked(const QPersistentModelIndex& itemIndex) noexcept {
    mBrowseIndex = itemIndex;
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb

#endif
