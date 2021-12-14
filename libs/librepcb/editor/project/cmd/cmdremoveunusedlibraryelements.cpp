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
#include "cmdremoveunusedlibraryelements.h"

#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/library/cmd/cmdprojectlibraryremoveelement.h>
#include <librepcb/project/library/projectlibrary.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/schematic.h>

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

CmdRemoveUnusedLibraryElements::CmdRemoveUnusedLibraryElements(
    Project& project) noexcept
  : UndoCommandGroup(tr("Remove unused library elements")), mProject(project) {
}

CmdRemoveUnusedLibraryElements::~CmdRemoveUnusedLibraryElements() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveUnusedLibraryElements::performExecute() {
  // get all used elements
  QSet<const library::Component*> usedComponents;
  QSet<const library::Device*> usedDevices;
  QSet<const library::Package*> usedPackages;
  QSet<const library::Symbol*> usedSymbols;
  foreach (const ComponentInstance* ci,
           mProject.getCircuit().getComponentInstances()) {
    Q_ASSERT(ci);
    usedComponents.insert(&ci->getLibComponent());
  }
  foreach (const Board* brd, mProject.getBoards()) {
    Q_ASSERT(brd);
    foreach (const BI_Device* dev, brd->getDeviceInstances()) {
      Q_ASSERT(dev);
      usedPackages.insert(&dev->getLibPackage());
      usedDevices.insert(&dev->getLibDevice());
    }
  }
  foreach (const Schematic* sch, mProject.getSchematics()) {
    Q_ASSERT(sch);
    foreach (const SI_Symbol* si, sch->getSymbols()) {
      Q_ASSERT(si);
      usedSymbols.insert(&si->getLibSymbol());
    }
  }

  // remove unused symbols
  foreach (library::Symbol* element, mProject.getLibrary().getSymbols()) {
    if (!usedSymbols.contains(element)) {
      appendChild(new CmdProjectLibraryRemoveElement<library::Symbol>(
          mProject.getLibrary(), *element));
    }
  }

  // remove unused packages
  foreach (library::Package* element, mProject.getLibrary().getPackages()) {
    if (!usedPackages.contains(element)) {
      appendChild(new CmdProjectLibraryRemoveElement<library::Package>(
          mProject.getLibrary(), *element));
    }
  }

  // remove unused devices
  foreach (library::Device* element, mProject.getLibrary().getDevices()) {
    if (!usedDevices.contains(element)) {
      appendChild(new CmdProjectLibraryRemoveElement<library::Device>(
          mProject.getLibrary(), *element));
    }
  }

  // remove unused components
  foreach (library::Component* element, mProject.getLibrary().getComponents()) {
    if (!usedComponents.contains(element)) {
      appendChild(new CmdProjectLibraryRemoveElement<library::Component>(
          mProject.getLibrary(), *element));
    }
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
