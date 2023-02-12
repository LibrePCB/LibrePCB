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
#include "boarddesignrulecheckdialog.h"

#include "ui_boarddesignrulecheckdialog.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheckDialog::BoardDesignRuleCheckDialog(
    Board& board, const BoardDesignRuleCheck::Options& options,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QDialog(parent), mBoard(board), mUi(new Ui::BoardDesignRuleCheckDialog) {
  mUi->setupUi(this);
  mUi->prgProgress->hide();  // Somehow looks ugly as long as unused.
  mUi->edtClearanceCopperCopper->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/clearance_copper_copper");
  mUi->edtClearanceCopperBoard->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/clearance_copper_board");
  mUi->edtClearanceCopperNpth->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/clearance_copper_npth");
  mUi->edtMinCopperWidth->configure(lengthUnit,
                                    LengthEditBase::Steps::generic(),
                                    settingsPrefix % "/min_copper_width");
  mUi->edtMinPthAnnularRing->configure(
      lengthUnit, LengthEditBase::Steps::generic(),
      settingsPrefix % "/min_pth_annular_ring");
  mUi->edtMinNpthDrillDiameter->configure(
      lengthUnit, LengthEditBase::Steps::drillDiameter(),
      settingsPrefix % "/min_npth_drill_diameter");
  mUi->edtMinNpthSlotWidth->configure(lengthUnit,
                                      LengthEditBase::Steps::drillDiameter(),
                                      settingsPrefix % "/min_npth_slot_width");
  mUi->edtMinPthDrillDiameter->configure(
      lengthUnit, LengthEditBase::Steps::drillDiameter(),
      settingsPrefix % "/min_pth_drill_diameter");
  mUi->edtMinPthSlotWidth->configure(lengthUnit,
                                     LengthEditBase::Steps::drillDiameter(),
                                     settingsPrefix % "/min_pth_slot_width");
  mUi->edtCourtyardOffset->configure(lengthUnit,
                                     LengthEditBase::Steps::generic(),
                                     settingsPrefix % "/courtyard_offset");
  for (QComboBox* cbx :
       {mUi->cbxWarnNpthSlotsConfig, mUi->cbxWarnPthSlotsConfig}) {
    cbx->addItem(
        tr("Only Curved"),
        QVariant::fromValue(BoardDesignRuleCheck::SlotsWarningLevel::Curved));
    cbx->addItem(tr("Multi-Segment or Curved"),
                 QVariant::fromValue(
                     BoardDesignRuleCheck::SlotsWarningLevel::MultiSegment));
    cbx->addItem(
        tr("All"),
        QVariant::fromValue(BoardDesignRuleCheck::SlotsWarningLevel::All));
  }
  QPushButton* btnRun =
      mUi->buttonBox->addButton(tr("Run DRC"), QDialogButtonBox::AcceptRole);
  btnRun->setDefault(true);  // Allow just pressing the return key to run DRC.
  connect(btnRun, &QPushButton::clicked, this,
          &BoardDesignRuleCheckDialog::btnRunDrcClicked);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &BoardDesignRuleCheckDialog::reject);
  connect(mUi->btnSelectAll, &QPushButton::clicked, this, [this](bool checked) {
    mUi->cbxRebuildPlanes->setChecked(checked);
    mUi->cbxClearanceCopperCopper->setChecked(checked);
    mUi->cbxClearanceCopperBoard->setChecked(checked);
    mUi->cbxClearanceCopperNpth->setChecked(checked);
    mUi->cbxMinCopperWidth->setChecked(checked);
    mUi->cbxMinPthAnnularRing->setChecked(checked);
    mUi->cbxMinNpthDrillDiameter->setChecked(checked);
    mUi->cbxMinNpthSlotWidth->setChecked(checked);
    mUi->cbxMinPthDrillDiameter->setChecked(checked);
    mUi->cbxMinPthSlotWidth->setChecked(checked);
    mUi->cbxWarnNpthSlots->setChecked(checked);
    mUi->cbxWarnPthSlots->setChecked(checked);
    mUi->cbxCourtyardOffset->setChecked(checked);
    mUi->cbxBrokenPadConnections->setChecked(checked);
    mUi->cbxMissingConnections->setChecked(checked);
  });

  // set options
  mUi->cbxRebuildPlanes->setChecked(options.rebuildPlanes);
  mUi->cbxClearanceCopperCopper->setChecked(options.checkCopperCopperClearance);
  mUi->edtClearanceCopperCopper->setValue(options.minCopperCopperClearance);
  mUi->cbxClearanceCopperBoard->setChecked(options.checkCopperBoardClearance);
  mUi->edtClearanceCopperBoard->setValue(options.minCopperBoardClearance);
  mUi->cbxClearanceCopperNpth->setChecked(options.checkCopperNpthClearance);
  mUi->edtClearanceCopperNpth->setValue(options.minCopperNpthClearance);
  mUi->cbxMinCopperWidth->setChecked(options.checkCopperWidth);
  mUi->edtMinCopperWidth->setValue(options.minCopperWidth);
  mUi->cbxMinPthAnnularRing->setChecked(options.checkPthAnnularRing);
  mUi->edtMinPthAnnularRing->setValue(options.minPthAnnularRing);
  mUi->cbxMinNpthDrillDiameter->setChecked(options.checkNpthDrillDiameter);
  mUi->edtMinNpthDrillDiameter->setValue(options.minNpthDrillDiameter);
  mUi->cbxMinNpthSlotWidth->setChecked(options.checkNpthSlotWidth);
  mUi->edtMinNpthSlotWidth->setValue(options.minNpthSlotWidth);
  mUi->cbxMinPthDrillDiameter->setChecked(options.checkPthDrillDiameter);
  mUi->edtMinPthDrillDiameter->setValue(options.minPthDrillDiameter);
  mUi->cbxMinPthSlotWidth->setChecked(options.checkPthSlotWidth);
  mUi->edtMinPthSlotWidth->setValue(options.minPthSlotWidth);
  mUi->cbxWarnNpthSlots->setChecked(options.checkNpthSlotsWarning);
  mUi->cbxWarnNpthSlotsConfig->setCurrentIndex(
      mUi->cbxWarnNpthSlotsConfig->findData(
          QVariant::fromValue(options.npthSlotsWarning)));
  mUi->cbxWarnPthSlots->setChecked(options.checkPthSlotsWarning);
  mUi->cbxWarnPthSlotsConfig->setCurrentIndex(
      mUi->cbxWarnPthSlotsConfig->findData(
          QVariant::fromValue(options.pthSlotsWarning)));
  mUi->cbxCourtyardOffset->setChecked(options.checkCourtyardClearance);
  mUi->edtCourtyardOffset->setValue(options.courtyardOffset);
  mUi->cbxBrokenPadConnections->setChecked(options.checkBrokenPadConnections);
  mUi->cbxMissingConnections->setChecked(options.checkMissingConnections);

  // Load the window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("drc_dialog/window_geometry").toByteArray());
}

