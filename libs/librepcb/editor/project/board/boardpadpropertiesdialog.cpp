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
#include "boardpadpropertiesdialog.h"

#include "../../undostack.h"
#include "../cmd/cmdboardpadedit.h"
#include "ui_boardpadpropertiesdialog.h"

#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/circuit/netsignal.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

BoardPadPropertiesDialog::BoardPadPropertiesDialog(
    BI_Pad& pad, UndoStack& undoStack, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mPad(pad),
    mUndoStack(undoStack),
    mHoles(mPad.getProperties().getHoles()),
    mSelectedHoleIndex(-1),
    mUi(new Ui::BoardPadPropertiesDialog) {
  mUi->setupUi(this);
  mUi->edtWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                           settingsPrefix % "/width");
  mUi->edtHeight->configure(lengthUnit, LengthEditBase::Steps::generic(),
                            settingsPrefix % "/height");
  mUi->edtRadiusRatio->setSingleStep(1.0);  // [%]
  mUi->edtRadiusAbs->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/radius_abs");
  mUi->edtHoleDiameter->configure(lengthUnit,
                                  LengthEditBase::Steps::drillDiameter(),
                                  settingsPrefix % "/hole_diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtRotation->setSingleStep(90.0);  // [°]
  mUi->edtStopMaskOffset->configure(lengthUnit,
                                    LengthEditBase::Steps::generic(),
                                    settingsPrefix % "/stop_mask_offset");
  mUi->edtSolderPasteOffset->configure(lengthUnit,
                                       LengthEditBase::Steps::generic(),
                                       settingsPrefix % "/solder_paste_offset");
  mUi->edtCopperClearance->configure(lengthUnit,
                                     LengthEditBase::Steps::generic(),
                                     settingsPrefix % "/copper_clearance");
  mUi->customShapePathEditor->setLengthUnit(lengthUnit);
  mUi->holeEditorWidget->configureClientSettings(
      lengthUnit, settingsPrefix % "/hole_editor");
  connect(mUi->lblHoleDetails, &QLabel::linkActivated, this,
          [this]() { mUi->tabWidget->setCurrentWidget(mUi->tabHoles); });
  connect(mUi->btnConvertToSmt, &QToolButton::clicked, this,
          &BoardPadPropertiesDialog::removeAllHoles);
  connect(mUi->btnConvertToTht, &QToolButton::clicked, this,
          &BoardPadPropertiesDialog::addHole);
  connect(mUi->holeEditorWidget, &HoleEditorWidget::diameterChanged, this,
          [this](const PositiveLength& diameter) {
            if (const std::shared_ptr<PadHole> hole =
                    mHoles.value(mSelectedHoleIndex)) {
              hole->setDiameter(diameter);
              updateGeneralTabHoleWidgets();
            }
          });
  connect(mUi->holeEditorWidget, &HoleEditorWidget::pathChanged, this,
          [this](const NonEmptyPath& path) {
            if (const std::shared_ptr<PadHole> hole =
                    mHoles.value(mSelectedHoleIndex)) {
              hole->setPath(path);
              updateGeneralTabHoleWidgets();
            }
          });
  connect(mUi->btnPreviousHole, &QToolButton::clicked, this,
          [this]() { setSelectedHole(mSelectedHoleIndex - 1); });
  connect(mUi->btnNextHole, &QToolButton::clicked, this,
          [this]() { setSelectedHole(mSelectedHoleIndex + 1); });
  connect(mUi->btnRemoveHole, &QToolButton::clicked, this,
          &BoardPadPropertiesDialog::removeSelectedHole);
  connect(mUi->btnAddHole, &QToolButton::clicked, this,
          &BoardPadPropertiesDialog::addHole);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &BoardPadPropertiesDialog::on_buttonBox_clicked);

  // Disable some widgets if not applicable for the selected shape.
  connect(mUi->btnShapeRound, &QRadioButton::toggled, this,
          &BoardPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeRect, &QRadioButton::toggled, this,
          &BoardPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeRoundedRect, &QRadioButton::toggled, this,
          &BoardPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeOctagon, &QRadioButton::toggled, this,
          &BoardPadPropertiesDialog::updateShapeDependentWidgets);
  connect(mUi->btnShapeCustom, &QRadioButton::toggled, this,
          &BoardPadPropertiesDialog::updateShapeDependentWidgets);

  // Automatically set/clear custom shape outline to improve user experience.
  mAutoCustomOutline = mPad.getProperties().getCustomShapeOutline();
  if (mPad.getProperties().getShape() != Pad::Shape::Custom) {
    const QVector<Path> outlines =
        mPad.getProperties().getGeometry().toOutlines();
    if (!outlines.isEmpty()) {
      mAutoCustomOutline = outlines.first().toOpenPath();
    }
  }
  connect(mUi->btnShapeCustom, &QRadioButton::toggled, this,
          [this](bool custom) {
            const Path path = mUi->customShapePathEditor->getPath();
            if (custom && (path.getVertices().isEmpty())) {
              mUi->customShapePathEditor->setPath(mAutoCustomOutline);
            } else if (!custom) {
              mAutoCustomOutline = path;
              mUi->customShapePathEditor->setPath(Path());
            }
          });

  // Auto-update relative and absolute radius input widgets.
  connect(mUi->edtRadiusAbs, &UnsignedLengthEdit::valueChanged, this,
          &BoardPadPropertiesDialog::updateRelativeRadius);
  connect(mUi->edtRadiusRatio, &UnsignedLimitedRatioEdit::valueChanged, this,
          &BoardPadPropertiesDialog::updateAbsoluteRadius);
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          &BoardPadPropertiesDialog::updateAbsoluteRadius);
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          &BoardPadPropertiesDialog::updateAbsoluteRadius);

  // Avoid creating pads with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtHoleDiameter->getValue()) {
              mUi->edtHoleDiameter->setValue(value);
            }
          });
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtHoleDiameter->getValue()) {
              mUi->edtHoleDiameter->setValue(value);
            }
          });
  connect(mUi->edtHoleDiameter, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value > mUi->edtWidth->getValue()) {
              mUi->edtWidth->setValue(value);
            }
            if (value > mUi->edtHeight->getValue()) {
              mUi->edtHeight->setValue(value);
            }
            if (const std::shared_ptr<PadHole> holePtr = mHoles.value(0)) {
              holePtr->setDiameter(value);
              mUi->holeEditorWidget->setDiameter(value);
            }
          });

  // Enable custom mask offset only when allowed.
  connect(mUi->rbtnStopMaskManual, &QRadioButton::toggled,
          mUi->edtStopMaskOffset, &LengthEdit::setEnabled);
  connect(mUi->rbtnSolderPasteManual, &QRadioButton::toggled,
          mUi->edtSolderPasteOffset, &LengthEdit::setEnabled);

  // Populate functions.
  for (int i = 0; i < static_cast<int>(Pad::Function::_COUNT); ++i) {
    const Pad::Function value = static_cast<Pad::Function>(i);
    mUi->cbxFunction->addItem(Pad::getFunctionDescriptionTr(value),
                              QVariant::fromValue(value));
  }

  // load pad attributes
  const NetSignal* ns = mPad.getNetSignal();
  mUi->lblNetSignal->setText(ns ? *ns->getName() : QString());
  mUi->cbxFunction->setCurrentIndex(mUi->cbxFunction->findData(
      QVariant::fromValue(mPad.getProperties().getFunction())));
  if (mPad.getComponentSide() == Pad::ComponentSide::Bottom) {
    mUi->btnComponentSideBottom->setChecked(true);
  } else {
    mUi->btnComponentSideTop->setChecked(true);
  }
  switch (mPad.getProperties().getShape()) {
    case Pad::Shape::RoundedRect:
      mUi->btnShapeRound->setChecked(*mPad.getProperties().getRadius() ==
                                     Ratio::fromPercent(100));
      mUi->btnShapeRect->setChecked(*mPad.getProperties().getRadius() ==
                                    Ratio::fromPercent(0));
      mUi->btnShapeRoundedRect->setChecked(
          (*mPad.getProperties().getRadius() != Ratio::fromPercent(0)) &&
          (*mPad.getProperties().getRadius() != Ratio::fromPercent(100)));
      break;
    case Pad::Shape::RoundedOctagon:
      mUi->btnShapeOctagon->setChecked(true);
      break;
    case Pad::Shape::Custom:
      mUi->btnShapeCustom->setChecked(true);
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  mUi->edtRadiusRatio->setValue(mPad.getProperties().getRadius());
  mUi->edtWidth->setValue(mPad.getProperties().getWidth());
  mUi->edtHeight->setValue(mPad.getProperties().getHeight());
  mUi->edtPosX->setValue(mPad.getProperties().getPosition().getX());
  mUi->edtPosY->setValue(mPad.getProperties().getPosition().getY());
  mUi->edtRotation->setValue(mPad.getProperties().getRotation());
  mUi->cbxLock->setChecked(mPad.getProperties().isLocked());
  mUi->customShapePathEditor->setPath(
      mPad.getProperties().getCustomShapeOutline());
  if (!mPad.getProperties().getStopMaskConfig().isEnabled()) {
    mUi->rbtnStopMaskOff->setChecked(true);
  } else if (std::optional<Length> offset =
                 mPad.getProperties().getStopMaskConfig().getOffset()) {
    mUi->rbtnStopMaskManual->setChecked(true);
    mUi->edtStopMaskOffset->setValue(*offset);
  } else {
    mUi->rbtnStopMaskAuto->setChecked(true);
  }
  if (!mPad.getProperties().getSolderPasteConfig().isEnabled()) {
    mUi->rbtnSolderPasteOff->setChecked(true);
  } else if (std::optional<Length> offset =
                 mPad.getProperties().getSolderPasteConfig().getOffset()) {
    mUi->rbtnSolderPasteManual->setChecked(true);
    mUi->edtSolderPasteOffset->setValue(*offset);
  } else {
    mUi->rbtnSolderPasteAuto->setChecked(true);
  }
  mUi->edtCopperClearance->setValue(mPad.getProperties().getCopperClearance());
  updateGeneralTabHoleWidgets();
  setSelectedHole(0);

  // Auto-update radius when manually(!) modifying the size.
  connect(mUi->edtWidth, &PositiveLengthEdit::valueChanged, this,
          &BoardPadPropertiesDialog::applyRecommendedRadius);
  connect(mUi->edtHeight, &PositiveLengthEdit::valueChanged, this,
          &BoardPadPropertiesDialog::applyRecommendedRadius);

  // Always select first tab.
  mUi->tabWidget->setCurrentIndex(0);
}

