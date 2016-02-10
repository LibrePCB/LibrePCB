/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdprojectlibraryaddelement.h"
#include "../projectlibrary.h"
#include <librepcblibrary/elements.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

template <typename ElementType>
CmdProjectLibraryAddElement<ElementType>::CmdProjectLibraryAddElement(ProjectLibrary& library,
                                                                      ElementType& element,
                                                                      UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Add element to library"), parent),
    mLibrary(library), mElement(element)
{
}

template <typename ElementType>
CmdProjectLibraryAddElement<ElementType>::~CmdProjectLibraryAddElement() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

template <typename ElementType>
void CmdProjectLibraryAddElement<ElementType>::redo() throw (Exception)
{
    addElement(); // throws an exception on error

    try
    {
        UndoCommand::redo(); // throws an exception on error
    }
    catch (Exception &e)
    {
        removeElement(); // throws an exception on error
        throw;
    }
}

template <typename ElementType>
void CmdProjectLibraryAddElement<ElementType>::undo() throw (Exception)
{
    removeElement(); // throws an exception on error

    try
    {
        UndoCommand::undo(); // throws an exception on error
    }
    catch (Exception& e)
    {
        addElement(); // throws an exception on error
        throw;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

template <>
void CmdProjectLibraryAddElement<library::Symbol>::addElement()
{
    mLibrary.addSymbol(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::SpiceModel>::addElement()
{
    mLibrary.addSpiceModel(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Package>::addElement()
{
    mLibrary.addPackage(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Component>::addElement()
{
    mLibrary.addComponent(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Device>::addElement()
{
    mLibrary.addDevice(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Symbol>::removeElement()
{
    mLibrary.removeSymbol(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::SpiceModel>::removeElement()
{
    mLibrary.removeSpiceModel(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Package>::removeElement()
{
    mLibrary.removePackage(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Component>::removeElement()
{
    mLibrary.removeComponent(mElement);
}

template <>
void CmdProjectLibraryAddElement<library::Device>::removeElement()
{
    mLibrary.removeDevice(mElement);
}

/*****************************************************************************************
 *  Explicit Template Instantiation
 ****************************************************************************************/

template class CmdProjectLibraryAddElement<library::Symbol>;
template class CmdProjectLibraryAddElement<library::SpiceModel>;
template class CmdProjectLibraryAddElement<library::Package>;
template class CmdProjectLibraryAddElement<library::Component>;
template class CmdProjectLibraryAddElement<library::Device>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
