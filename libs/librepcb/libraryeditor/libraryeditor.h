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

#ifndef LIBREPCB_LIBRARY_LIBRARYEDITOR_H
#define LIBREPCB_LIBRARY_LIBRARYEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "newelementwizard/newelementwizard.h"

#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/units/all_length_units.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStackActionGroup;
class ExclusiveActionGroup;
class TransactionalFileSystem;

namespace workspace {
class Workspace;
}

namespace library {

class Library;

namespace editor {

class EditorWidgetBase;
class LibraryOverviewWidget;

namespace Ui {
class LibraryEditor;
}

/*******************************************************************************
 *  Class LibraryEditor
 ******************************************************************************/

/**
 * @brief The LibraryEditor class
 *
 * @author ubruhin
 * @date 2015-06-28
 */
class LibraryEditor final : public QMainWindow,
                            public IF_GraphicsLayerProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryEditor()                           = delete;
  LibraryEditor(const LibraryEditor& other) = delete;
  LibraryEditor(workspace::Workspace& ws, const FilePath& libFp, bool readOnly);
  ~LibraryEditor() noexcept;

  /**
   * @copydoc librepcb::IF_GraphicsLayerProvider::getLayer()
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
   * @copydoc librepcb::IF_GraphicsLayerProvider::getAllLayers()
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
   * QObject#deleteLater() which means that this object will be deleted in the
   * Qt's event loop.
   *
   * @warning This method can be called both from within this class and from
   * outside this class (for example from the #ControlPanel). But if you call
   * this method from outside this class, you may have to delete the object
   *          yourself afterwards! In special cases, the deleteLater() mechanism
   *          could lead in fatal errors otherwise!
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
  void newElementTriggered() noexcept;
  void saveTriggered() noexcept;
  void showElementInFileExplorerTriggered() noexcept;
  void cutTriggered() noexcept;
  void copyTriggered() noexcept;
  void pasteTriggered() noexcept;
  void rotateCwTriggered() noexcept;
  void rotateCcwTriggered() noexcept;
  void removeTriggered() noexcept;
  void abortCommandTriggered() noexcept;
  void zoomInTriggered() noexcept;
  void zoomOutTriggered() noexcept;
  void zoomAllTriggered() noexcept;
  void editGridPropertiesTriggered() noexcept;
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
                                   bool            isNewElement) noexcept;
  void currentTabChanged(int index) noexcept;
  void tabCloseRequested(int index) noexcept;
  bool closeTab(int index) noexcept;
  void cursorPositionChanged(const Point& pos) noexcept;

private:  // Methods
  void setActiveEditorWidget(EditorWidgetBase* widget);
  void newLibraryElement(NewElementWizardContext::ElementType type);
  void duplicateLibraryElement(NewElementWizardContext::ElementType type,
                               const FilePath&                      fp);
  void editNewLibraryElement(NewElementWizardContext::ElementType type,
                             const FilePath&                      fp);
  void updateTabTitles() noexcept;
  void closeEvent(QCloseEvent* event) noexcept override;
  void addLayer(const QString& name, bool forceVisible = false) noexcept;

private:  // Data
  workspace::Workspace&                mWorkspace;
  bool                                 mIsOpenedReadOnly;
  QScopedPointer<Ui::LibraryEditor>    mUi;
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;
  QList<GraphicsLayer*>                mLayers;
  EditorWidgetBase*                    mCurrentEditorWidget;
  Library*                             mLibrary;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_LIBRARYEDITOR_H