BoardPadPropertiesDialog::~BoardPadPropertiesDialog() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardPadPropertiesDialog::updateShapeDependentWidgets(
    bool checked) noexcept {
  if (checked) {
    const bool round = mUi->btnShapeRound->isChecked();
    const bool roundedRect = mUi->btnShapeRoundedRect->isChecked();
    const bool octagon = mUi->btnShapeOctagon->isChecked();
    const bool custom = mUi->btnShapeCustom->isChecked();
    mUi->edtRadiusRatio->setEnabled(roundedRect || octagon);
    mUi->edtRadiusAbs->setEnabled(roundedRect || octagon);
    mUi->edtWidth->setEnabled(!custom);
    mUi->edtHeight->setEnabled(!custom);
    if (round) {
      mUi->edtRadiusRatio->setValue(
          UnsignedLimitedRatio(Ratio::fromPercent(100)));
    } else if (roundedRect) {
      applyRecommendedRadius();
    } else {
      mUi->edtRadiusRatio->setValue(
          UnsignedLimitedRatio(Ratio::fromPercent(0)));
    }
  }
}

void BoardPadPropertiesDialog::updateAbsoluteRadius() noexcept {
  QSignalBlocker blocker(mUi->edtRadiusAbs);  // Avoid endless loop.
  const UnsignedLimitedRatio ratio = mUi->edtRadiusRatio->getValue();
  const Length maxValue =
      std::min(mUi->edtWidth->getValue(), mUi->edtHeight->getValue()) / 2;
  mUi->edtRadiusAbs->setValue(UnsignedLength(
      qBound(Length(0), Length::fromMm(maxValue.toMm() * ratio->toNormalized()),
             maxValue)));
}

