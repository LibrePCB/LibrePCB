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

#ifndef LIBREPCB_PROJECTEDITOR_RENAMENETSEGMENTDIALOG_H
#define LIBREPCB_PROJECTEDITOR_RENAMENETSEGMENTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace project {

class NetSignal;
class SI_NetSegment;

namespace editor {

namespace Ui {
class RenameNetSegmentDialog;
}

/*******************************************************************************
 *  Class RenameNetSegmentDialog
 ******************************************************************************/

/**
 * @brief The RenameNetSegmentDialog class
 */
class RenameNetSegmentDialog final : public QDialog {
  Q_OBJECT

  enum class Action {
    NONE,
    INVALID_NAME,
    RENAME_NETSIGNAL,
    MERGE_NETSIGNALS,
    MOVE_NETSEGMENT_TO_EXISTING_NET,
    MOVE_NETSEGMENT_TO_NEW_NET,
  };

public:
  // Constructors / Destructor
  RenameNetSegmentDialog() = delete;
  RenameNetSegmentDialog(const RenameNetSegmentDialog& other) = delete;
  RenameNetSegmentDialog(UndoStack& undoStack, SI_NetSegment& segment,
                         QWidget* parent = nullptr) noexcept;
  ~RenameNetSegmentDialog() noexcept;

  // General Methods
  virtual void accept() noexcept override;

  // Operator Overloads
  RenameNetSegmentDialog& operator=(const RenameNetSegmentDialog& rhs) = delete;

private:  // Methods
  void updateAction() noexcept;

private:  // Data
  UndoStack& mUndoStack;
  SI_NetSegment& mNetSegment;
  QScopedPointer<Ui::RenameNetSegmentDialog> mUi;
  Action mAction;
  QString mNewNetName;
  NetSignal* mNewNetSignal;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
