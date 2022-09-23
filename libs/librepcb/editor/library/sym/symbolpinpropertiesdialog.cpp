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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symbolpinpropertiesdialog.h"

#include "../../undostack.h"
#include "../cmd/cmdsymbolpinedit.h"
#include "ui_symbolpinpropertiesdialog.h"

#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

SymbolPinPropertiesDialog::SymbolPinPropertiesDialog(
    SymbolPin& pin, UndoStack& undoStack, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mSymbolPin(pin),
    mUndoStack(undoStack),
    mUi(new Ui::SymbolPinPropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtLength->configure(lengthUnit, LengthEditBase::Steps::pinLength(),
                            settingsPrefix % "/length");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &SymbolPinPropertiesDialog::on_buttonBox_clicked);

  // load pin attributes
  mUi->edtName->setText(*mSymbolPin.getName());
  mUi->edtPosX->setValue(mSymbolPin.getPosition().getX());
  mUi->edtPosY->setValue(mSymbolPin.getPosition().getY());
  mUi->edtRotation->setValue(mSymbolPin.getRotation());
  mUi->edtLength->setValue(mSymbolPin.getLength());

  // preselect name
  mUi->edtName->selectAll();
}

SymbolPinPropertiesDialog::~SymbolPinPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SymbolPinPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->edtName->setReadOnly(readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  mUi->edtLength->setReadOnly(readOnly);
  if (readOnly) {
    mUi->buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Close);
  } else {
    mUi->buttonBox->setStandardButtons(
        QDialogButtonBox::StandardButton::Apply |
        QDialogButtonBox::StandardButton::Cancel |
        QDialogButtonBox::StandardButton::Ok);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolPinPropertiesDialog::on_buttonBox_clicked(QAbstractButton* button) {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      applyChanges();
      break;
    case QDialogButtonBox::AcceptRole:
      if (applyChanges()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

bool SymbolPinPropertiesDialog::applyChanges() noexcept {
  try {
    CircuitIdentifier name(mUi->edtName->text().trimmed());  // can throw
    QScopedPointer<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(mSymbolPin));
    cmd->setName(name, false);
    cmd->setLength(mUi->edtLength->getValue(), false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setRotation(mUi->edtRotation->getValue(), false);
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
