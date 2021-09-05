/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/graphics/if_graphicsvieweventhandler.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsView;
class UndoStackActionGroup;
class ExclusiveActionGroup;

namespace project {

class Project;
class Schematic;
class SI_Symbol;

namespace editor {

class ProjectEditor;
class SchematicPagesDock;
class ErcMsgDock;
class SchematicEditorFsm;

namespace Ui {
class SchematicEditor;
}

/*******************************************************************************
 *  Class SchematicEditor
 ******************************************************************************/

/**
 * @brief The SchematicEditor class
 */
class SchematicEditor final : public QMainWindow,
                              public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditor() = delete;
  SchematicEditor(const SchematicEditor& other) = delete;
  explicit SchematicEditor(ProjectEditor& projectEditor, Project& project);
  ~SchematicEditor();

  // Getters
  ProjectEditor& getProjectEditor() const noexcept { return mProjectEditor; }
  Project& getProject() const noexcept { return mProject; }
  int getActiveSchematicIndex() const noexcept { return mActiveSchematicIndex; }
  Schematic* getActiveSchematic() const noexcept;

  // Setters
  bool setActiveSchematicIndex(int index) noexcept;

  // General Methods
  void abortAllCommands() noexcept;

  // Operator Overloadings
  SchematicEditor& operator=(const SchematicEditor& rhs) = delete;

protected:
  void closeEvent(QCloseEvent* event);

private slots:

  // Actions
  void on_actionClose_Project_triggered();
  void on_actionRenameSheet_triggered();
  void on_actionGrid_triggered();
  void on_actionPrint_triggered();
  void on_actionPDF_Export_triggered();
  void on_actionExportAsSvg_triggered();
  void on_actionGenerateBom_triggered();
  void on_actionAddComp_Resistor_triggered();
  void on_actionAddComp_BipolarCapacitor_triggered();
  void on_actionAddComp_UnipolarCapacitor_triggered();
  void on_actionAddComp_Inductor_triggered();
  void on_actionAddComp_gnd_triggered();
  void on_actionAddComp_vcc_triggered();
  void on_actionProjectProperties_triggered();
  void on_actionUpdateLibrary_triggered();

signals:
  void activeSchematicChanged(int index);

private:
  // Private Methods
  bool graphicsViewEventHandler(QEvent* event);
  void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;
  void addSchematic() noexcept;
  void removeSchematic(int index) noexcept;
  void renameSchematic(int index) noexcept;
  QList<SI_Symbol*> getSearchCandidates() noexcept;
  QStringList getSearchToolBarCompleterList() noexcept;
  void goToSymbol(const QString& name, unsigned int index) noexcept;
  void updateComponentToolbarIcons() noexcept;
  bool useIeee315Symbols() const noexcept;

  // General Attributes
  ProjectEditor& mProjectEditor;
  Project& mProject;
  Ui::SchematicEditor* mUi;
  GraphicsView* mGraphicsView;
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;

  int mActiveSchematicIndex;

  // Docks
  SchematicPagesDock* mPagesDock;
  ErcMsgDock* mErcMsgDock;

  // Finite State Machine
  SchematicEditorFsm* mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SCHEMATICEDITOR_H