void BoardPadPropertiesDialog::updateRelativeRadius() noexcept {
  QSignalBlocker blocker(mUi->edtRadiusRatio);  // Avoid endless loop.
  const UnsignedLength value = mUi->edtRadiusAbs->getValue();
  const Length maxValue =
      std::min(mUi->edtWidth->getValue(), mUi->edtHeight->getValue()) / 2;
  mUi->edtRadiusRatio->setValue(UnsignedLimitedRatio(
      qBound(Ratio::fromPercent(0),
             Ratio::fromNormalized(value->toMm() / maxValue.toMm()),
             Ratio::fromPercent(100))));
}

void BoardPadPropertiesDialog::applyRecommendedRadius() noexcept {
  if (mUi->btnShapeRoundedRect->isChecked()) {
    mUi->edtRadiusRatio->setValue(Pad::getRecommendedRadius(
        mUi->edtWidth->getValue(), mUi->edtHeight->getValue()));
  }
}

void BoardPadPropertiesDialog::addHole() noexcept {
  mHoles.append(std::make_shared<PadHole>(
      Uuid::createRandom(), PositiveLength(800000), makeNonEmptyPath(Point())));
  setSelectedHole(mHoles.count() - 1);
  if (mHoles.count() == 1) {
    applyTypicalThtProperties();
  }
  updateGeneralTabHoleWidgets();
}

