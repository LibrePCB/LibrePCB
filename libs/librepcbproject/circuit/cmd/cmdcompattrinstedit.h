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

#ifndef LIBREPCB_PROJECT_CMDCOMPATTRINSTEDIT_H
#define LIBREPCB_PROJECT_CMDCOMPATTRINSTEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/undocommand.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class AttributeType;
class AttributeUnit;

namespace project {

class ComponentInstance;
class ComponentAttributeInstance;

/*****************************************************************************************
 *  Class CmdCompAttrInstEdit
 ****************************************************************************************/

/**
 * @brief The CmdCompAttrInstEdit class
 */
class CmdCompAttrInstEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdCompAttrInstEdit(ComponentInstance& cmp, ComponentAttributeInstance& attr,
                            const AttributeType& newType, const QString& newValue,
                            const AttributeUnit* newUnit) noexcept;
        ~CmdCompAttrInstEdit() noexcept;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() throw (Exception) override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() throw (Exception) override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() throw (Exception) override;


        // Private Member Variables

        // Attributes from the constructor
        ComponentInstance& mComponentInstance;
        ComponentAttributeInstance& mAttrInst;

        // General Attributes
        const AttributeType* mOldType;
        const AttributeType* mNewType;
        QString mOldValue;
        QString mNewValue;
        const AttributeUnit* mOldUnit;
        const AttributeUnit* mNewUnit;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_CMDCOMPATTRINSTEDIT_H
