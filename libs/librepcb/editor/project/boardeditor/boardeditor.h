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

#ifndef LIBREPCB_EDITOR_BOARDEDITOR_H
#define LIBREPCB_EDITOR_BOARDEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../dialogs/graphicsexportdialog.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "ui_boardeditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class Project;

namespace editor {

class BoardDesignRuleCheckMessagesDock;
class BoardEditorFsm;
class BoardLayersDock;
class ErcMsgDock;
class ExclusiveActionGroup;
class GraphicsView;
class ProjectEditor;
class SearchToolBar;
class StandardEditorCommandHandler;
class ToolBarProxy;
class UndoStackActionGroup;
class UnplacedComponentsDock;

namespace Ui {
class BoardEditor;
}

/*******************************************************************************
 *  Class BoardEditor
 ******************************************************************************/

/**
 * @brief The BoardEditor class
 */
class BoardEditor final : public QMainWindow,
                          public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditor() = delete;
  BoardEditor(const BoardEditor& other) = delete;
  explicit BoardEditor(ProjectEditor& projectEditor, Project& project);
  ~BoardEditor();

  // Getters
  ProjectEditor& getProjectEditor() const noexcept { return mProjectEditor; }
  Project& getProject() const noexcept { return mProject; }
  Board* getActiveBoard() const noexcept { return mActiveBoard.data(); }

  // Setters
  bool setActiveBoardIndex(int index) noexcept;

  // General Methods
  void abortAllCommands() noexcept;
  void abortBlockingToolsInOtherEditors() noexcept;

  // Operator Overloadings
  BoardEditor& operator=(const BoardEditor& rhs) = delete;

protected:
  void closeEvent(QCloseEvent* event);

public slots:

  void boardAdded(int newIndex);
  void boardRemoved(int oldIndex);

private slots:

  // Actions
  void on_tabBar_currentChanged(int index);
  void on_lblUnplacedComponentsNote_linkActivated();

private:
  // Private Methods
  void createActions() noexcept;
  void createToolBars() noexcept;
  void createDockWidgets() noexcept;
  void createMenus() noexcept;
  void updateBoardActionGroup() noexcept;
  bool graphicsViewEventHandler(QEvent* event);
  void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;
  void unplacedComponentsCountChanged(int count) noexcept;
  void runDrcNonInteractive() noexcept;
  void updateBoardDrcMessages(
      const Board& board,
      const QList<BoardDesignRuleCheckMessage>& messages) noexcept;
  void highlightDrcMessage(const BoardDesignRuleCheckMessage& msg,
                           bool zoomTo) noexcept;
  void clearDrcMarker() noexcept;
  QList<BI_Device*> getSearchCandidates() noexcept;
  QStringList getSearchToolBarCompleterList() noexcept;
  void goToDevice(const QString& name, int index) noexcept;
  void newBoard() noexcept;
  void copyBoard() noexcept;
  void removeBoard() noexcept;
  void setGridProperties(const GridProperties& grid,
                         bool applyToBoard) noexcept;
  void execGridPropertiesDialog() noexcept;
  void execDesignRulesDialog() noexcept;
  void execDesignRuleCheckDialog() noexcept;
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;

  // General Attributes
  ProjectEditor& mProjectEditor;
  Project& mProject;
  QScopedPointer<Ui::BoardEditor> mUi;
  QScopedPointer<ToolBarProxy> mCommandToolBarProxy;
  QScopedPointer<StandardEditorCommandHandler> mStandardCommandHandler;

  // DRC
  BoardDesignRuleCheck::Options mDrcOptions;
  QHash<Uuid, QList<BoardDesignRuleCheckMessage>>
      mDrcMessages;  ///< Key: Board UUID
  QScopedPointer<QGraphicsPathItem> mDrcLocationGraphicsItem;

  // Misc
  QPointer<Board> mActiveBoard;
  QScopedPointer<BoardEditorFsm> mFsm;

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
  QScopedPointer<QAction> mActionSchematicEditor;
  QScopedPointer<QAction> mActionControlPanel;
  QScopedPointer<QAction> mActionProjectProperties;
  QScopedPointer<QAction> mActionProjectSettings;
  QScopedPointer<QAction> mActionNetClasses;
  QScopedPointer<QAction> mActionUpdateLibrary;
  QScopedPointer<QAction> mActionLayerStack;
  QScopedPointer<QAction> mActionDesignRules;
  QScopedPointer<QAction> mActionDesignRuleCheck;
  QScopedPointer<QAction> mActionImportDxf;
  QScopedPointer<QAction> mActionExportLppz;
  QScopedPointer<QAction> mActionExportImage;
  QScopedPointer<QAction> mActionExportPdf;
  QScopedPointer<QAction> mActionPrint;
  QScopedPointer<QAction> mActionGenerateBom;
  QScopedPointer<QAction> mActionGenerateFabricationData;
  QScopedPointer<QAction> mActionGeneratePickPlace;
  QScopedPointer<QAction> mActionOrderPcb;
  QScopedPointer<QAction> mActionNewBoard;
  QScopedPointer<QAction> mActionCopyBoard;
  QScopedPointer<QAction> mActionRemoveBoard;
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
  QScopedPointer<QAction> mActionFlipHorizontal;
  QScopedPointer<QAction> mActionFlipVertical;
  QScopedPointer<QAction> mActionSnapToGrid;
  QScopedPointer<QAction> mActionResetAllTexts;
  QScopedPointer<QAction> mActionProperties;
  QScopedPointer<QAction> mActionRemove;
  QScopedPointer<QAction> mActionShowPlanes;
  QScopedPointer<QAction> mActionHidePlanes;
  QScopedPointer<QAction> mActionRebuildPlanes;
  QScopedPointer<QAction> mActionAbort;
  QScopedPointer<QAction> mActionToolSelect;
  QScopedPointer<QAction> mActionToolTrace;
  QScopedPointer<QAction> mActionToolVia;
  QScopedPointer<QAction> mActionToolPolygon;
  QScopedPointer<QAction> mActionToolText;
  QScopedPointer<QAction> mActionToolPlane;
  QScopedPointer<QAction> mActionToolHole;
  QScopedPointer<QAction> mActionToolMeasure;
  QScopedPointer<QAction> mActionDockErc;
  QScopedPointer<QAction> mActionDockDrc;
  QScopedPointer<QAction> mActionDockLayers;
  QScopedPointer<QAction> mActionDockPlaceDevices;

  // Action groups
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;
  QScopedPointer<QActionGroup> mBoardActionGroup;

  // Toolbars
  QScopedPointer<QToolBar> mToolBarFile;
  QScopedPointer<QToolBar> mToolBarEdit;
  QScopedPointer<QToolBar> mToolBarView;
  QScopedPointer<SearchToolBar> mToolBarSearch;
  QScopedPointer<QToolBar> mToolBarCommand;
  QScopedPointer<QToolBar> mToolBarTools;

  // Docks
  QScopedPointer<UnplacedComponentsDock> mDockUnplacedComponents;
  QScopedPointer<BoardLayersDock> mDockLayers;
  QScopedPointer<ErcMsgDock> mDockErc;
  QScopedPointer<BoardDesignRuleCheckMessagesDock> mDockDrc;

  // Menus
  QPointer<QMenu> mMenuBoard;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
