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

#ifndef LIBREPCB_EDITOR_BOARDSETUPDIALOG_H
#define LIBREPCB_EDITOR_BOARDSETUPDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

namespace editor {

class UndoStack;

namespace Ui {
class BoardSetupDialog;
}

/*******************************************************************************
 *  Class BoardSetupDialog
 ******************************************************************************/

/**
 * @brief The BoardSetupDialog class
 */
class BoardSetupDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardSetupDialog() = delete;
  BoardSetupDialog(const BoardSetupDialog& other) = delete;
  BoardSetupDialog(Board& board, UndoStack& undoStack,
                   QWidget* parent = 0) noexcept;
  ~BoardSetupDialog();

  // General Methods
  void openDrcSettingsTab() noexcept;

  // Operator Overloadings
  BoardSetupDialog& operator=(const BoardSetupDialog& rhs) = delete;

private:  // Methods
  void buttonBoxClicked(QAbstractButton* button);
  void load() noexcept;
  bool apply() noexcept;

private:  // Date
  Board& mBoard;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::BoardSetupDialog> mUi;

  static const QString sSettingsPrefix;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
