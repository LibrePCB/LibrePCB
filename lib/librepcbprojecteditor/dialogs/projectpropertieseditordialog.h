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

#ifndef PROJECT_PROJECTPROPERTIESEDITORDIALOG_H
#define PROJECT_PROJECTPROPERTIESEDITORDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class UndoStack;

namespace project {
class Project;
}

namespace Ui {
class ProjectPropertiesEditorDialog;
}

/*****************************************************************************************
 *  Class ProjectPropertiesEditorDialog
 ****************************************************************************************/

namespace project {

/**
 * @brief The ProjectPropertiesEditorDialog class
 */
class ProjectPropertiesEditorDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ProjectPropertiesEditorDialog(Project& project, UndoStack& undoStack,
                                               QWidget* parent);
        ~ProjectPropertiesEditorDialog() noexcept;


    private:

        // make some methods inaccessible...
        ProjectPropertiesEditorDialog();
        ProjectPropertiesEditorDialog(const ProjectPropertiesEditorDialog& other);
        ProjectPropertiesEditorDialog& operator=(const ProjectPropertiesEditorDialog& rhs);

        // Private Methods
        void keyPressEvent(QKeyEvent* e);
        void accept();
        bool applyChanges() noexcept;


        // General
        Project& mProject;
        Ui::ProjectPropertiesEditorDialog* mUi;
        UndoStack& mUndoStack;
        bool mCommandActive;
};

} // namespace project

#endif // PROJECT_PROJECTPROPERTIESEDITORDIALOG_H
