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

#ifndef LIBREPCB_EDITOR_LIBRARYEDITOR_H
#define LIBREPCB_EDITOR_LIBRARYEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editorwidgetbase.h"
#include "newelementwizard/newelementwizard.h"

#include <librepcb/core/fileio/directorylock.h>
#include <librepcb/core/graphics/graphicslayer.h>

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
class LibraryOverviewWidget;
class SearchToolBar;
class StandardEditorCommandHandler;
class UndoStackActionGroup;

namespace Ui {
class LibraryEditor;
}

/*******************************************************************************
 *  Class LibraryEditor
 ******************************************************************************/

/**
 * @brief The LibraryEditor class
 */
class LibraryEditor final : public QMainWindow,
                            public IF_GraphicsLayerProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryEditor() = delete;
  LibraryEditor(const LibraryEditor& other) = delete;
  LibraryEditor(Workspace& ws, const FilePath& libFp, bool readOnly);
  ~LibraryEditor() noexcept;

  /**
   * @copydoc ::librepcb::IF_GraphicsLayerProvider::getLayer()
   */
  GraphicsLayer* getLayer(const QString& name) const noexcept override {
    foreach (GraphicsLayer* layer, mLayers) {
      if (layer->getName() == name) {
        return layer;
      }
    }
    return nullptr;
  }

  /**
   * @copydoc ::librepcb::IF_GraphicsLayerProvider::getAllLayers()
   */
  QList<GraphicsLayer*> getAllLayers() const noexcept override {
    return mLayers;
  }

  /**
   * @brief Close the library editor (this will destroy this object!)
   *
   * If there are unsaved changes to the library, this method will ask the user
   * whether the changes should be saved or not. If the user clicks on "cancel"
   * or the library could not be saved successfully, this method will return
   * false. If there was no such error, this method will call
   * QObject::deleteLater() which means that this object will be deleted in the
   * Qt's event loop.
   *
   * @warning This method can be called both from within this class and from
   * outside this class (for example from the control panel). But if you call
   * this method from outside this class, you may have to delete the object
   * yourself afterwards! In special cases, the deleteLater() mechanism could
   * lead in fatal errors otherwise!
   *
   * @param askForSave    If true and there are unsaved changes, this method
   * shows a message box to ask whether the library should be saved or not. If
   * false, the library will NOT be saved.
   *
   * @return true on success (editor closed), false on failure (editor stays
   * open)
   */
  bool closeAndDestroy(bool askForSave) noexcept;

  // Operator Overloadings
  LibraryEditor& operator=(const LibraryEditor& rhs) = delete;

private:  // GUI Event Handlers
  void newComponentCategoryTriggered() noexcept;
  void newPackageCategoryTriggered() noexcept;
  void newSymbolTriggered() noexcept;
  void newPackageTriggered() noexcept;
  void newComponentTriggered() noexcept;
  void newDeviceTriggered() noexcept;
  void editComponentCategoryTriggered(const FilePath& fp) noexcept;
  void editPackageCategoryTriggered(const FilePath& fp) noexcept;
  void editSymbolTriggered(const FilePath& fp) noexcept;
  void editPackageTriggered(const FilePath& fp) noexcept;
  void editComponentTriggered(const FilePath& fp) noexcept;
  void editDeviceTriggered(const FilePath& fp) noexcept;
  void duplicateComponentCategoryTriggered(const FilePath& fp) noexcept;
  void duplicatePackageCategoryTriggered(const FilePath& fp) noexcept;
  void duplicateSymbolTriggered(const FilePath& fp) noexcept;
  void duplicatePackageTriggered(const FilePath& fp) noexcept;
  void duplicateComponentTriggered(const FilePath& fp) noexcept;
  void duplicateDeviceTriggered(const FilePath& fp) noexcept;
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
  void keyPressEvent(QKeyEvent* event) noexcept override;
  void closeEvent(QCloseEvent* event) noexcept override;
  bool closeAllTabs(bool withNonClosable, bool askForSave) noexcept;
  void addLayer(const QString& name, bool forceVisible = false) noexcept;

private:  // Data
  Workspace& mWorkspace;
  bool mIsOpenedReadOnly;
  QScopedPointer<Ui::LibraryEditor> mUi;
  QScopedPointer<StandardEditorCommandHandler> mStandardCommandHandler;
  QList<GraphicsLayer*> mLayers;
  EditorWidgetBase* mCurrentEditorWidget;
  Library* mLibrary;

  // Actions
  QScopedPointer<QAction> mActionAboutLibrePcb;
  QScopedPointer<QAction> mActionAboutQt;
  QScopedPointer<QAction> mActionOnlineDocumentation;
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
  QScopedPointer<QAction> mActionFlipHorizontal;
  QScopedPointer<QAction> mActionFlipVertical;
  QScopedPointer<QAction> mActionSnapToGrid;
  QScopedPointer<QAction> mActionProperties;
  QScopedPointer<QAction> mActionRemove;
  QScopedPointer<QAction> mActionAbort;
  QScopedPointer<QAction> mActionToolSelect;
  QScopedPointer<QAction> mActionToolLine;
  QScopedPointer<QAction> mActionToolRect;
  QScopedPointer<QAction> mActionToolPolygon;
  QScopedPointer<QAction> mActionToolCircle;
  QScopedPointer<QAction> mActionToolText;
  QScopedPointer<QAction> mActionToolName;
  QScopedPointer<QAction> mActionToolValue;
  QScopedPointer<QAction> mActionToolPin;
  QScopedPointer<QAction> mActionToolSmtPad;
  QScopedPointer<QAction> mActionToolThtPad;
  QScopedPointer<QAction> mActionToolHole;

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
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
