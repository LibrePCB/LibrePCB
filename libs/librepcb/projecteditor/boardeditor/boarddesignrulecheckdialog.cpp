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
    QWidget* parent) noexcept
  : QDialog(parent), mBoard(board), mUi(new Ui::BoardDesignRuleCheckDialog) {
  mUi->setupUi(this);
  mUi->edtClearanceCopperCopper->setSingleStep(0.1);  // [mm]
  mUi->edtClearanceCopperBoard->setSingleStep(0.1);   // [mm]
  mUi->edtClearanceCopperNpth->setSingleStep(0.1);    // [mm]
  mUi->edtMinCopperWidth->setSingleStep(0.1);         // [mm]
  mUi->edtMinPthRestring->setSingleStep(0.1);         // [mm]
  mUi->edtMinPthDrillDiameter->setSingleStep(0.1);    // [mm]
  mUi->edtMinNpthDrillDiameter->setSingleStep(0.1);   // [mm]
  mUi->edtCourtyardOffset->setSingleStep(0.1);        // [mm]
  connect(mUi->btnRun, &QPushButton::clicked, this,
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
  options.minCopperBoardClearance  = mUi->edtClearanceCopperBoard->getValue();
  options.minCopperNpthClearance   = mUi->edtClearanceCopperNpth->getValue();
  options.minCopperWidth           = mUi->edtMinCopperWidth->getValue();
  options.minPthRestring           = mUi->edtMinPthRestring->getValue();
  options.minPthDrillDiameter      = mUi->edtMinPthDrillDiameter->getValue();
  options.minNpthDrillDiameter     = mUi->edtMinNpthDrillDiameter->getValue();
  options.courtyardOffset          = mUi->edtCourtyardOffset->getValue();
  return options;
}

/*******************************************************************************
 *  GUI Event Handlers
 ******************************************************************************/

void BoardDesignRuleCheckDialog::btnRunDrcClicked() noexcept {
  mUi->grpOptions->setEnabled(false);
  mUi->btnRun->setEnabled(false);
  mUi->buttonBox->setEnabled(false);

  try {
    mUi->lstMessages->clear();
    mUi->lstProgress->clear();

    BoardDesignRuleCheck drc(mBoard, getOptions());
    connect(&drc, &BoardDesignRuleCheck::progressPercent, mUi->prgProgress,
            &QProgressBar::setValue);
    connect(&drc, &BoardDesignRuleCheck::progressStatus, mUi->lstProgress,
            static_cast<void (QListWidget::*)(const QString&)>(
                &QListWidget::addItem));
    connect(&drc, &BoardDesignRuleCheck::progressMessage, mUi->lstMessages,
            static_cast<void (QListWidget::*)(const QString&)>(
                &QListWidget::addItem));

    // Use the progressStatus() signal (because it is not emitted too often
    // which would lead to flickering) to update both list widgets.
    connect(&drc, SIGNAL(progressStatus(QString)), mUi->lstProgress,
            SLOT(repaint()));
    connect(&drc, SIGNAL(progressStatus(QString)), mUi->lstMessages,
            SLOT(repaint()));

    drc.execute();  // can throw
    mMessages = drc.getMessages();
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }

  mUi->grpOptions->setEnabled(true);
  mUi->btnRun->setEnabled(true);
  mUi->buttonBox->setEnabled(true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
