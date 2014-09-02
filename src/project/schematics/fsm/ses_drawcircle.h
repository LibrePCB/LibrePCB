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

#ifndef PROJECT_SES_DRAWCIRCLE_H
#define PROJECT_SES_DRAWCIRCLE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "schematiceditorstate.h"

/*****************************************************************************************
 *  Class SES_DrawCircle
 ****************************************************************************************/

namespace project {


/**
 * @brief The SES_DrawCircle class
 */
class SES_DrawCircle final : public SchematicEditorState
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SES_DrawCircle(SchematicEditor& editor);
        ~SES_DrawCircle();

        // General Methods
        State process(SchematicEditorEvent* event) noexcept;
        void entry(State previousState) noexcept;
        void exit(State nextState) noexcept;
};

} // namespace project

#endif // PROJECT_SES_DRAWCIRCLE_H
