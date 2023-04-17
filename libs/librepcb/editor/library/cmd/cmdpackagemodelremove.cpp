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
#include "cmdpackagemodelremove.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/pkg/packagemodel.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPackageModelRemove::CmdPackageModelRemove(
    Package& pkg, std::shared_ptr<PackageModel> model) noexcept
  : UndoCommand(tr("Remove 3D model")),
    mPackage(pkg),
    mModel(model),
    mIndex(-1) {
}

CmdPackageModelRemove::~CmdPackageModelRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackageModelRemove::performExecute() {
  mFileContent = mPackage.getDirectory().readIfExists(mModel->getFileName());

  mIndex = mPackage.getModels().indexOf(mModel.get());
  if (mIndex < 0) throw LogicError(__FILE__, __LINE__, "Element not in list.");

  for (auto footprint : mPackage.getFootprints().values()) {
    if (footprint->getModels().contains(mModel->getUuid())) {
      mRemovedFromFootprints.append(footprint);
    }
  }

  performRedo();  // can throw
  return true;
}

void CmdPackageModelRemove::performUndo() {
  if (!mFileContent.isNull()) {
    mPackage.getDirectory().write(mModel->getFileName(),
                                  mFileContent);  // can throw
  }
  foreach (auto footprint, mRemovedFromFootprints) {
    QSet<Uuid> models = footprint->getModels();
    models.insert(mModel->getUuid());
    footprint->setModels(models);
  }
  mPackage.getModels().insert(mIndex, mModel);
}

void CmdPackageModelRemove::performRedo() {
  if (!mFileContent.isNull()) {
    mPackage.getDirectory().removeFile(mModel->getFileName());  // can throw
  }
  foreach (auto footprint, mRemovedFromFootprints) {
    QSet<Uuid> models = footprint->getModels();
    models.remove(mModel->getUuid());
    footprint->setModels(models);
  }
  auto item = mPackage.getModels().take(mIndex);
  Q_ASSERT(item == mModel);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
