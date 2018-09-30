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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLPINPROPERTIESDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLPINPROPERTIESDIALOG_H

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

namespace library {

class SymbolPin;

namespace editor {

namespace Ui {
class SymbolPinPropertiesDialog;
}

/*******************************************************************************
 *  Class SymbolPinPropertiesDialog
 ******************************************************************************/

/**
 * @brief The SymbolPinPropertiesDialog class
 */
class SymbolPinPropertiesDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolPinPropertiesDialog()                                       = delete;
  SymbolPinPropertiesDialog(const SymbolPinPropertiesDialog& other) = delete;
  SymbolPinPropertiesDialog(SymbolPin& pin, UndoStack& undoStack,
                            QWidget* parent = nullptr) noexcept;
  ~SymbolPinPropertiesDialog() noexcept;

  // Operator Overloadings
  SymbolPinPropertiesDialog& operator=(const SymbolPinPropertiesDialog& rhs) =
      delete;

private:  // Methods
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;

private:  // Data
  SymbolPin&                                    mSymbolPin;
  UndoStack&                                    mUndoStack;
  QScopedPointer<Ui::SymbolPinPropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLPINPROPERTIESDIALOG_H
