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

#ifndef PROJECT_SCHEMATICEDITOR_H
#define PROJECT_SCHEMATICEDITOR_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../../common/cadscene.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace Ui {
class SchematicEditor;
}

namespace project {
class Project;
class Schematic;
class SchematicPagesDock;
class ErcMsgDock;
class SES_FSM;
}

/*****************************************************************************************
 *  Class SchematicEditor
 ****************************************************************************************/

namespace project {

/**
 * @brief The SchematicEditor class
 */
class SchematicEditor : public QMainWindow, public IF_CADSceneEventHandler
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicEditor(Project& project, bool readOnly);
        ~SchematicEditor();

        // Getters
        Project& getProject() const noexcept {return mProject;}
        int getActiveSchematicIndex() const noexcept {return mActiveSchematicIndex;}
        Schematic* getActiveSchematic() const noexcept;

        // Setters
        bool setActiveSchematicIndex(int index) noexcept;

    protected:

        void closeEvent(QCloseEvent* event);

    private slots:

        // Actions
        void on_actionClose_Project_triggered();
        void on_actionUndo_triggered();
        void on_actionRedo_triggered();
        void on_actionGrid_triggered();
        void on_actionPDF_Export_triggered();
        void on_actionToolAddComponent_triggered();
        void on_actionAddGenCmp_Resistor_triggered();
        void on_actionAddGenCmp_Capacitor_triggered();
        void on_actionAddGenCmp_Inductor_triggered();
        void on_actionAddGenCmp_gnd_triggered();
        void on_actionAddGenCmp_vcc_triggered();
        void on_actionProjectProperties_triggered();

    signals:

        void activeSchematicChanged(int oldIndex, int newIndex);

    private:

        // make some methods inaccessible...
        SchematicEditor();
        SchematicEditor(const SchematicEditor& other);
        SchematicEditor& operator=(const SchematicEditor& rhs);

        // Private Methods
        bool cadSceneEventHandler(QEvent* event);

        // General Attributes
        Project& mProject;
        Ui::SchematicEditor* mUi;

        int mActiveSchematicIndex;

        // Docks
        SchematicPagesDock* mPagesDock;
        ErcMsgDock* mErcMsgDock;

        // Finite State Machine
        SES_FSM* mFsm;
};

} // namespace project

#endif // PROJECT_SCHEMATICEDITOR_H
