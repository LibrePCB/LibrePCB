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

#ifndef LIBREPCB_PROJECT_CMDPROJECTLIBRARYADDELEMENT_H
#define LIBREPCB_PROJECT_CMDPROJECTLIBRARYADDELEMENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace project {

class ProjectLibrary;

/*****************************************************************************************
 *  Class CmdProjectLibraryAddElement
 ****************************************************************************************/

/**
 * @brief The CmdProjectLibraryAddElement class
 */
template <typename ElementType>
class CmdProjectLibraryAddElement final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdProjectLibraryAddElement(ProjectLibrary& library,
                                             const ElementType& element,
                                             UndoCommand* parent = 0) throw (Exception);
        ~CmdProjectLibraryAddElement() noexcept;

        // Getters
        const ElementType& getElement() const noexcept {return mElement;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        void addElement();
        void removeElement();


        ProjectLibrary& mLibrary;
        const ElementType& mElement;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDPROJECTLIBRARYADDELEMENT_H