BoardDesignRuleCheckDialog::~BoardDesignRuleCheckDialog() {
  // Save the window geometry.
  QSettings clientSettings;
  clientSettings.setValue("drc_dialog/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

BoardDesignRuleCheck::Options BoardDesignRuleCheckDialog::getOptions() const
    noexcept {
  BoardDesignRuleCheck::Options options;
  options.rebuildPlanes = mUi->cbxRebuildPlanes->isChecked();
  options.checkCopperCopperClearance =
      mUi->cbxClearanceCopperCopper->isChecked();
  options.minCopperCopperClearance = mUi->edtClearanceCopperCopper->getValue();
  options.checkCopperBoardClearance = mUi->cbxClearanceCopperBoard->isChecked();
  options.minCopperBoardClearance = mUi->edtClearanceCopperBoard->getValue();
  options.checkCopperNpthClearance = mUi->cbxClearanceCopperNpth->isChecked();
  options.minCopperNpthClearance = mUi->edtClearanceCopperNpth->getValue();
  options.checkCopperWidth = mUi->cbxMinCopperWidth->isChecked();
  options.minCopperWidth = mUi->edtMinCopperWidth->getValue();
  options.checkPthAnnularRing = mUi->cbxMinPthAnnularRing->isChecked();
  options.minPthAnnularRing = mUi->edtMinPthAnnularRing->getValue();
  options.checkNpthDrillDiameter = mUi->cbxMinNpthDrillDiameter->isChecked();
  options.minNpthDrillDiameter = mUi->edtMinNpthDrillDiameter->getValue();
  options.checkNpthSlotWidth = mUi->cbxMinNpthSlotWidth->isChecked();
  options.minNpthSlotWidth = mUi->edtMinNpthSlotWidth->getValue();
  options.checkPthDrillDiameter = mUi->cbxMinPthDrillDiameter->isChecked();
  options.minPthDrillDiameter = mUi->edtMinPthDrillDiameter->getValue();
  options.checkPthSlotWidth = mUi->cbxMinPthSlotWidth->isChecked();
  options.minPthSlotWidth = mUi->edtMinPthSlotWidth->getValue();
  options.checkNpthSlotsWarning = mUi->cbxWarnNpthSlots->isChecked();
  options.npthSlotsWarning =
      mUi->cbxWarnNpthSlotsConfig->currentData()
          .value<BoardDesignRuleCheck::SlotsWarningLevel>();
  options.checkPthSlotsWarning = mUi->cbxWarnPthSlots->isChecked();
  options.pthSlotsWarning =
      mUi->cbxWarnPthSlotsConfig->currentData()
          .value<BoardDesignRuleCheck::SlotsWarningLevel>();
  options.checkCourtyardClearance = mUi->cbxCourtyardOffset->isChecked();
  options.courtyardOffset = mUi->edtCourtyardOffset->getValue();
  options.checkBrokenPadConnections = mUi->cbxBrokenPadConnections->isChecked();
  options.checkMissingConnections = mUi->cbxMissingConnections->isChecked();
  return options;
}

/*******************************************************************************
 *  GUI Event Handlers
 ******************************************************************************/

void BoardDesignRuleCheckDialog::btnRunDrcClicked() noexcept {
  mUi->grpOptions->setEnabled(false);
  mUi->buttonBox->setEnabled(false);
  mUi->prgProgress->show();

  try {
    mUi->lstMessages->clear();

    BoardDesignRuleCheck drc(mBoard, getOptions());
    connect(&drc, &BoardDesignRuleCheck::progressPercent, mUi->prgProgress,
            &QProgressBar::setValue);
    connect(&drc, &BoardDesignRuleCheck::progressStatus, mUi->prgProgress,
            &QProgressBar::setFormat);
    connect(&drc, &BoardDesignRuleCheck::progressMessage, mUi->lstMessages,
            static_cast<void (QListWidget::*)(const QString&)>(
                &QListWidget::addItem));

    // Use the progressStatus() signal (because it is not emitted too often
    // which would lead to flickering) to update the list widget(s).
    connect(&drc, SIGNAL(progressStatus(QString)), mUi->lstMessages,
            SLOT(repaint()));

    drc.execute();  // can throw
    mUi->prgProgress->setToolTip(drc.getProgressStatus().join("\n"));
    mMessages = drc.getMessages();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }

  mUi->grpOptions->setEnabled(true);
  mUi->buttonBox->setEnabled(true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
