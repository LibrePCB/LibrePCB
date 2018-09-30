/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "boardlayerstacksetupdialog.h"

#include "ui_boardlayerstacksetupdialog.h"

#include <librepcb/common/undostack.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardlayerstackedit.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardLayerStackSetupDialog::BoardLayerStackSetupDialog(
    BoardLayerStack& layerStack, UndoStack& undoStack, QWidget* parent) noexcept
  : QDialog(parent),
    mLayerStack(layerStack),
    mUndoStack(undoStack),
    mUi(new Ui::BoardLayerStackSetupDialog) {
  mUi->setupUi(this);

  mUi->spbxNbrOfInnerCopperLayers->setMinimum(0);
  mUi->spbxNbrOfInnerCopperLayers->setMaximum(
      GraphicsLayer::getInnerLayerCount());
  mUi->spbxNbrOfInnerCopperLayers->setValue(mLayerStack.getInnerLayerCount());
}

BoardLayerStackSetupDialog::~BoardLayerStackSetupDialog() noexcept {
  mUi.reset();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardLayerStackSetupDialog::keyPressEvent(QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Return:
      accept();
      break;
    case Qt::Key_Escape:
      reject();
      break;
    default:
      QDialog::keyPressEvent(e);
      break;
  }
}

void BoardLayerStackSetupDialog::accept() {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool BoardLayerStackSetupDialog::applyChanges() noexcept {
  try {
    QScopedPointer<CmdBoardLayerStackEdit> cmd(
        new CmdBoardLayerStackEdit(mLayerStack));
    cmd->setInnerLayerCount(mUi->spbxNbrOfInnerCopperLayers->value());
    mUndoStack.execCmd(cmd.take());
    return true;
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
