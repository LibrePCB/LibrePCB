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

#include "../font/strokefont.h"
#include "../geometry/cmd/cmdstroketextedit.h"
#include "../geometry/stroketext.h"
#include "../graphics/graphicslayer.h"
#include "../undostack.h"
#include "ui_stroketextpropertiesdialog.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

StrokeTextPropertiesDialog::StrokeTextPropertiesDialog(
    StrokeText& text, UndoStack& undoStack, QList<GraphicsLayer*> layers,
    QWidget* parent) noexcept
  : QDialog(parent),
    mText(text),
    mUndoStack(undoStack),
    mUi(new Ui::StrokeTextPropertiesDialog) {
  mUi->setupUi(this);

  foreach (const GraphicsLayer* layer, layers) {
    mUi->cbxLayer->addItem(layer->getNameTr(), layer->getName());
  }

  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &StrokeTextPropertiesDialog::on_buttonBox_clicked);
  connect(mUi->cbxLetterSpacingAuto, &QCheckBox::toggled,
          mUi->spbxLetterSpacingRatio, &QDoubleSpinBox::setDisabled);
  connect(mUi->cbxLineSpacingAuto, &QCheckBox::toggled,
          mUi->spbxLineSpacingRatio, &QDoubleSpinBox::setDisabled);

  // load text attributes
  selectLayerNameInCombobox(*mText.getLayerName());
  mUi->edtText->setPlainText(mText.getText());
  mUi->alignmentSelector->setAlignment(mText.getAlign());
  mUi->spbHeight->setValue(mText.getHeight()->toMm());
  mUi->spbxStrokeWidth->setValue(mText.getStrokeWidth()->toMm());
  if (mText.getLetterSpacing().isAuto()) {
    mUi->cbxLetterSpacingAuto->setChecked(true);
    mUi->spbxLetterSpacingRatio->setEnabled(false);
    const StrokeFont* font = text.getCurrentFont();
    Ratio ratio = font ? font->getLetterSpacing() : Ratio::percent100();
    mUi->spbxLetterSpacingRatio->setValue(ratio.toPercent());
  } else {
    mUi->cbxLetterSpacingAuto->setChecked(false);
    mUi->spbxLetterSpacingRatio->setEnabled(true);
    mUi->spbxLetterSpacingRatio->setValue(
        mText.getLetterSpacing().getRatio().toPercent());
  }
  if (mText.getLineSpacing().isAuto()) {
    mUi->cbxLineSpacingAuto->setChecked(true);
    mUi->spbxLineSpacingRatio->setEnabled(false);
    const StrokeFont* font = text.getCurrentFont();
    Ratio ratio = font ? font->getLineSpacing() : Ratio::percent100();
    mUi->spbxLineSpacingRatio->setValue(ratio.toPercent());
  } else {
    mUi->cbxLineSpacingAuto->setChecked(false);
    mUi->spbxLineSpacingRatio->setEnabled(true);
    mUi->spbxLineSpacingRatio->setValue(
        mText.getLineSpacing().getRatio().toPercent());
  }
  mUi->spbPosX->setValue(mText.getPosition().getX().toMm());
  mUi->spbPosY->setValue(mText.getPosition().getY().toMm());
  mUi->spbRotation->setValue(mText.getRotation().toDeg());
  mUi->cbxMirrored->setChecked(mText.getMirrored());
  mUi->cbxAutoRotate->setChecked(mText.getAutoRotate());
}

StrokeTextPropertiesDialog::~StrokeTextPropertiesDialog() noexcept {
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
    cmd->setStrokeWidth(
        UnsignedLength(Length::fromMm(mUi->spbxStrokeWidth->value())),
        false);  // can throw
    if (mUi->cbxLetterSpacingAuto->isChecked()) {
      cmd->setLetterSpacing(StrokeTextSpacing(), false);
    } else {
      cmd->setLetterSpacing(StrokeTextSpacing(Ratio::fromPercent(
                                mUi->spbxLetterSpacingRatio->value())),
                            false);
    }
    if (mUi->cbxLineSpacingAuto->isChecked()) {
      cmd->setLineSpacing(StrokeTextSpacing(), false);
    } else {
      cmd->setLineSpacing(StrokeTextSpacing(Ratio::fromPercent(
                              mUi->spbxLineSpacingRatio->value())),
                          false);
    }
    cmd->setHeight(PositiveLength(Length::fromMm(mUi->spbHeight->value())),
                   false);  // can throw
    cmd->setPosition(
        Point::fromMm(mUi->spbPosX->value(), mUi->spbPosY->value()), false);
    cmd->setRotation(Angle::fromDeg(mUi->spbRotation->value()), false);
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

}  // namespace librepcb
