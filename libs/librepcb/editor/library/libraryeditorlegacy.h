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

#ifndef LIBREPCB_EDITOR_LIBRARYEDITORLEGACY_H
#define LIBREPCB_EDITOR_LIBRARYEDITORLEGACY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editorwidgetbase.h"
#include "newelementwizard/newelementwizard.h"

#include <librepcb/core/fileio/directorylock.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class TransactionalFileSystem;
class Workspace;

namespace editor {

class ExclusiveActionGroup;
class GraphicsLayerList;
class StandardEditorCommandHandler;
class UndoStackActionGroup;

namespace Ui {
class LibraryEditorLegacy;
}

/*******************************************************************************
 *  Class LibraryEditorLegacy
 ******************************************************************************/

/**
 * @brief The LibraryEditorLegacy class
 */
class LibraryEditorLegacy final : public QMainWindow {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryEditorLegacy() = delete;
  LibraryEditorLegacy(const LibraryEditorLegacy& other) = delete;
  LibraryEditorLegacy(Workspace& ws, Library& lib, bool readOnly);
  ~LibraryEditorLegacy() noexcept;

  bool requestClose() noexcept;
  void openComponent(const FilePath& fp) noexcept;
  void openDevice(const FilePath& fp) noexcept;
  void duplicateComponent(const FilePath& fp) noexcept;
  void duplicateDevice(const FilePath& fp) noexcept;
  void forceCloseTabs(const QSet<FilePath>& fp) noexcept;

  // Operator Overloadings
  LibraryEditorLegacy& operator=(const LibraryEditorLegacy& rhs) = delete;

signals:
  void aboutLibrePcbRequested();

private:  // GUI Event Handlers
  void closeTabIfOpen(const FilePath& fp) noexcept;
  template <typename EditWidgetType>
  void editLibraryElementTriggered(const FilePath& fp,
                                   bool isNewElement) noexcept;
  void currentTabChanged(int index) noexcept;
  void tabCloseRequested(int index) noexcept;
  bool closeTab(int index) noexcept;

private:  // Methods
  void createActions() noexcept;
  void createToolBars() noexcept;
  void createMenus() noexcept;
  EditorWidgetBase::Context createContext(bool isNewElement) noexcept;
  void setAvailableFeatures(
      const QSet<EditorWidgetBase::Feature>& features) noexcept;
  void setActiveEditorWidget(EditorWidgetBase* widget);
  void newLibraryElement(NewElementWizardContext::ElementType type);
  void duplicateLibraryElement(NewElementWizardContext::ElementType type,
                               const FilePath& fp);
  void editNewLibraryElement(NewElementWizardContext::ElementType type,
                             const FilePath& fp);
  void updateTabTitles() noexcept;
  void tabCountChanged() noexcept;
  void closeEvent(QCloseEvent* event) noexcept override;
  bool closeAllTabs(bool withNonClosable, bool askForSave) noexcept;

private:  // Data
  Workspace& mWorkspace;
  bool mIsOpenedReadOnly;
  QScopedPointer<Ui::LibraryEditorLegacy> mUi;
  QScopedPointer<StandardEditorCommandHandler> mStandardCommandHandler;
  std::unique_ptr<GraphicsLayerList> mLayers;
  EditorWidgetBase* mCurrentEditorWidget;
  Library* mLibrary;

  // Actions
  QScopedPointer<QAction> mActionAboutLibrePcb;
  QScopedPointer<QAction> mActionAboutQt;
  QScopedPointer<QAction> mActionOnlineDocumentation;
  QScopedPointer<QAction> mActionKeyboardShortcutsReference;
  QScopedPointer<QAction> mActionWebsite;
  QScopedPointer<QAction> mActionSave;
  QScopedPointer<QAction> mActionSaveAll;
  QScopedPointer<QAction> mActionCloseTab;
  QScopedPointer<QAction> mActionCloseAllTabs;
  QScopedPointer<QAction> mActionCloseWindow;
  QScopedPointer<QAction> mActionQuit;
  QScopedPointer<QAction> mActionFileManager;
  QScopedPointer<QAction> mActionRescanLibraries;
  QScopedPointer<QAction> mActionImportDxf;
  QScopedPointer<QAction> mActionImportEagleLibrary;
  QScopedPointer<QAction> mActionImportKiCadLibrary;
  QScopedPointer<QAction> mActionExportImage;
  QScopedPointer<QAction> mActionExportPdf;
  QScopedPointer<QAction> mActionPrint;
  QScopedPointer<QAction> mActionNewElement;
  QScopedPointer<QAction> mActionNextPage;
  QScopedPointer<QAction> mActionPreviousPage;
  QScopedPointer<QAction> mActionFind;
  QScopedPointer<QAction> mActionSelectAll;
  QScopedPointer<QAction> mActionGridProperties;
  QScopedPointer<QAction> mActionGridIncrease;
  QScopedPointer<QAction> mActionGridDecrease;
  QScopedPointer<QAction> mActionToggleBgImage;
  QScopedPointer<QAction> mActionZoomFit;
  QScopedPointer<QAction> mActionZoomIn;
  QScopedPointer<QAction> mActionZoomOut;
  QScopedPointer<QAction> mActionToggle3D;
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
  QScopedPointer<QAction> mActionFlipHorizontal;
  QScopedPointer<QAction> mActionFlipVertical;
  QScopedPointer<QAction> mActionMoveAlign;
  QScopedPointer<QAction> mActionSnapToGrid;
  QScopedPointer<QAction> mActionProperties;
  QScopedPointer<QAction> mActionRemove;
  QScopedPointer<QAction> mActionHelperTools;
  QScopedPointer<QAction> mActionGenerateOutline;
  QScopedPointer<QAction> mActionGenerateCourtyard;
  QScopedPointer<QAction> mActionAbort;
  QScopedPointer<QAction> mActionToolSelect;
  QScopedPointer<QAction> mActionToolLine;
  QScopedPointer<QAction> mActionToolRect;
  QScopedPointer<QAction> mActionToolPolygon;
  QScopedPointer<QAction> mActionToolCircle;
  QScopedPointer<QAction> mActionToolArc;
  QScopedPointer<QAction> mActionToolText;
  QScopedPointer<QAction> mActionToolName;
  QScopedPointer<QAction> mActionToolValue;
  QScopedPointer<QAction> mActionToolPin;
  QScopedPointer<QAction> mActionToolSmtPadStandard;
  QScopedPointer<QAction> mActionToolThtPad;
  QScopedPointer<QAction> mActionToolSpecialPadThermal;
  QScopedPointer<QAction> mActionToolSpecialPadBga;
  QScopedPointer<QAction> mActionToolSpecialPadEdgeConnector;
  QScopedPointer<QAction> mActionToolSpecialPadTest;
  QScopedPointer<QAction> mActionToolSpecialPadLocalFiducial;
  QScopedPointer<QAction> mActionToolSpecialPadGlobalFiducial;
  QScopedPointer<QAction> mActionToolZone;
  QScopedPointer<QAction> mActionToolHole;
  QScopedPointer<QAction> mActionToolMeasure;
  QScopedPointer<QAction> mActionReNumberPads;

  // Action groups
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;

  // Toolbars
  QScopedPointer<QToolBar> mToolBarFile;
  QScopedPointer<QToolBar> mToolBarEdit;
  QScopedPointer<QToolBar> mToolBarView;
  QScopedPointer<QToolBar> mToolBarCommand;
  QScopedPointer<QToolBar> mToolBarTools;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
