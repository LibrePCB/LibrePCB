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

#ifndef PROJECT_CMDGENCOMPATTRINSTREMOVE_H
#define PROJECT_CMDGENCOMPATTRINSTREMOVE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../../common/undocommand.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class GenCompInstance;
class GenCompAttributeInstance;
}

/*****************************************************************************************
 *  Class CmdGenCompAttrInstRemove
 ****************************************************************************************/

namespace project {

/**
 * @brief The CmdGenCompAttrInstRemove class
 */
class CmdGenCompAttrInstRemove final : public UndoCommand
{
    public:

        // Constructors / Destructor
        explicit CmdGenCompAttrInstRemove(GenCompInstance& genComp,
                                          GenCompAttributeInstance& attr,
                                          UndoCommand* parent = 0) throw (Exception);
        ~CmdGenCompAttrInstRemove() noexcept;

        // Inherited from UndoCommand
        void redo() throw (Exception) override;
        void undo() throw (Exception) override;

    private:

        GenCompInstance& mGenCompInstance;
        GenCompAttributeInstance& mAttrInstance;
};

} // namespace project

#endif // PROJECT_CMDGENCOMPATTRINSTREMOVE_H
