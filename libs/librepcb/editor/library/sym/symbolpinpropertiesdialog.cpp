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
    std::shared_ptr<SymbolPin> pin, UndoStack& undoStack,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent),
    mSymbolPin(pin),
    mUndoStack(undoStack),
    mUi(new Ui::SymbolPinPropertiesDialog) {
  Q_ASSERT(mSymbolPin);
  mUi->setupUi(this);
  mUi->edtName->setMaxLength(CircuitIdentifierConstraint::MAX_LENGTH);
  mUi->edtLength->configure(lengthUnit, LengthEditBase::Steps::pinLength(),
                            settingsPrefix % "/length");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]
  mUi->edtNameHeight->configure(lengthUnit, LengthEditBase::Steps::textHeight(),
                                settingsPrefix % "/name_height");
  mUi->edtNameHeight->setDefaultValueToolTip(
      *SymbolPin::getDefaultNameHeight());
  mUi->edtNamePosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                              settingsPrefix % "/name_pos_x");
  mUi->edtNamePosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                              settingsPrefix % "/name_pos_y");
  updateNamePositionTooltip(mSymbolPin->getLength());
  mUi->edtNameRotation->setSingleStep(90.0);  // [°]
  mUi->lblNameAlignment->setText(mUi->lblNameAlignment->text() % "<br/><i>" %
                                 tr("(at 0° rotation)") % "</i>");
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &SymbolPinPropertiesDialog::on_buttonBox_clicked);

  // load pin attributes
  mUi->edtName->setText(*mSymbolPin->getName());
  mUi->edtPosX->setValue(mSymbolPin->getPosition().getX());
  mUi->edtPosY->setValue(mSymbolPin->getPosition().getY());
  mUi->edtRotation->setValue(mSymbolPin->getRotation());
  mUi->edtLength->setValue(mSymbolPin->getLength());
  mUi->edtNameHeight->setValue(mSymbolPin->getNameHeight());
  mUi->edtNamePosX->setValue(mSymbolPin->getNamePosition().getX());
  mUi->edtNamePosY->setValue(mSymbolPin->getNamePosition().getY());
  mUi->edtNameRotation->setValue(mSymbolPin->getNameRotation());
  mUi->edtNameAlignment->setAlignment(mSymbolPin->getNameAlignment());

  // Setup auto-move text checkbox. Check it if the text is on the right side
  // of the pin.
  mUi->cbxAutoMoveText->setChecked(
      (mSymbolPin->getNamePosition().getX() >= *mSymbolPin->getLength()));
  connect(mUi->edtLength, &UnsignedLengthEdit::valueChanged, this,
          [this](const UnsignedLength& length, const Length& diff) {
            if (mUi->cbxAutoMoveText->isChecked()) {
              mUi->edtNamePosX->setValue(mUi->edtNamePosX->getValue() + diff);
            }
            updateNamePositionTooltip(length);
          });

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
  mUi->cbxAutoMoveText->setEnabled(!readOnly);
  mUi->edtNameHeight->setReadOnly(readOnly);
  mUi->edtNamePosX->setReadOnly(readOnly);
  mUi->edtNamePosY->setReadOnly(readOnly);
  mUi->edtNameRotation->setReadOnly(readOnly);
  mUi->edtNameAlignment->setReadOnly(readOnly);
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

void SymbolPinPropertiesDialog::updateNamePositionTooltip(
    const UnsignedLength& length) noexcept {
  const Point pos = SymbolPin::getDefaultNamePosition(length);
  mUi->edtNamePosX->setDefaultValueToolTip(pos.getX());
  mUi->edtNamePosY->setDefaultValueToolTip(pos.getY());
}

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
    std::unique_ptr<CmdSymbolPinEdit> cmd(new CmdSymbolPinEdit(mSymbolPin));
    cmd->setName(name, false);
    cmd->setLength(mUi->edtLength->getValue(), false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setRotation(mUi->edtRotation->getValue(), false);
    cmd->setNameHeight(mUi->edtNameHeight->getValue(), false);
    cmd->setNamePosition(
        Point(mUi->edtNamePosX->getValue(), mUi->edtNamePosY->getValue()),
        false);
    cmd->setNameRotation(mUi->edtNameRotation->getValue(), false);
    cmd->setNameAlignment(mUi->edtNameAlignment->getAlignment(), false);
    mUndoStack.execCmd(cmd.release());
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
