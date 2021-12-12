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
#include "renamenetsegmentdialog.h"

#include "../../project/cmd/cmdnetsignaladd.h"
#include "../../project/cmd/cmdnetsignaledit.h"
#include "../../undostack.h"
#include "../cmd/cmdchangenetsignalofschematicnetsegment.h"
#include "../cmd/cmdcombinenetsignals.h"
#include "ui_renamenetsegmentdialog.h"

#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>

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

RenameNetSegmentDialog::RenameNetSegmentDialog(UndoStack& undoStack,
                                               SI_NetSegment& segment,
                                               QWidget* parent) noexcept
  : QDialog(parent),
    mUndoStack(undoStack),
    mNetSegment(segment),
    mUi(new Ui::RenameNetSegmentDialog),
    mAction(Action::NONE),
    mNewNetName(),
    mNewNetSignal(nullptr) {
  mUi->setupUi(this);

  // Change completer to popup mode since the default inline completer is
  // annoying when you want to insert a new, non existing net name (you
  // would have to explicitly remove the autocompleted suffix).
  mUi->cbxNetName->completer()->setCompletionMode(QCompleter::PopupCompletion);

  // Populate netsignal combobox
  QStringList netsignals;
  foreach (const NetSignal* signal, mNetSegment.getCircuit().getNetSignals()) {
    Q_ASSERT(signal);
    // Ignore auto-named signals since typically the user is interested only
    // in manually named nets (and hundreds of auto-named nets would clutter
    // the dropdown anyway).
    if (!signal->hasAutoName()) {
      netsignals.append(*signal->getName());
    }
  }
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(netsignals.begin(), netsignals.end(),
            [&collator](const QString& lhs, const QString& rhs) {
              return collator(lhs, rhs);
            });
  mUi->cbxNetName->addItems(netsignals);
  int index = netsignals.indexOf(*segment.getNetSignal().getName());
  if (index >= 0) {
    mUi->cbxNetName->setCurrentIndex(index);
  } else {
    mUi->cbxNetName->setCurrentText(*segment.getNetSignal().getName());
  }

  int segmentCount = segment.getNetSignal().getSchematicNetSegments().count();
  mUi->rbtnRenameWholeNet->setText(
      QString(mUi->rbtnRenameWholeNet->text()).arg(segmentCount));
  if (segmentCount <= 1) {
    // segment == whole net, so the choice does not make sense
    mUi->rbtnRenameWholeNet->setChecked(true);
    mUi->rbtnRenameNetSegmentOnly->setEnabled(false);
  }
  updateAction();  // update description text

  // Set focus to net name to allow typing right after opening the dialog.
  mUi->cbxNetName->setFocus();
  mUi->cbxNetName->lineEdit()->selectAll();

  connect(mUi->cbxNetName, &QComboBox::currentTextChanged, this,
          &RenameNetSegmentDialog::updateAction);
  connect(mUi->rbtnRenameWholeNet, &QRadioButton::toggled, this,
          &RenameNetSegmentDialog::updateAction);
}

RenameNetSegmentDialog::~RenameNetSegmentDialog() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void RenameNetSegmentDialog::accept() noexcept {
  try {
    CircuitIdentifier name(mNewNetName);  // can throw
    switch (mAction) {
      case Action::RENAME_NETSIGNAL: {
        QScopedPointer<CmdNetSignalEdit> cmd(new CmdNetSignalEdit(
            mNetSegment.getCircuit(), mNetSegment.getNetSignal()));
        cmd->setName(name, false);
        mUndoStack.execCmd(cmd.take());  // can throw
        break;
      }
      case Action::MERGE_NETSIGNALS: {
        Q_ASSERT(mNewNetSignal);
        mUndoStack.execCmd(new CmdCombineNetSignals(
            mNetSegment.getCircuit(), mNetSegment.getNetSignal(),
            *mNewNetSignal));  // can throw
        break;
      }
      case Action::MOVE_NETSEGMENT_TO_EXISTING_NET:
      case Action::MOVE_NETSEGMENT_TO_NEW_NET: {
        UndoStackTransaction transaction(
            mUndoStack,
            tr("Change net of net segment"));  // can throw
        if (!mNewNetSignal) {
          CmdNetSignalAdd* cmd = new CmdNetSignalAdd(
              mNetSegment.getCircuit(),
              mNetSegment.getNetSignal().getNetClass(), name);
          transaction.append(cmd);  // can throw
          mNewNetSignal = cmd->getNetSignal();
          Q_ASSERT(mNewNetSignal);
        }
        transaction.append(new CmdChangeNetSignalOfSchematicNetSegment(
            mNetSegment, *mNewNetSignal));  // can throw
        transaction.commit();  // can throw
        break;
      }
      default: { break; }
    }
    QDialog::accept();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RenameNetSegmentDialog::updateAction() noexcept {
  mNewNetName = cleanCircuitIdentifier(mUi->cbxNetName->currentText());
  mNewNetSignal = mNetSegment.getCircuit().getNetSignalByName(mNewNetName);
  bool renameWholeNet = mUi->rbtnRenameWholeNet->isChecked();

  if (!mNewNetName.isEmpty()) {
    QString desc;
    if (mNewNetSignal == &mNetSegment.getNetSignal()) {
      mAction = Action::NONE;
      desc = tr("No change is made.");
    } else if (renameWholeNet && (mNewNetSignal)) {
      mAction = Action::MERGE_NETSIGNALS;
      desc = tr("The whole net '%1' will be merged into the net '%2'.")
                 .arg(*mNetSegment.getNetSignal().getName(), mNewNetName);
    } else if (renameWholeNet && (!mNewNetSignal)) {
      mAction = Action::RENAME_NETSIGNAL;
      desc = tr("The whole net '%1' will be renamed to '%2'.")
                 .arg(*mNetSegment.getNetSignal().getName(), mNewNetName);
    } else if ((!renameWholeNet) && (mNewNetSignal)) {
      mAction = Action::MOVE_NETSEGMENT_TO_EXISTING_NET;
      desc = tr("The segment will be moved to the existing net '%1'.")
                 .arg(mNewNetName);
    } else if ((!renameWholeNet) && (!mNewNetSignal)) {
      mAction = Action::MOVE_NETSEGMENT_TO_NEW_NET;
      desc =
          tr("The segment will be moved to the new net '%1'.").arg(mNewNetName);
    } else {
      mAction = Action::INVALID_NAME;  // Not correct, but sufficient
      desc = "UNKNOWN ERROR";
      qCritical() << "Unhandled case in RenameNetSegmentDialog!";
    }
    mUi->lblDescription->setText(desc);
    mUi->lblDescription->setStyleSheet("");
  } else {
    mAction = Action::INVALID_NAME;
    mUi->lblDescription->setText(tr("Invalid name!"));
    mUi->lblDescription->setStyleSheet("QLabel {color: red;}");
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
