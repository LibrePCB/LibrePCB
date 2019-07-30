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

#ifndef EDITABLETABLEWIDGETRECEIVER_H
#define EDITABLETABLEWIDGETRECEIVER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  EditableTableWidgetReceiver Class
 ******************************************************************************/

class EditableTableWidgetReceiver : public QObject {
  Q_OBJECT

public:
  QVariant mAddData;
  QVariant mRemoveData;
  QVariant mCopyData;
  QVariant mEditData;
  QVariant mMoveUpData;
  QVariant mMoveDownData;
  QVariant mBrowseData;

  void btnAddClicked(const QVariant& data) noexcept { mAddData = data; }
  void btnRemoveClicked(const QVariant& data) noexcept { mRemoveData = data; }
  void btnCopyClicked(const QVariant& data) noexcept { mCopyData = data; }
  void btnEditClicked(const QVariant& data) noexcept { mEditData = data; }
  void btnMoveUpClicked(const QVariant& data) noexcept { mMoveUpData = data; }
  void btnMoveDownClicked(const QVariant& data) noexcept {
    mMoveDownData = data;
  }
  void btnBrowseClicked(const QVariant& data) noexcept { mBrowseData = data; }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

#endif  // EDITABLETABLEWIDGETRECEIVER_H
