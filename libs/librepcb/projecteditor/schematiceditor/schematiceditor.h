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

#ifndef LIBREPCB_PROJECT_SCHEMATICEDITOR_H
#define LIBREPCB_PROJECT_SCHEMATICEDITOR_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/graphics/if_graphicsvieweventhandler.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsView;
class GridProperties;
class UndoStackActionGroup;
class ExclusiveActionGroup;

namespace project {

class Project;
class Schematic;

namespace editor {

class ProjectEditor;
class SchematicPagesDock;
class ErcMsgDock;
class SES_FSM;

namespace Ui {
class SchematicEditor;
}

/*****************************************************************************************
 *  Class SchematicEditor
 ****************************************************************************************/

/**
 * @brief The SchematicEditor class
 */
class SchematicEditor final : public QMainWindow, public IF_GraphicsViewEventHandler
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicEditor(ProjectEditor& projectEditor, Project& project);
        ~SchematicEditor();

        // Getters
        ProjectEditor& getProjectEditor() const noexcept {return mProjectEditor;}
        Project& getProject() const noexcept {return mProject;}
        int getActiveSchematicIndex() const noexcept {return mActiveSchematicIndex;}
        Schematic* getActiveSchematic() const noexcept;
        const GridProperties& getGridProperties() const noexcept {return *mGridProperties;}

        // Setters
        bool setActiveSchematicIndex(int index) noexcept;

        // General Methods
        void abortAllCommands() noexcept;

    protected:

        void closeEvent(QCloseEvent* event);

    private slots:

        // Actions
        void on_actionClose_Project_triggered();
        void on_actionNew_Schematic_Page_triggered();
        void on_actionGrid_triggered();
        void on_actionPDF_Export_triggered();
        void on_actionAddComp_Resistor_triggered();
        void on_actionAddComp_BipolarCapacitor_triggered();
        void on_actionAddComp_UnipolarCapacitor_triggered();
        void on_actionAddComp_Inductor_triggered();
        void on_actionAddComp_gnd_triggered();
        void on_actionAddComp_vcc_triggered();
        void on_actionProjectProperties_triggered();


    signals:

        void activeSchematicChanged(int oldIndex, int newIndex);


    private:

        // make some methods inaccessible...
        SchematicEditor();
        SchematicEditor(const SchematicEditor& other);
        SchematicEditor& operator=(const SchematicEditor& rhs);

        // Private Methods
        bool graphicsViewEventHandler(QEvent* event);
        void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;

        // General Attributes
        ProjectEditor& mProjectEditor;
        Project& mProject;
        Ui::SchematicEditor* mUi;
        GraphicsView* mGraphicsView;
        GridProperties* mGridProperties;
        QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
        QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;

        int mActiveSchematicIndex;

        // Docks
        SchematicPagesDock* mPagesDock;
        ErcMsgDock* mErcMsgDock;

        // Finite State Machine
        SES_FSM* mFsm;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_SCHEMATICEDITOR_H
