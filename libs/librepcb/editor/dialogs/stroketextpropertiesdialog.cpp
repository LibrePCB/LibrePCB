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
#include "stroketextpropertiesdialog.h"

#include "../cmd/cmdstroketextedit.h"
#include "../undostack.h"
#include "ui_stroketextpropertiesdialog.h"

#include <librepcb/core/font/strokefont.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/graphics/graphicslayer.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

StrokeTextPropertiesDialog::StrokeTextPropertiesDialog(
    StrokeText& text, UndoStack& undoStack, QList<GraphicsLayer*> layers,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent),
    mText(text),
    mUndoStack(undoStack),
    mUi(new Ui::StrokeTextPropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtHeight->configure(lengthUnit, LengthEditBase::Steps::textHeight(),
                            settingsPrefix % "/height");
  mUi->edtStrokeWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                 settingsPrefix % "/stroke_width");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]

  foreach (const GraphicsLayer* layer, layers) {
    mUi->cbxLayer->addItem(layer->getNameTr(), layer->getName());
  }

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &StrokeTextPropertiesDialog::on_buttonBox_clicked);
  connect(mUi->cbxLetterSpacingAuto, &QCheckBox::toggled,
          mUi->edtLetterSpacingRatio, &RatioEdit::setDisabled);
  connect(mUi->cbxLineSpacingAuto, &QCheckBox::toggled,
          mUi->edtLineSpacingRatio, &RatioEdit::setDisabled);

  // load text attributes
  selectLayerNameInCombobox(*mText.getLayerName());
  mUi->edtText->setPlainText(mText.getText());
  mUi->alignmentSelector->setAlignment(mText.getAlign());
  mUi->edtHeight->setValue(mText.getHeight());
  mUi->edtStrokeWidth->setValue(mText.getStrokeWidth());
  if (mText.getLetterSpacing().isAuto()) {
    mUi->cbxLetterSpacingAuto->setChecked(true);
    mUi->edtLetterSpacingRatio->setEnabled(false);
    const StrokeFont* font = text.getCurrentFont();
    Ratio ratio = font ? font->getLetterSpacing() : Ratio::percent100();
    mUi->edtLetterSpacingRatio->setValue(ratio);
  } else {
    mUi->cbxLetterSpacingAuto->setChecked(false);
    mUi->edtLetterSpacingRatio->setEnabled(true);
    mUi->edtLetterSpacingRatio->setValue(mText.getLetterSpacing().getRatio());
  }
  if (mText.getLineSpacing().isAuto()) {
    mUi->cbxLineSpacingAuto->setChecked(true);
    mUi->edtLineSpacingRatio->setEnabled(false);
    const StrokeFont* font = text.getCurrentFont();
    Ratio ratio = font ? font->getLineSpacing() : Ratio::percent100();
    mUi->edtLineSpacingRatio->setValue(ratio);
  } else {
    mUi->cbxLineSpacingAuto->setChecked(false);
    mUi->edtLineSpacingRatio->setEnabled(true);
    mUi->edtLineSpacingRatio->setValue(mText.getLineSpacing().getRatio());
  }
  mUi->edtPosX->setValue(mText.getPosition().getX());
  mUi->edtPosY->setValue(mText.getPosition().getY());
  mUi->edtRotation->setValue(mText.getRotation());
  mUi->cbxMirrored->setChecked(mText.getMirrored());
  mUi->cbxAutoRotate->setChecked(mText.getAutoRotate());

  // set focus to text so the user can immediately start typing to change it
  mUi->edtText->selectAll();
  mUi->edtText->setFocus();
}

StrokeTextPropertiesDialog::~StrokeTextPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void StrokeTextPropertiesDialog::setReadOnly(bool readOnly) noexcept {
  mUi->edtText->setReadOnly(readOnly);
  mUi->cbxLayer->setDisabled(readOnly);
  mUi->alignmentSelector->setReadOnly(readOnly);
  mUi->edtHeight->setReadOnly(readOnly);
  mUi->edtStrokeWidth->setReadOnly(readOnly);
  mUi->edtLetterSpacingRatio->setReadOnly(readOnly);
  mUi->cbxLetterSpacingAuto->setCheckable(!readOnly);
  mUi->edtLineSpacingRatio->setReadOnly(readOnly);
  mUi->cbxLineSpacingAuto->setCheckable(!readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  mUi->cbxMirrored->setCheckable(!readOnly);
  mUi->cbxAutoRotate->setCheckable(!readOnly);
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

void StrokeTextPropertiesDialog::on_buttonBox_clicked(QAbstractButton* button) {
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

bool StrokeTextPropertiesDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(mText));
    if (mUi->cbxLayer->currentIndex() >= 0 &&
        mUi->cbxLayer->currentData().isValid()) {
      cmd->setLayerName(
          GraphicsLayerName(mUi->cbxLayer->currentData().toString()),
          false);  // can throw
    }
    cmd->setText(mUi->edtText->toPlainText(), false);
    cmd->setAlignment(mUi->alignmentSelector->getAlignment(), false);
    cmd->setStrokeWidth(mUi->edtStrokeWidth->getValue(), false);
    if (mUi->cbxLetterSpacingAuto->isChecked()) {
      cmd->setLetterSpacing(StrokeTextSpacing(), false);
    } else {
      cmd->setLetterSpacing(
          StrokeTextSpacing(mUi->edtLetterSpacingRatio->getValue()), false);
    }
    if (mUi->cbxLineSpacingAuto->isChecked()) {
      cmd->setLineSpacing(StrokeTextSpacing(), false);
    } else {
      cmd->setLineSpacing(
          StrokeTextSpacing(mUi->edtLineSpacingRatio->getValue()), false);
    }
    cmd->setHeight(mUi->edtHeight->getValue(), false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setRotation(mUi->edtRotation->getValue(), false);
    cmd->setMirrored(mUi->cbxMirrored->isChecked(), false);
    cmd->setAutoRotate(mUi->cbxAutoRotate->isChecked(), false);
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

void StrokeTextPropertiesDialog::selectLayerNameInCombobox(
    const QString& name) noexcept {
  mUi->cbxLayer->setCurrentIndex(mUi->cbxLayer->findData(name));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
