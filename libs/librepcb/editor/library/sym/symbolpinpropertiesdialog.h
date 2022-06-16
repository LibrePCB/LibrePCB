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

#ifndef LIBREPCB_EDITOR_SYMBOLPINPROPERTIESDIALOG_H
#define LIBREPCB_EDITOR_SYMBOLPINPROPERTIESDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;
class SymbolPin;

namespace editor {

class UndoStack;

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
  SymbolPinPropertiesDialog() = delete;
  SymbolPinPropertiesDialog(const SymbolPinPropertiesDialog& other) = delete;
  SymbolPinPropertiesDialog(std::shared_ptr<SymbolPin> pin,
                            UndoStack& undoStack, const LengthUnit& lengthUnit,
                            const QString& settingsPrefix,
                            QWidget* parent = nullptr) noexcept;
  ~SymbolPinPropertiesDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  SymbolPinPropertiesDialog& operator=(const SymbolPinPropertiesDialog& rhs) =
      delete;

private:  // Methods
  void on_buttonBox_clicked(QAbstractButton* button);
  bool applyChanges() noexcept;

private:  // Data
  std::shared_ptr<SymbolPin> mSymbolPin;
  UndoStack& mUndoStack;
  QScopedPointer<Ui::SymbolPinPropertiesDialog> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
