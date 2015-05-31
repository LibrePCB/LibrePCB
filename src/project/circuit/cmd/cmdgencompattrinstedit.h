/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_CMDGENCOMPATTRINSTEDIT_H
#define PROJECT_CMDGENCOMPATTRINSTEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/undocommand.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class AttributeType;
class AttributeUnit;

namespace project {
class GenCompInstance;
class GenCompAttributeInstance;
}

/*****************************************************************************************
 *  Class CmdGenCompAttrInstEdit
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompAttrInstEdit class
 */
class CmdGenCompAttrInstEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompAttrInstEdit(GenCompInstance& genComp,
                                        GenCompAttributeInstance& attr,
                                        const AttributeType& newType,
                                        const QString& newValue,
                                        const AttributeUnit* newUnit,
                                        UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompAttrInstEdit() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        // Attributes from the constructor
        GenCompInstance& mGenCompInst;
        GenCompAttributeInstance& mAttrInst;

        // General Attributes
        const AttributeType* mOldType;
        const AttributeType* mNewType;
        QString mOldValue;
        QString mNewValue;
        const AttributeUnit* mOldUnit;
        const AttributeUnit* mNewUnit;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPATTRINSTEDIT_H
