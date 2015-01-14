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

#ifndef PROJECT_SYMBOLINSTANCEPROPERTIESDIALOG_H
#define PROJECT_SYMBOLINSTANCEPROPERTIESDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class UndoCommand;

namespace project {
class Project;
class GenCompInstance;
class SymbolInstance;
}

namespace Ui {
class SymbolInstancePropertiesDialog;
}

/*****************************************************************************************
 *  Class SymbolInstancePropertiesDialog
 ****************************************************************************************/

namespace project {

/**
 * @brief The SymbolInstancePropertiesDialog class
 */
class SymbolInstancePropertiesDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SymbolInstancePropertiesDialog(Project& project, GenCompInstance& genComp,
                                                SymbolInstance& symbol, QWidget* parent) noexcept;
        ~SymbolInstancePropertiesDialog() noexcept;


    private:

        // make some methods inaccessible...
        SymbolInstancePropertiesDialog();
        SymbolInstancePropertiesDialog(const SymbolInstancePropertiesDialog& other);
        SymbolInstancePropertiesDialog& operator=(const SymbolInstancePropertiesDialog& rhs);

        // Private Methods
        void accept();
        bool applyChanges() noexcept;
        void execCmd(UndoCommand* cmd);
        void endCmd();
        void abortCmd();


        // General
        Project& mProject;
        GenCompInstance& mGenCompInstance;
        SymbolInstance& mSymbolInstance;
        Ui::SymbolInstancePropertiesDialog* mUi;
        bool mCommandActive;
};

} // namespace project

#endif // PROJECT_SYMBOLINSTANCEPROPERTIESDIALOG_H
