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

#ifndef PROJECT_SES_ADDCOMPONENTS_H
#define PROJECT_SES_ADDCOMPONENTS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "ses_base.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace library {
class Component;
class ComponentSymbolVariant;
class ComponentSymbolVariantItem;
}

namespace project {
class ComponentInstance;
class SI_Symbol;
class CmdSymbolInstanceEdit;
class AddGenCompDialog;
}

/*****************************************************************************************
 *  Class SES_AddComponents
 ****************************************************************************************/

namespace project {


/**
 * @brief The SES_AddComponents class
 */
class SES_AddComponents final : public SES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SES_AddComponents(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                                   GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~SES_AddComponents();

        // General Methods
        ProcRetVal process(SEE_Base* event) noexcept override;
        bool entry(SEE_Base* event) noexcept override;
        bool exit(SEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSceneEvent(SEE_Base* event) noexcept;
        void startAddingComponent(const Uuid& genComp = Uuid(), const Uuid& symbVar = Uuid()) throw (Exception);
        bool abortCommand(bool showErrMsgBox) noexcept;


        // Attributes
        bool mIsUndoCmdActive;
        AddGenCompDialog* mAddGenCompDialog;
        Angle mLastAngle;

        // information about the current symbol to place
        const library::Component* mGenComp;
        const library::ComponentSymbolVariant* mGenCompSymbVar;
        const library::ComponentSymbolVariantItem* mCurrentSymbVarItem;
        SI_Symbol* mCurrentSymbolToPlace;
        CmdSymbolInstanceEdit* mCurrentSymbolEditCommand;
};

} // namespace project

#endif // PROJECT_SES_ADDCOMPONENTS_H
