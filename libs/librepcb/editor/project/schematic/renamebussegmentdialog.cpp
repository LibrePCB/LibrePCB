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
#include "renamebussegmentdialog.h"

#include "../../project/cmd/cmdbusadd.h"
#include "../../project/cmd/cmdbusedit.h"
#include "../../undostack.h"
#include "../cmd/cmdchangebusofschematicbussegment.h"
#include "../cmd/cmdcombinebuses.h"
#include "ui_renamebussegmentdialog.h"

#include <librepcb/core/project/circuit/bus.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/schematic/items/si_bussegment.h>
#include <librepcb/core/utils/toolbox.h>

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

RenameBusSegmentDialog::RenameBusSegmentDialog(UndoStack& undoStack,
                                               SI_BusSegment& segment,
                                               QWidget* parent) noexcept
  : QDialog(parent),
    mUndoStack(undoStack),
    mSegment(segment),
    mUi(new Ui::RenameBusSegmentDialog),
    mAction(Action::NONE),
    mNewBusName(),
    mNewBus(nullptr) {
  mUi->setupUi(this);

  // Change completer to popup mode since the default inline completer is
  // annoying when you want to insert a new, non existing bus name (you
  // would have to explicitly remove the autocompleted suffix).
  mUi->cbxBusName->completer()->setCompletionMode(QCompleter::PopupCompletion);

  // Populate buses combobox
  QStringList buses;
  foreach (const Bus* bus, mSegment.getCircuit().getBuses()) {
    Q_ASSERT(bus);
    // Ignore auto-named buses since typically the user is interested only
    // in manually named buses.
    if (!bus->hasAutoName()) {
      buses.append(*bus->getName());
    }
  }
  Toolbox::sortNumeric(buses, Qt::CaseInsensitive, false);
  mUi->cbxBusName->addItems(buses);
  int index = buses.indexOf(*segment.getBus().getName());
  if (index >= 0) {
    mUi->cbxBusName->setCurrentIndex(index);
  } else {
    mUi->cbxBusName->setCurrentText(*segment.getBus().getName());
  }

  int segmentCount = segment.getBus().getSchematicBusSegments().count();
  mUi->rbtnRenameWholeBus->setText(
      QString(mUi->rbtnRenameWholeBus->text()).arg(segmentCount));
  if (segmentCount <= 1) {
    // segment == whole bus, so the choice does not make sense
    mUi->rbtnRenameWholeBus->setChecked(true);
    mUi->rbtnRenameBusSegmentOnly->setEnabled(false);
  }
  updateAction();  // update description text

  // Set focus to bus name to allow typing right after opening the dialog.
  mUi->cbxBusName->setFocus();
  mUi->cbxBusName->lineEdit()->selectAll();

  connect(mUi->cbxBusName, &QComboBox::currentTextChanged, this,
          &RenameBusSegmentDialog::updateAction);
  connect(mUi->rbtnRenameWholeBus, &QRadioButton::toggled, this,
          &RenameBusSegmentDialog::updateAction);
}

RenameBusSegmentDialog::~RenameBusSegmentDialog() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void RenameBusSegmentDialog::accept() noexcept {
  try {
    BusName name(mNewBusName);  // can throw
    switch (mAction) {
      case Action::RENAME_BUS: {
        std::unique_ptr<CmdBusEdit> cmd(new CmdBusEdit(mSegment.getBus()));
        cmd->setName(name, false);
        mUndoStack.execCmd(cmd.release());  // can throw
        break;
      }
      case Action::MERGE_BUSES: {
        Q_ASSERT(mNewBus);
        mUndoStack.execCmd(new CmdCombineBuses(mSegment.getCircuit(),
                                               mSegment.getBus(),
                                               *mNewBus));  // can throw
        break;
      }
      case Action::MOVE_SEGMENT_TO_EXISTING_BUS:
      case Action::MOVE_SEGMENT_TO_NEW_BUS: {
        UndoStackTransaction transaction(
            mUndoStack,
            tr("Change Bus of Bus Segment"));  // can throw
        if (!mNewBus) {
          mNewBus = new Bus(mSegment.getCircuit(), Uuid::createRandom(), name,
                            false, false, std::nullopt);
          transaction.append(new CmdBusAdd(*mNewBus));  // can throw
        }
        transaction.append(new CmdChangeBusOfSchematicBusSegment(
            mSegment, *mNewBus));  // can throw
        transaction.commit();  // can throw
        break;
      }
      default: {
        break;
      }
    }
    QDialog::accept();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RenameBusSegmentDialog::updateAction() noexcept {
  mNewBusName = cleanBusName(mUi->cbxBusName->currentText());
  mNewBus = mSegment.getCircuit().getBusByName(mNewBusName);
  bool renameWholeBus = mUi->rbtnRenameWholeBus->isChecked();

  if (!mNewBusName.isEmpty()) {
    QString desc;
    if (mNewBus == &mSegment.getBus()) {
      mAction = Action::NONE;
      desc = tr("No change is made.");
    } else if (renameWholeBus && (mNewBus)) {
      mAction = Action::MERGE_BUSES;
      desc = tr("The whole bus '%1' will be merged into the bus '%2'.")
                 .arg(*mSegment.getBus().getName(), mNewBusName);
    } else if (renameWholeBus && (!mNewBus)) {
      mAction = Action::RENAME_BUS;
      desc = tr("The whole bus '%1' will be renamed to '%2'.")
                 .arg(*mSegment.getBus().getName(), mNewBusName);
    } else if ((!renameWholeBus) && (mNewBus)) {
      mAction = Action::MOVE_SEGMENT_TO_EXISTING_BUS;
      desc = tr("The segment will be moved to the existing bus '%1'.")
                 .arg(mNewBusName);
    } else if ((!renameWholeBus) && (!mNewBus)) {
      mAction = Action::MOVE_SEGMENT_TO_NEW_BUS;
      desc =
          tr("The segment will be moved to the new bus '%1'.").arg(mNewBusName);
    } else {
      mAction = Action::INVALID_NAME;  // Not correct, but sufficient
      desc = "UNKNOWN ERROR";
      qCritical()
          << "Unhandled case in RenameBusSegmentDialog::updateAction()!";
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
