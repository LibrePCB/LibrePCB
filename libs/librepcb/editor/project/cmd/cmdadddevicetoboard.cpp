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
#include "cmdadddevicetoboard.h"

#include "../../project/cmd/cmdcomponentinstanceedit.h"
#include "../../project/cmd/cmddeviceinstanceadd.h"
#include "../../project/cmd/cmdprojectlibraryaddelement.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdAddDeviceToBoard::CmdAddDeviceToBoard(
    Workspace& workspace, Board& board, ComponentInstance& cmpInstance,
    const Uuid& deviceUuid, const tl::optional<Uuid>& footprintUuid,
    const tl::optional<Uuid>& preferredModelUuid, const Point& position,
    const Angle& rotation, bool mirror) noexcept
  : UndoCommandGroup(tr("Add device to board")),
    mWorkspace(workspace),
    mBoard(board),
    mComponentInstance(cmpInstance),
    mDeviceUuid(deviceUuid),
    mFootprintUuid(footprintUuid),
    mPreferredModelUuid(preferredModelUuid),
    mPosition(position),
    mRotation(rotation),
    mMirror(mirror),
    mDeviceInstance(nullptr) {
}

CmdAddDeviceToBoard::~CmdAddDeviceToBoard() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdAddDeviceToBoard::performExecute() {
  // if an error occurs, undo all already executed child commands
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  // if there is no such device in the project's library, copy it from the
  // workspace library to the project's library
  Device* dev = mBoard.getProject().getLibrary().getDevice(mDeviceUuid);
  if (!dev) {
    FilePath devFp = mWorkspace.getLibraryDb().getLatest<Device>(mDeviceUuid);
    if (!devFp.isValid()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The device with the UUID \"%1\" does not exist in the "
             "workspace library!")
              .arg(mDeviceUuid.toStr()));
    }
    dev = Device::open(std::unique_ptr<TransactionalDirectory>(
                           new TransactionalDirectory(
                               TransactionalFileSystem::openRO(devFp))))
              .release();  // can throw
    CmdProjectLibraryAddElement<Device>* cmdAddToLibrary =
        new CmdProjectLibraryAddElement<Device>(
            mBoard.getProject().getLibrary(), *dev);
    execNewChildCmd(cmdAddToLibrary);  // can throw
  }
  Q_ASSERT(dev);

  // if there is no such package in the project's library, copy it from the
  // workspace library to the project's library
  Uuid pkgUuid = dev->getPackageUuid();
  Package* pkg = mBoard.getProject().getLibrary().getPackage(pkgUuid);
  if (!pkg) {
    FilePath pkgFp = mWorkspace.getLibraryDb().getLatest<Package>(pkgUuid);
    if (!pkgFp.isValid()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The package with the UUID \"%1\" does not exist in the "
             "workspace library!")
              .arg(pkgUuid.toStr()));
    }
    pkg = Package::open(std::unique_ptr<TransactionalDirectory>(
                            new TransactionalDirectory(
                                TransactionalFileSystem::openRO(pkgFp))))
              .release();  // can throw
    CmdProjectLibraryAddElement<Package>* cmdAddToLibrary =
        new CmdProjectLibraryAddElement<Package>(
            mBoard.getProject().getLibrary(), *pkg);
    execNewChildCmd(cmdAddToLibrary);  // can throw
  }
  Q_ASSERT(pkg);

  // TODO: remove this!
  if (!mFootprintUuid && pkg->getFootprints().count() > 0) {
    mFootprintUuid = pkg->getFootprints().first()->getUuid();
  }
  if (!mFootprintUuid) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Package does not have any footprints: %1")
                           .arg(pkg->getUuid().toStr()));
  }

  // create new device (ownership by board)
  mDeviceInstance =
      new BI_Device(mBoard, mComponentInstance, mDeviceUuid, *mFootprintUuid,
                    mPosition, mRotation, mMirror, false, true);  // can throw

  // Assign 3D model. Use the default model if no valid preferred model was
  // specified.
  if ((!mPreferredModelUuid) ||
      (!pkg->getModels().contains(*mPreferredModelUuid)) ||
      (!pkg->getFootprints()
            .get(*mFootprintUuid)
            ->getModels()
            .contains(*mPreferredModelUuid))) {
    mPreferredModelUuid = mDeviceInstance->getDefaultLibModelUuid();
  }
  mDeviceInstance->setModel(mPreferredModelUuid);

  // Make sure there is at least one assembly option for this device.
  if (!mComponentInstance.getCompatibleDevices().contains(mDeviceUuid)) {
    if (mComponentInstance.getLockAssembly()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The component in the schematic does not specify the chosen "
             "device as compatible and is locked for modifications from the "
             "board editor. Either add a corresponding assembly option to the "
             "component in the schematic, or remove the lock from the "
             "component."));
    }
    const QSet<Uuid> assemblyVariants =
        mDeviceInstance->doesPackageRequireAssembly(true)
        ? mComponentInstance.getCircuit().getAssemblyVariants().getUuidSet()
        : QSet<Uuid>();
    ComponentAssemblyOptionList assemblyOptions =
        mComponentInstance.getAssemblyOptions();
    assemblyOptions.append(std::make_shared<ComponentAssemblyOption>(
        mDeviceUuid, mDeviceInstance->getLibDevice().getAttributes(),
        assemblyVariants, PartList{}));
    std::unique_ptr<CmdComponentInstanceEdit> cmd(new CmdComponentInstanceEdit(
        mComponentInstance.getCircuit(), mComponentInstance));
    cmd->setAssemblyOptions(assemblyOptions);
    execNewChildCmd(cmd.release());  // can throw
  }

  // add a new device instance to the board
  execNewChildCmd(new CmdDeviceInstanceAdd(*mDeviceInstance));  // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
