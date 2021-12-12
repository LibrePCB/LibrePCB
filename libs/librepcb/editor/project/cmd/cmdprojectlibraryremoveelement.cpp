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
#include "cmdprojectlibraryremoveelement.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/projectlibrary.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename ElementType>
CmdProjectLibraryRemoveElement<ElementType>::CmdProjectLibraryRemoveElement(
    ProjectLibrary& library, ElementType& element) noexcept
  : UndoCommand(tr("Remove element from library")),
    mLibrary(library),
    mElement(element) {
}

template <typename ElementType>
CmdProjectLibraryRemoveElement<
    ElementType>::~CmdProjectLibraryRemoveElement() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

template <typename ElementType>
bool CmdProjectLibraryRemoveElement<ElementType>::performExecute() {
  performRedo();  // can throw

  return true;
}

template <typename ElementType>
void CmdProjectLibraryRemoveElement<ElementType>::performUndo() {
  addElement();  // can throw
}

template <typename ElementType>
void CmdProjectLibraryRemoveElement<ElementType>::performRedo() {
  removeElement();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <>
void CmdProjectLibraryRemoveElement<Symbol>::addElement() {
  mLibrary.addSymbol(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Package>::addElement() {
  mLibrary.addPackage(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Component>::addElement() {
  mLibrary.addComponent(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Device>::addElement() {
  mLibrary.addDevice(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Symbol>::removeElement() {
  mLibrary.removeSymbol(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Package>::removeElement() {
  mLibrary.removePackage(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Component>::removeElement() {
  mLibrary.removeComponent(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<Device>::removeElement() {
  mLibrary.removeDevice(mElement);
}

/*******************************************************************************
 *  Explicit Template Instantiation
 ******************************************************************************/

template class CmdProjectLibraryRemoveElement<Symbol>;
template class CmdProjectLibraryRemoveElement<Package>;
template class CmdProjectLibraryRemoveElement<Component>;
template class CmdProjectLibraryRemoveElement<Device>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
