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
#include "../project/cmd/cmdboardstroketextedit.h"
#include "../undostack.h"
#include "ui_stroketextpropertiesdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/font/strokefont.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

StrokeTextPropertiesDialog::StrokeTextPropertiesDialog(
    StrokeText* libObj, BI_StrokeText* boardObj, UndoStack& undoStack,
    const QSet<const Layer*>& layers, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mLibraryObj(libObj),
    mBoardObj(boardObj),
    mUndoStack(undoStack),
    mUi(new Ui::StrokeTextPropertiesDialog) {
  mUi->setupUi(this);
  mUi->cbxLayer->setLayers(layers);
  mUi->edtHeight->configure(lengthUnit, LengthEditBase::Steps::textHeight(),
                            settingsPrefix % "/height");
  mUi->edtStrokeWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                 settingsPrefix % "/stroke_width");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &StrokeTextPropertiesDialog::on_buttonBox_clicked);
  connect(mUi->cbxLetterSpacingAuto, &QCheckBox::toggled,
          mUi->edtLetterSpacingRatio, &RatioEdit::setDisabled);
  connect(mUi->cbxLineSpacingAuto, &QCheckBox::toggled,
          mUi->edtLineSpacingRatio, &RatioEdit::setDisabled);
}

StrokeTextPropertiesDialog::StrokeTextPropertiesDialog(
    StrokeText& text, UndoStack& undoStack, const QSet<const Layer*>& layers,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : StrokeTextPropertiesDialog(&text, nullptr, undoStack, layers, lengthUnit,
                               settingsPrefix, parent) {
  load(text, Application::getDefaultStrokeFont());
}

StrokeTextPropertiesDialog::StrokeTextPropertiesDialog(
    BI_StrokeText& text, UndoStack& undoStack, const QSet<const Layer*>& layers,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : StrokeTextPropertiesDialog(nullptr, &text, undoStack, layers, lengthUnit,
                               settingsPrefix, parent) {
  load(text.getData(), text.getFont());
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
  mUi->cbxLetterSpacingAuto->setEnabled(!readOnly);
  mUi->edtLineSpacingRatio->setReadOnly(readOnly);
  mUi->cbxLineSpacingAuto->setEnabled(!readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  mUi->cbxAutoRotate->setEnabled(!readOnly);
  mUi->cbxMirrored->setEnabled(!readOnly);
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

template <typename T>
void StrokeTextPropertiesDialog::load(const T& obj,
                                      const StrokeFont& font) noexcept {
  mUi->cbxLayer->setCurrentLayer(obj.getLayer());
  mUi->edtText->setPlainText(obj.getText());
  mUi->alignmentSelector->setAlignment(obj.getAlign());
  mUi->edtHeight->setValue(obj.getHeight());
  mUi->edtStrokeWidth->setValue(obj.getStrokeWidth());
  if (const tl::optional<Ratio>& ratio = obj.getLetterSpacing().getRatio()) {
    mUi->cbxLetterSpacingAuto->setChecked(false);
    mUi->edtLetterSpacingRatio->setEnabled(true);
    mUi->edtLetterSpacingRatio->setValue(*ratio);
  } else {
    mUi->cbxLetterSpacingAuto->setChecked(true);
    mUi->edtLetterSpacingRatio->setEnabled(false);
    mUi->edtLetterSpacingRatio->setValue(font.getLetterSpacing());
  }
  if (const tl::optional<Ratio>& ratio = obj.getLineSpacing().getRatio()) {
    mUi->cbxLineSpacingAuto->setChecked(false);
    mUi->edtLineSpacingRatio->setEnabled(true);
    mUi->edtLineSpacingRatio->setValue(*ratio);
  } else {
    mUi->cbxLineSpacingAuto->setChecked(true);
    mUi->edtLineSpacingRatio->setEnabled(false);
    mUi->edtLineSpacingRatio->setValue(font.getLineSpacing());
  }
  mUi->edtPosX->setValue(obj.getPosition().getX());
  mUi->edtPosY->setValue(obj.getPosition().getY());
  mUi->edtRotation->setValue(obj.getRotation());
  mUi->cbxMirrored->setChecked(obj.getMirrored());
  mUi->cbxAutoRotate->setChecked(obj.getAutoRotate());

  // set focus to text so the user can immediately start typing to change it
  mUi->edtText->selectAll();
  mUi->edtText->setFocus();
}

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
    if (mLibraryObj) {
      QScopedPointer<CmdStrokeTextEdit> cmd(
          new CmdStrokeTextEdit(*mLibraryObj));
      applyChanges(*cmd);
      mUndoStack.execCmd(cmd.take());  // can throw
    }
    if (mBoardObj) {
      QScopedPointer<CmdBoardStrokeTextEdit> cmd(
          new CmdBoardStrokeTextEdit(*mBoardObj));
      applyChanges(*cmd);
      mUndoStack.execCmd(cmd.take());  // can throw
    }
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

template <typename T>
void StrokeTextPropertiesDialog::applyChanges(T& cmd) {
  if (auto layer = mUi->cbxLayer->getCurrentLayer()) {
    cmd.setLayer(*layer, false);
  }
  cmd.setText(mUi->edtText->toPlainText(), false);
  cmd.setAlignment(mUi->alignmentSelector->getAlignment(), false);
  cmd.setStrokeWidth(mUi->edtStrokeWidth->getValue(), false);
  if (mUi->cbxLetterSpacingAuto->isChecked()) {
    cmd.setLetterSpacing(StrokeTextSpacing(), false);
  } else {
    cmd.setLetterSpacing(
        StrokeTextSpacing(mUi->edtLetterSpacingRatio->getValue()), false);
  }
  if (mUi->cbxLineSpacingAuto->isChecked()) {
    cmd.setLineSpacing(StrokeTextSpacing(), false);
  } else {
    cmd.setLineSpacing(StrokeTextSpacing(mUi->edtLineSpacingRatio->getValue()),
                       false);
  }
  cmd.setHeight(mUi->edtHeight->getValue(), false);
  cmd.setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                  false);
  cmd.setRotation(mUi->edtRotation->getValue(), false);
  cmd.setMirrored(mUi->cbxMirrored->isChecked(), false);
  cmd.setAutoRotate(mUi->cbxAutoRotate->isChecked(), false);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
