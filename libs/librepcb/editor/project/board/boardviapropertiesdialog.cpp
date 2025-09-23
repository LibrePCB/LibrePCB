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
#include "boardviapropertiesdialog.h"

#include "../../project/cmd/cmdboardviaedit.h"
#include "../../undostack.h"
#include "ui_boardviapropertiesdialog.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boarddesignrules.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/netsignal.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardViaPropertiesDialog::BoardViaPropertiesDialog(
    Project& project, BI_Via& via, UndoStack& undoStack,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent),
    mProject(project),
    mVia(via),
    mUi(new Ui::BoardViaPropertiesDialog),
    mUndoStack(undoStack) {
  mUi->setupUi(this);
  mUi->edtSize->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/size");
  mUi->edtDrillDiameter->configure(lengthUnit,
                                   LengthEditBase::Steps::drillDiameter(),
                                   settingsPrefix % "/drill_diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtExposureOffset->configure(lengthUnit,
                                    LengthEditBase::Steps::generic(),
                                    settingsPrefix % "/exposure_offset");
  QSet<const Layer*> layers = via.getBoard().getCopperLayers();
  layers.insert(&via.getVia().getStartLayer());
  layers.insert(&via.getVia().getEndLayer());
  mUi->cbxStartLayer->setLayers(layers);
  mUi->cbxEndLayer->setLayers(layers);
  connect(mUi->rbtnExposureManual, &QRadioButton::toggled,
          mUi->edtExposureOffset, &LengthEdit::setEnabled);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &BoardViaPropertiesDialog::buttonBoxClicked);

  // Helper to apply the calculated size to the size spinbox.
  auto applySizeFromDesignRules = [this]() {
    mUi->edtSize->setValue(Via::calcSizeFromRules(
        mUi->edtDrillDiameter->getValue(),
        mVia.getBoard().getDesignRules().getViaAnnularRing()));
  };

  // Set up automatic/manual size toggle switch.
  connect(mUi->cbxSizeFromDesignRules, &QCheckBox::toggled, this,
          [this, applySizeFromDesignRules](bool checked) {
            mUi->edtSize->setEnabled(!checked);
            if (checked) {
              applySizeFromDesignRules();
            }
          });

  // Avoid creating vias with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  connect(mUi->edtSize, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            if (value < mUi->edtDrillDiameter->getValue()) {
              mUi->edtDrillDiameter->setValue(value);
            }
          });
  connect(mUi->edtDrillDiameter, &PositiveLengthEdit::valueChanged, this,
          [this, applySizeFromDesignRules](const PositiveLength& value) {
            if (mUi->cbxSizeFromDesignRules->isChecked()) {
              applySizeFromDesignRules();
            } else if (value > mUi->edtSize->getValue()) {
              mUi->edtSize->setValue(value);
            }
          });

  // netsignal name
  mUi->lblNetSignal->setText(mVia.getNetSegment().getNetNameToDisplay(true));

  // Position spinboxes
  mUi->edtPosX->setValue(mVia.getPosition().getX());
  mUi->edtPosY->setValue(mVia.getPosition().getY());

  // drill diameter spinbox
  mUi->edtDrillDiameter->setValue(mVia.getDrillDiameter());

  // size spinbox / checkbox
  mUi->cbxSizeFromDesignRules->setChecked(!mVia.getSize());
  if (const auto& size = mVia.getSize()) {
    mUi->edtSize->setValue(*size);
  } else {
    applySizeFromDesignRules();
  }

  // Layers.
  mUi->cbxStartLayer->setCurrentLayer(via.getVia().getStartLayer());
  mUi->cbxEndLayer->setCurrentLayer(via.getVia().getEndLayer());

  // Stop mask.
  if (!mVia.getVia().getExposureConfig().isEnabled()) {
    mUi->rbtnExposureOff->setChecked(true);
  } else if (const auto& offset =
                 mVia.getVia().getExposureConfig().getOffset()) {
    mUi->rbtnExposureManual->setChecked(true);
    mUi->edtExposureOffset->setValue(*offset);
  } else {
    mUi->rbtnExposureAuto->setChecked(true);
  }
}

BoardViaPropertiesDialog::~BoardViaPropertiesDialog() noexcept {
  mUi.reset();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardViaPropertiesDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
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

void BoardViaPropertiesDialog::accept() {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool BoardViaPropertiesDialog::applyChanges() noexcept {
  try {
    std::unique_ptr<CmdBoardViaEdit> cmd(new CmdBoardViaEdit(mVia));
    cmd->setPosition(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()),
                     false);
    cmd->setDrillAndSize(mUi->edtDrillDiameter->getValue(),
                         mUi->cbxSizeFromDesignRules->isChecked()
                             ? std::nullopt
                             : std::make_optional(mUi->edtSize->getValue()),
                         false);  // can throw
    const Layer* startLayer = mUi->cbxStartLayer->getCurrentLayer();
    const Layer* endLayer = mUi->cbxEndLayer->getCurrentLayer();
    cmd->setLayers(startLayer ? *startLayer : mVia.getVia().getStartLayer(),
                   endLayer ? *endLayer : mVia.getVia().getEndLayer());
    if (mUi->rbtnExposureOff->isChecked()) {
      cmd->setExposureConfig(MaskConfig::off());
    } else if (mUi->rbtnExposureAuto->isChecked()) {
      cmd->setExposureConfig(MaskConfig::automatic());
    } else if (mUi->rbtnExposureManual->isChecked()) {
      cmd->setExposureConfig(
          MaskConfig::manual(mUi->edtExposureOffset->getValue()));
    } else {
      qCritical() << "Unknown UI configuration for via stop mask.";
    }
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
