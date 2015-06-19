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

#ifndef PROJECT_CMDGENCOMPATTRINSTADD_H
#define PROJECT_CMDGENCOMPATTRINSTADD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/undocommand.h>

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
 *  Class CmdGenCompAttrInstAdd
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompAttrInstAdd class
 */
class CmdGenCompAttrInstAdd final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompAttrInstAdd(GenCompInstance& genComp, const QString& key,
                                       const AttributeType& type, const QString& value,
                                       const AttributeUnit* unit, UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompAttrInstAdd() noexcept;

        // Getters
        GenCompAttributeInstance* getAttrInstance() const noexcept {return mAttrInstance;}

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        GenCompInstance& mGenCompInstance;
        QString mKey;
        const AttributeType& mType;
        QString mValue;
        const AttributeUnit* mUnit;
        GenCompAttributeInstance* mAttrInstance;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPATTRINSTADD_H