void BoardPadPropertiesDialog::removeSelectedHole() noexcept {
  mHoles.remove(mSelectedHoleIndex);
  setSelectedHole(mSelectedHoleIndex);
  if (mHoles.isEmpty()) {
    applyTypicalSmtProperties();
  }
  updateGeneralTabHoleWidgets();
}

void BoardPadPropertiesDialog::removeAllHoles() noexcept {
  mHoles.clear();
  setSelectedHole(0);
  applyTypicalSmtProperties();
  updateGeneralTabHoleWidgets();
}

void BoardPadPropertiesDialog::updateGeneralTabHoleWidgets() noexcept {
  if (mHoles.isEmpty()) {
    mUi->lblHoleDetails->setVisible(false);
    mUi->edtHoleDiameter->setVisible(false);
    mUi->btnConvertToSmt->setVisible(false);
    mUi->btnConvertToTht->setVisible(true);
  } else {
    mUi->btnConvertToTht->setVisible(false);
    if (mHoles.count() == 1) {
      mUi->lblHoleDetails->setVisible(false);
      mUi->edtHoleDiameter->setVisible(true);
      mUi->edtHoleDiameter->setValue(mHoles.first()->getDiameter());
    } else {
      mUi->edtHoleDiameter->setVisible(false);
      mUi->lblHoleDetails->setVisible(true);
    }
    mUi->btnConvertToSmt->setVisible(true);
  }
}

void BoardPadPropertiesDialog::setSelectedHole(int index) noexcept {
  mSelectedHoleIndex = qBound(-1, index, mHoles.count() - 1);
  const std::shared_ptr<PadHole> hole = mHoles.value(mSelectedHoleIndex);
  if (hole) {
    mUi->lblSelectedHole->setText(
        tr("Hole %1 of %2").arg(mSelectedHoleIndex + 1).arg(mHoles.count()));
    mUi->holeEditorWidget->setDiameter(hole->getDiameter());
    mUi->holeEditorWidget->setPath(hole->getPath());
  } else {
    mUi->lblSelectedHole->setText(tr("Pad has no holes"));
  }
  mUi->btnPreviousHole->setEnabled(mSelectedHoleIndex > 0);
  mUi->btnNextHole->setEnabled(mSelectedHoleIndex < (mHoles.count() - 1));
  mUi->btnRemoveHole->setEnabled(!mHoles.isEmpty());
  mUi->holeEditorWidget->setVisible(hole ? true : false);
}

