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

#ifndef LIBREPCB_EDITOR_EDITNETCLASSESDIALOG_H
#define LIBREPCB_EDITOR_EDITNETCLASSESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;

namespace editor {

class UndoStack;

namespace Ui {
class EditNetClassesDialog;
}

/*******************************************************************************
 *  Class EditNetclassesDialog
 ******************************************************************************/

/**
 * @brief The EditNetClassesDialog class
 */
class EditNetClassesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  EditNetClassesDialog() = delete;
  EditNetClassesDialog(const EditNetClassesDialog& other) = delete;
  explicit EditNetClassesDialog(Circuit& circuit, UndoStack& undoStack,
                                QWidget* parent = 0);
  ~EditNetClassesDialog() noexcept;

  // Operator Overloadings
  EditNetClassesDialog& operator=(const EditNetClassesDialog& rhs) = delete;

private slots:

  void on_tableWidget_itemChanged(QTableWidgetItem* item);
  void on_btnAdd_clicked();
  void on_btnRemove_clicked();

private:
  // General Attributes
  Circuit& mCircuit;
  Ui::EditNetClassesDialog* mUi;
  UndoStack& mUndoStack;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
