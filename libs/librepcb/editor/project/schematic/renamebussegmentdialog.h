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

#ifndef LIBREPCB_EDITOR_RENAMEBUSSEGMENTDIALOG_H
#define LIBREPCB_EDITOR_RENAMEBUSSEGMENTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Bus;
class SI_BusSegment;

namespace editor {

class UndoStack;

namespace Ui {
class RenameBusSegmentDialog;
}

/*******************************************************************************
 *  Class RenameBusSegmentDialog
 ******************************************************************************/

/**
 * @brief The RenameBusSegmentDialog class
 */
class RenameBusSegmentDialog final : public QDialog {
  Q_OBJECT

  enum class Action {
    NONE,
    INVALID_NAME,
    RENAME_BUS,
    MERGE_BUSES,
    MOVE_SEGMENT_TO_EXISTING_BUS,
    MOVE_SEGMENT_TO_NEW_BUS,
  };

public:
  // Constructors / Destructor
  RenameBusSegmentDialog() = delete;
  RenameBusSegmentDialog(const RenameBusSegmentDialog& other) = delete;
  RenameBusSegmentDialog(UndoStack& undoStack, SI_BusSegment& segment,
                         QWidget* parent = nullptr) noexcept;
  ~RenameBusSegmentDialog() noexcept;

  // General Methods
  virtual void accept() noexcept override;

  // Operator Overloads
  RenameBusSegmentDialog& operator=(const RenameBusSegmentDialog& rhs) = delete;

private:  // Methods
  void updateAction() noexcept;

private:  // Data
  UndoStack& mUndoStack;
  SI_BusSegment& mSegment;
  QScopedPointer<Ui::RenameBusSegmentDialog> mUi;
  Action mAction;
  QString mNewBusName;
  Bus* mNewBus;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
