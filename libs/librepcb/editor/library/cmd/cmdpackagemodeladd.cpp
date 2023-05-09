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
#include "cmdpackagemodeladd.h"

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

CmdPackageModelAdd::CmdPackageModelAdd(Package& pkg,
                                       std::shared_ptr<PackageModel> model,
                                       const QByteArray& fileContent,
                                       bool addToFootprints) noexcept
  : UndoCommand(tr("Add 3D model")),
    mPackage(pkg),
    mModel(model),
    mFileContent(fileContent),
    mAddToFootprints(addToFootprints) {
}

CmdPackageModelAdd::~CmdPackageModelAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPackageModelAdd::performExecute() {
  if (mAddToFootprints) {
    for (auto footprint : mPackage.getFootprints().values()) {
      if (!footprint->getModels().contains(mModel->getUuid())) {
        mAddedToFootprints.append(footprint);
      }
    }
  }

  performRedo();  // can throw
  return true;
}

void CmdPackageModelAdd::performUndo() {
  if (!mFileContent.isEmpty()) {
    mPackage.getDirectory().removeFile(mModel->getFileName());  // can throw
  }
  foreach (auto footprint, mAddedToFootprints) {
    QSet<Uuid> models = footprint->getModels();
    models.remove(mModel->getUuid());
    footprint->setModels(models);
  }
  mPackage.getModels().remove(mModel.get());
}

void CmdPackageModelAdd::performRedo() {
  if (!mFileContent.isEmpty()) {
    if (mPackage.getDirectory().fileExists(mModel->getFileName())) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("File exists already: %1").arg(mModel->getFileName()));
    }
    mPackage.getDirectory().write(mModel->getFileName(),
                                  mFileContent);  // can throw
  }
  mPackage.getModels().append(mModel);
  foreach (auto footprint, mAddedToFootprints) {
    QSet<Uuid> models = footprint->getModels();
    models.insert(mModel->getUuid());
    footprint->setModels(models);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
