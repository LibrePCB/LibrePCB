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
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class Workspace;

namespace editor {

class GuiApplication;
class LibraryEditorLegacy;
class LibraryEditorTab;
class UndoStack;

/*******************************************************************************
 *  Class LibraryEditor
 ******************************************************************************/

/**
 * @brief The LibraryEditor class
 */
class LibraryEditor final : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<LibraryEditor> onUiDataChanged;

  // Constructors / Destructor
  LibraryEditor() = delete;
  LibraryEditor(const LibraryEditor& other) = delete;
  LibraryEditor(GuiApplication& app, std::unique_ptr<Library> lib, int uiIndex,
                QObject* parent = nullptr) noexcept;
  ~LibraryEditor() noexcept;

  // General Methods
  GuiApplication& getApp() noexcept { return mApp; }
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  FilePath getFilePath() const noexcept;
  Library& getLibrary() noexcept { return *mLibrary; }
  UndoStack& getUndoStack() noexcept { return *mUndoStack; }
  bool isWritable() const noexcept;
  int getUiIndex() const noexcept { return mUiIndex; }
  void setUiIndex(int index) noexcept;
  ui::LibraryData getUiData() const noexcept;
  void setUiData(const ui::LibraryData& data) noexcept;

  /**
   * @brief Request to close the library
   *
   * If there are unsaved changes to the library, this method will ask the user
   * whether the changes should be saved or not. If the user clicks on "cancel"
   * or the library could not be saved successfully, this method will return
   * false. If there were no unsaved changes or they were successfully saved,
   * the method returns true.
   *
   * @retval true   Library is safe to be closed.
   * @retval false  Library still has unsaved changes.
   */
  bool requestClose() noexcept;

  bool hasUnsavedChanges() const noexcept;

  /**
   * @brief Set the flag that manual modifications (no undo stack) are made
   */
  void setManualModificationsMade() noexcept;

  /**
   * @brief Save the library to the harddisc
   *
   * @return true on success, false on failure
   */
  bool save() noexcept;

  void registerTab(LibraryEditorTab& tab) noexcept;
  void unregisterTab(LibraryEditorTab& tab) noexcept;

  void forceClosingTabs(const QSet<FilePath>& fp) noexcept;

  // Legacy editors
  void openLegacySymbolEditor(const FilePath& fp) noexcept;
  void openLegacyPackageEditor(const FilePath& fp) noexcept;
  void openLegacyComponentEditor(const FilePath& fp) noexcept;
  void openLegacyDeviceEditor(const FilePath& fp) noexcept;
  void duplicateInLegacySymbolEditor(const FilePath& fp) noexcept;
  void duplicateInLegacyPackageEditor(const FilePath& fp) noexcept;
  void duplicateInLegacyComponentEditor(const FilePath& fp) noexcept;
  void duplicateInLegacyDeviceEditor(const FilePath& fp) noexcept;

  // Operator Overloadings
  LibraryEditor& operator=(const LibraryEditor& rhs) = delete;

signals:
  void uiIndexChanged();
  void manualModificationsMade();
  void aboutToBeDestroyed();

private:
  GuiApplication& mApp;
  Workspace& mWorkspace;
  std::unique_ptr<Library> mLibrary;
  int mUiIndex;
  std::unique_ptr<UndoStack> mUndoStack;

  /// Modifications bypassing the undo stack
  bool mManualModificationsMade;

  QVector<QPointer<LibraryEditorTab>> mRegisteredTabs;

  std::unique_ptr<LibraryEditorLegacy> mLegacyEditor;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
