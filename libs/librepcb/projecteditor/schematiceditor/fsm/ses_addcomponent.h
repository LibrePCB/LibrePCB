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

#ifndef LIBREPCB_PROJECT_SES_ADDCOMPONENT_H
#define LIBREPCB_PROJECT_SES_ADDCOMPONENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "ses_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Component;
class ComponentSymbolVariant;
class ComponentSymbolVariantItem;
}

namespace project {

class ComponentInstance;
class SI_Symbol;
class CmdSymbolInstanceEdit;

namespace editor {

class AddComponentDialog;

/*****************************************************************************************
 *  Class SES_AddComponent
 ****************************************************************************************/

/**
 * @brief The SES_AddComponent class
 */
class SES_AddComponent final : public SES_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SES_AddComponent(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                                  GraphicsView& editorGraphicsView, UndoStack& undoStack);
        ~SES_AddComponent();

        // General Methods
        ProcRetVal process(SEE_Base* event) noexcept override;
        bool entry(SEE_Base* event) noexcept override;
        bool exit(SEE_Base* event) noexcept override;


    private:

        // Private Methods
        ProcRetVal processSceneEvent(SEE_Base* event) noexcept;
        void startAddingComponent(const Uuid& cmp = Uuid(), const Uuid& symbVar = Uuid()) throw (Exception);
        bool abortCommand(bool showErrMsgBox) noexcept;


        // Attributes
        bool mIsUndoCmdActive;
        AddComponentDialog* mAddComponentDialog;
        Angle mLastAngle;

        // information about the current component/symbol to place
        ComponentInstance* mCurrentComponent;
        int mCurrentSymbVarItemIndex;
        SI_Symbol* mCurrentSymbolToPlace;
        CmdSymbolInstanceEdit* mCurrentSymbolEditCommand;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SES_ADDCOMPONENT_H
