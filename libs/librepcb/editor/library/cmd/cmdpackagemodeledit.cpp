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
#include "cmdpackagemodeledit.h"

#include <librepcb/core/library/pkg/package.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPackageModelEdit::CmdPackageModelEdit(Package& package,
                                         PackageModel& model) noexcept
  : UndoCommand(tr("Edit 3D Model")),
    mPackage(package),
    mModel(model),
    mOldName(model.getName()),
    mNewName(mOldName),
    mOldStepContent(),
    mNewStepContent() {
}

CmdPackageModelEdit::~CmdPackageModelEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdPackageModelEdit::setName(const ElementName& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

void CmdPackageModelEdit::setStepContent(const QByteArray& content) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewStepContent = content;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackageModelEdit::performExecute() {
  if ((!mNewStepContent.isNull()) &&
      (mPackage.getDirectory().fileExists(mModel.getFileName()))) {
    mOldStepContent = mPackage.getDirectory().read(mModel.getFileName());
  }

  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewStepContent != mOldStepContent) return true;
  return false;
}

void CmdPackageModelEdit::performUndo() {
  if (!mNewStepContent.isNull()) {
    if (mOldStepContent.isNull()) {
      mPackage.getDirectory().removeFile(mModel.getFileName());
    } else {
      mPackage.getDirectory().write(mModel.getFileName(), mOldStepContent);
    }
  }
  mModel.setName(mOldName);
}

void CmdPackageModelEdit::performRedo() {
  if (!mNewStepContent.isNull()) {
    mPackage.getDirectory().write(mModel.getFileName(),
                                  mNewStepContent);  // can throw
  }
  mModel.setName(mNewName);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
