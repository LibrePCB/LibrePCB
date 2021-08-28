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

#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/drc/boarddesignrulecheck.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
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
  mUi->edtMinPthRestring->configure(lengthUnit,
                                    LengthEditBase::Steps::generic(),
                                    settingsPrefix % "/min_pth_restring");
  mUi->edtMinPthDrillDiameter->configure(
      lengthUnit, LengthEditBase::Steps::drillDiameter(),
      settingsPrefix % "/min_pth_drill_diameter");
  mUi->edtMinNpthDrillDiameter->configure(
      lengthUnit, LengthEditBase::Steps::drillDiameter(),
      settingsPrefix % "/min_npth_drill_diameter");
  mUi->edtCourtyardOffset->configure(lengthUnit,
                                     LengthEditBase::Steps::generic(),
                                     settingsPrefix % "/courtyard_offset");
  QPushButton* btnRun =
      mUi->buttonBox->addButton(tr("Run DRC"), QDialogButtonBox::ActionRole);
  btnRun->setDefault(true);  // Allow just pressing the return key to run DRC.
  connect(btnRun, &QPushButton::clicked, this,
          &BoardDesignRuleCheckDialog::btnRunDrcClicked);

  // set options
  mUi->edtClearanceCopperCopper->setValue(options.minCopperCopperClearance);
  mUi->edtClearanceCopperBoard->setValue(options.minCopperBoardClearance);
  mUi->edtClearanceCopperNpth->setValue(options.minCopperNpthClearance);
  mUi->edtMinCopperWidth->setValue(options.minCopperWidth);
  mUi->edtMinPthRestring->setValue(options.minPthRestring);
  mUi->edtMinPthDrillDiameter->setValue(options.minPthDrillDiameter);
  mUi->edtMinNpthDrillDiameter->setValue(options.minNpthDrillDiameter);
  mUi->edtCourtyardOffset->setValue(options.courtyardOffset);
}

BoardDesignRuleCheckDialog::~BoardDesignRuleCheckDialog() {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

BoardDesignRuleCheck::Options BoardDesignRuleCheckDialog::getOptions() const
    noexcept {
  BoardDesignRuleCheck::Options options;
  options.minCopperCopperClearance = mUi->edtClearanceCopperCopper->getValue();
  options.minCopperBoardClearance = mUi->edtClearanceCopperBoard->getValue();
  options.minCopperNpthClearance = mUi->edtClearanceCopperNpth->getValue();
  options.minCopperWidth = mUi->edtMinCopperWidth->getValue();
  options.minPthRestring = mUi->edtMinPthRestring->getValue();
  options.minPthDrillDiameter = mUi->edtMinPthDrillDiameter->getValue();
  options.minNpthDrillDiameter = mUi->edtMinNpthDrillDiameter->getValue();
  options.courtyardOffset = mUi->edtCourtyardOffset->getValue();
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
}  // namespace project
}  // namespace librepcb
