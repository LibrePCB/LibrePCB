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
#include "cmdprojectlibraryaddelement.h"

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
CmdProjectLibraryAddElement<ElementType>::CmdProjectLibraryAddElement(
    ProjectLibrary& library, ElementType& element) noexcept
  : UndoCommand(tr("Add element to library")),
    mLibrary(library),
    mElement(element) {
}

template <typename ElementType>
CmdProjectLibraryAddElement<
    ElementType>::~CmdProjectLibraryAddElement() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

template <typename ElementType>
bool CmdProjectLibraryAddElement<ElementType>::performExecute() {
  performRedo();  // can throw

  return true;
}

template <typename ElementType>
void CmdProjectLibraryAddElement<ElementType>::performUndo() {
  removeElement();  // can throw
}

template <typename ElementType>
void CmdProjectLibraryAddElement<ElementType>::performRedo() {
  addElement();  // can throw
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <>
void CmdProjectLibraryAddElement<Symbol>::addElement() {
  mLibrary.addSymbol(mElement);
}

template <>
void CmdProjectLibraryAddElement<Package>::addElement() {
  mLibrary.addPackage(mElement);
}

template <>
void CmdProjectLibraryAddElement<Component>::addElement() {
  mLibrary.addComponent(mElement);
}

template <>
void CmdProjectLibraryAddElement<Device>::addElement() {
  mLibrary.addDevice(mElement);
}

template <>
void CmdProjectLibraryAddElement<Symbol>::removeElement() {
  mLibrary.removeSymbol(mElement);
}

template <>
void CmdProjectLibraryAddElement<Package>::removeElement() {
  mLibrary.removePackage(mElement);
}

template <>
void CmdProjectLibraryAddElement<Component>::removeElement() {
  mLibrary.removeComponent(mElement);
}

template <>
void CmdProjectLibraryAddElement<Device>::removeElement() {
  mLibrary.removeDevice(mElement);
}

/*******************************************************************************
 *  Explicit Template Instantiation
 ******************************************************************************/

template class CmdProjectLibraryAddElement<Symbol>;
template class CmdProjectLibraryAddElement<Package>;
template class CmdProjectLibraryAddElement<Component>;
template class CmdProjectLibraryAddElement<Device>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