void BoardPadPropertiesDialog::applyTypicalThtProperties() noexcept {
  mUi->rbtnSolderPasteOff->setChecked(true);
}

void BoardPadPropertiesDialog::applyTypicalSmtProperties() noexcept {
  mUi->rbtnSolderPasteAuto->setChecked(true);
}

void BoardPadPropertiesDialog::on_buttonBox_clicked(QAbstractButton* button) {
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

bool BoardPadPropertiesDialog::applyChanges() noexcept {
  // Clean and validate custom outline path.
  const Path customOutlinePath =
      mUi->customShapePathEditor->getPath().cleaned().toOpenPath();
  mUi->customShapePathEditor->setPath(customOutlinePath);
  if (mUi->btnShapeCustom->isChecked() &&
      (!PadGeometry::isValidCustomOutline(customOutlinePath))) {
    QMessageBox::critical(
        this, tr("Invalid outline"),
        tr("The custom pad outline does not represent a valid area."));
    return false;
  }

  try {
    std::unique_ptr<CmdBoardPadEdit> cmd(new CmdBoardPadEdit(mPad));
    QVariant function = mUi->cbxFunction->currentData();
    if (function.isValid() && function.canConvert<Pad::Function>()) {
      cmd->setFunction(function.value<Pad::Function>(), false);
    }
    if (mUi->btnComponentSideTop->isChecked()) {
      cmd->setComponentSideAndHoles(Pad::ComponentSide::Top, mHoles,
                                    false);  // can throw
    } else if (mUi->btnComponentSideBottom->isChecked()) {
      cmd->setComponentSideAndHoles(Pad::ComponentSide::Bottom, mHoles,
                                    false);  // can throw
    } else {
      Q_ASSERT(false);
    }
    if (mUi->btnShapeOctagon->isChecked()) {
      cmd->setShape(Pad::Shape::RoundedOctagon, false);
    } else if (mUi->btnShapeCustom->isChecked()) {
      cmd->setShape(Pad::Shape::Custom, false);
    } else {
      cmd->setShape(Pad::Shape::RoundedRect, false);
    }
    cmd->setRadius(mUi->edtRadiusRatio->getValue(), false);
    cmd->setWidth(mUi->edtWidth->getValue(), false);
    cmd->setHeight(mUi->edtHeight->getValue(), false);
    cmd->setCustomShapeOutline(customOutlinePath);
    if (mUi->rbtnStopMaskManual->isChecked()) {
      cmd->setStopMaskConfig(
          MaskConfig::manual(mUi->edtStopMaskOffset->getValue()), false);
    } else if (mUi->rbtnStopMaskAuto->isChecked()) {
      cmd->setStopMaskConfig(MaskConfig::automatic(), false);
    } else {
      cmd->setStopMaskConfig(MaskConfig::off(), false);
    }
    if (mUi->rbtnSolderPasteManual->isChecked()) {
      cmd->setSolderPasteConfig(
          MaskConfig::manual(mUi->edtSolderPasteOffset->getValue()));
    } else if (mUi->rbtnSolderPasteAuto->isChecked()) {
      cmd->setSolderPasteConfig(MaskConfig::automatic());
    } else {
      cmd->setSolderPasteConfig(MaskConfig::off());
    }
    cmd->setCopperClearance(mUi->edtCopperClearance->getValue(), false);
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setRotation(mUi->edtRotation->getValue(), false);
    cmd->setLocked(mUi->cbxLock->isChecked());
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
