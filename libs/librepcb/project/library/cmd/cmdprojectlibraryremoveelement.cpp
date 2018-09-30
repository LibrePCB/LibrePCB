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
#include "cmdprojectlibraryremoveelement.h"

#include "../projectlibrary.h"

#include <librepcb/library/elements.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

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
void CmdProjectLibraryRemoveElement<library::Symbol>::addElement() {
  mLibrary.addSymbol(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Package>::addElement() {
  mLibrary.addPackage(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Component>::addElement() {
  mLibrary.addComponent(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Device>::addElement() {
  mLibrary.addDevice(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Symbol>::removeElement() {
  mLibrary.removeSymbol(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Package>::removeElement() {
  mLibrary.removePackage(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Component>::removeElement() {
  mLibrary.removeComponent(mElement);
}

template <>
void CmdProjectLibraryRemoveElement<library::Device>::removeElement() {
  mLibrary.removeDevice(mElement);
}

/*******************************************************************************
 *  Explicit Template Instantiation
 ******************************************************************************/

template class CmdProjectLibraryRemoveElement<library::Symbol>;
template class CmdProjectLibraryRemoveElement<library::Package>;
template class CmdProjectLibraryRemoveElement<library::Component>;
template class CmdProjectLibraryRemoveElement<library::Device>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
