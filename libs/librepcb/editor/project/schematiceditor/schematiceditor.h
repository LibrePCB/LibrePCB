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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITOR_H
#define LIBREPCB_EDITOR_SCHEMATICEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../dialogs/graphicsexportdialog.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "ui_schematiceditor.h"

#include <librepcb/core/serialization/fileformatmigration.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;
class SI_Symbol;
class Schematic;

namespace editor {

class ErcMsgDock;
class ExclusiveActionGroup;
class GraphicsView;
class ProjectEditor;
class SchematicEditorFsm;
class SchematicPagesDock;
class SearchToolBar;
class StandardEditorCommandHandler;
class ToolBarProxy;
class UndoStackActionGroup;

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
  explicit SchematicEditor(
      ProjectEditor& projectEditor, Project& project,
      const tl::optional<QList<FileFormatMigration::Message>>& upgradeMessages);
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
  void abortBlockingToolsInOtherEditors() noexcept;

  // Operator Overloadings
  SchematicEditor& operator=(const SchematicEditor& rhs) = delete;

protected:
  void closeEvent(QCloseEvent* event);

signals:
  void activeSchematicChanged(int index);

private:
  // Private Methods
  void createActions() noexcept;
  void createToolBars() noexcept;
  void createDockWidgets() noexcept;
  void createMenus() noexcept;
  bool graphicsViewEventHandler(QEvent* event);
  void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;
  void addSchematic() noexcept;
  void removeSchematic(int index) noexcept;
  void renameSchematic(int index) noexcept;
  QList<SI_Symbol*> getSearchCandidates() noexcept;
  QStringList getSearchToolBarCompleterList() noexcept;
  void goToSymbol(const QString& name, int index) noexcept;
  void updateEmptySchematicMessage() noexcept;
  void updateComponentToolbarIcons() noexcept;
  void setGridProperties(const PositiveLength& interval, const LengthUnit& unit,
                         Theme::GridStyle style,
                         bool applyToSchematics) noexcept;
  void execGridPropertiesDialog() noexcept;
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  bool useIeee315Symbols() const noexcept;
  void showUpgradeMessages(
      QList<FileFormatMigration::Message> messages) noexcept;

  // General Attributes
  ProjectEditor& mProjectEditor;
  Project& mProject;
  QScopedPointer<Ui::SchematicEditor> mUi;
  QScopedPointer<ToolBarProxy> mCommandToolBarProxy;
  QScopedPointer<StandardEditorCommandHandler> mStandardCommandHandler;
  int mActiveSchematicIndex;
  QScopedPointer<SchematicEditorFsm> mFsm;

  // Actions
  QScopedPointer<QAction> mActionAboutLibrePcb;
  QScopedPointer<QAction> mActionAboutQt;
  QScopedPointer<QAction> mActionOnlineDocumentation;
  QScopedPointer<QAction> mActionKeyboardShortcutsReference;
  QScopedPointer<QAction> mActionWebsite;
  QScopedPointer<QAction> mActionSaveProject;
  QScopedPointer<QAction> mActionCloseProject;
  QScopedPointer<QAction> mActionCloseWindow;
  QScopedPointer<QAction> mActionQuit;
  QScopedPointer<QAction> mActionFileManager;
  QScopedPointer<QAction> mActionBoardEditor;
  QScopedPointer<QAction> mActionControlPanel;
  QScopedPointer<QAction> mActionProjectProperties;
  QScopedPointer<QAction> mActionProjectSettings;
  QScopedPointer<QAction> mActionNetClasses;
  QScopedPointer<QAction> mActionUpdateLibrary;
  QScopedPointer<QAction> mActionExportLppz;
  QScopedPointer<QAction> mActionExportImage;
  QScopedPointer<QAction> mActionExportPdf;
  QScopedPointer<QAction> mActionPrint;
  QScopedPointer<QAction> mActionGenerateBom;
  QScopedPointer<QAction> mActionOrderPcb;
  QScopedPointer<QAction> mActionNewSheet;
  QScopedPointer<QAction> mActionRenameSheet;
  QScopedPointer<QAction> mActionRemoveSheet;
  QScopedPointer<QAction> mActionNextPage;
  QScopedPointer<QAction> mActionPreviousPage;
  QScopedPointer<QAction> mActionFind;
  QScopedPointer<QAction> mActionFindNext;
  QScopedPointer<QAction> mActionFindPrevious;
  QScopedPointer<QAction> mActionSelectAll;
  QScopedPointer<QAction> mActionGridProperties;
  QScopedPointer<QAction> mActionGridIncrease;
  QScopedPointer<QAction> mActionGridDecrease;
  QScopedPointer<QAction> mActionZoomFit;
  QScopedPointer<QAction> mActionZoomIn;
  QScopedPointer<QAction> mActionZoomOut;
  QScopedPointer<QAction> mActionUndo;
  QScopedPointer<QAction> mActionRedo;
  QScopedPointer<QAction> mActionCut;
  QScopedPointer<QAction> mActionCopy;
  QScopedPointer<QAction> mActionPaste;
  QScopedPointer<QAction> mActionMoveLeft;
  QScopedPointer<QAction> mActionMoveRight;
  QScopedPointer<QAction> mActionMoveUp;
  QScopedPointer<QAction> mActionMoveDown;
  QScopedPointer<QAction> mActionRotateCcw;
  QScopedPointer<QAction> mActionRotateCw;
  QScopedPointer<QAction> mActionMirrorHorizontal;
  QScopedPointer<QAction> mActionMirrorVertical;
  QScopedPointer<QAction> mActionResetAllTexts;
  QScopedPointer<QAction> mActionProperties;
  QScopedPointer<QAction> mActionRemove;
  QScopedPointer<QAction> mActionAbort;
  QScopedPointer<QAction> mActionToolSelect;
  QScopedPointer<QAction> mActionToolWire;
  QScopedPointer<QAction> mActionToolNetLabel;
  QScopedPointer<QAction> mActionToolPolygon;
  QScopedPointer<QAction> mActionToolText;
  QScopedPointer<QAction> mActionToolComponent;
  QScopedPointer<QAction> mActionToolMeasure;
  QScopedPointer<QAction> mActionComponentResistor;
  QScopedPointer<QAction> mActionComponentInductor;
  QScopedPointer<QAction> mActionComponentCapacitorBipolar;
  QScopedPointer<QAction> mActionComponentCapacitorUnipolar;
  QScopedPointer<QAction> mActionComponentGnd;
  QScopedPointer<QAction> mActionComponentVcc;
  QScopedPointer<QAction> mActionDockPages;
  QScopedPointer<QAction> mActionDockErc;

  // Action groups
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;

  // Toolbars
  QScopedPointer<QToolBar> mToolBarFile;
  QScopedPointer<QToolBar> mToolBarEdit;
  QScopedPointer<QToolBar> mToolBarView;
  QScopedPointer<SearchToolBar> mToolBarSearch;
  QScopedPointer<QToolBar> mToolBarCommand;
  QScopedPointer<QToolBar> mToolBarTools;
  QScopedPointer<QToolBar> mToolBarComponents;

  // Docks
  QScopedPointer<SchematicPagesDock> mDockPages;
  QScopedPointer<ErcMsgDock> mDockErc;

  // Connections
  QVector<QMetaObject::Connection> mSchematicConnections;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
