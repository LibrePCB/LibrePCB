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

#ifndef LIBREPCB_PROJECT_BOARDVIAPROPERTIESDIALOG_H
#define LIBREPCB_PROJECT_BOARDVIAPROPERTIESDIALOG_H

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
class UndoCommand;
class LengthUnit;

namespace project {

class Project;
class BI_Via;

namespace editor {

namespace Ui {
class BoardViaPropertiesDialog;
}

/*******************************************************************************
 *  Class BoardViaPropertiesDialog
 ******************************************************************************/

/**
 * @brief The BoardViaPropertiesDialog class
 */
class BoardViaPropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardViaPropertiesDialog()                                      = delete;
  BoardViaPropertiesDialog(const BoardViaPropertiesDialog& other) = delete;
  explicit BoardViaPropertiesDialog(Project& project, BI_Via& via,
                                    UndoStack&        undoStack,
                                    const LengthUnit& lengthUnit,
                                    const QString&    settingsPrefix,
                                    QWidget*          parent) noexcept;
  ~BoardViaPropertiesDialog() noexcept;

private:
  // Private Methods
  void keyPressEvent(QKeyEvent* e);
  void accept();
  bool applyChanges() noexcept;

  // General
  Project&                                     mProject;
  BI_Via&                                      mVia;
  QScopedPointer<Ui::BoardViaPropertiesDialog> mUi;
  UndoStack&                                   mUndoStack;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARDVIAPROPERTIESDIALOG_H
