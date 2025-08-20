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
#include "textpropertiesdialog.h"

#include "../cmd/cmdtextedit.h"
#include "../undostack.h"
#include "ui_textpropertiesdialog.h"

#include <librepcb/core/geometry/text.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

TextPropertiesDialog::TextPropertiesDialog(Text& text, UndoStack& undoStack,
                                           const QSet<const Layer*>& layers,
                                           const LengthUnit& lengthUnit,
                                           const QString& settingsPrefix,
                                           QWidget* parent) noexcept
  : QDialog(parent),
    mText(text),
    mUndoStack(undoStack),
    mUi(new Ui::TextPropertiesDialog) {
  mUi->setupUi(this);
  mUi->cbxLayer->setLayers(layers);
  mUi->edtHeight->configure(lengthUnit, LengthEditBase::Steps::textHeight(),
                            settingsPrefix % "/height");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &TextPropertiesDialog::on_buttonBox_clicked);

  // load text attributes
  mUi->cbxLayer->setCurrentLayer(mText.getLayer());
  mUi->edtText->setPlainText(mText.getText());
  mUi->alignmentSelector->setAlignment(mText.getAlign());
  mUi->edtHeight->setValue(mText.getHeight());
  mUi->edtPosX->setValue(mText.getPosition().getX());
  mUi->edtPosY->setValue(mText.getPosition().getY());
  mUi->edtRotation->setValue(mText.getRotation());
  mUi->cbxLock->setChecked(mText.isLocked());

  // set focus to text so the user can immediately start typing to change it
  mUi->edtText->selectAll();
  mUi->edtText->setFocus();
}

TextPropertiesDialog::~TextPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void TextPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->cbxLayer->setDisabled(readOnly);
  mUi->edtText->setReadOnly(readOnly);
  mUi->alignmentSelector->setReadOnly(readOnly);
  mUi->edtHeight->setReadOnly(readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  mUi->cbxLock->setDisabled(readOnly);
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

void TextPropertiesDialog::on_buttonBox_clicked(QAbstractButton* button) {
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

bool TextPropertiesDialog::applyChanges() noexcept {
  try {
    std::unique_ptr<CmdTextEdit> cmd(new CmdTextEdit(mText));
    if (auto layer = mUi->cbxLayer->getCurrentLayer()) {
      cmd->setLayer(*layer, false);
    }
    cmd->setText(mUi->edtText->toPlainText().trimmed(), false);
    cmd->setAlignment(mUi->alignmentSelector->getAlignment(), false);
    cmd->setHeight(mUi->edtHeight->getValue(), false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setRotation(mUi->edtRotation->getValue(), false);
    cmd->setLocked(mUi->cbxLock->isChecked(), false);
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
