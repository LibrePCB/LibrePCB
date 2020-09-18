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

#ifndef LIBREPCB_PROJECT_PROJECTEDITOR_H
#define LIBREPCB_PROJECT_PROJECTEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/fileio/serializableobject.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
class QMainWindow;

namespace librepcb {

class UndoStack;
class LengthUnit;

namespace workspace {
class Workspace;
}

namespace project {

class Project;

namespace editor {

class SchematicEditor;
class BoardEditor;

/*******************************************************************************
 *  Class ProjectEditor
 ******************************************************************************/

/**
 * @brief The ProjectEditor class
 */
class ProjectEditor final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectEditor() = delete;
  ProjectEditor(const Project& other) = delete;

  /**
   * @brief The constructor
   */
  ProjectEditor(workspace::Workspace& workspace, Project& project);

  /**
   * @brief The destructor
   */
  ~ProjectEditor() noexcept;

  // Getters: General

  workspace::Workspace& getWorkspace() const noexcept { return mWorkspace; }
  Project& getProject() const noexcept { return mProject; }
  const LengthUnit& getDefaultLengthUnit() const noexcept;

  /**
   * @brief Get a reference to the undo stack of the project
   *
   * @return A reference to the UndoStack object
   */
  UndoStack& getUndoStack() const noexcept { return *mUndoStack; }

  // General Methods

  /**
   * @brief Inform the editor that a project related window is about to close
   *
   * The project must be closed and destroyed automatically after the last
   * opened window of the project is closed, because without a window the user
   * is no longer able to close the project himself. So, every project related
   * window have to "ask" the ::librepcb::project::editor::ProjectEditor object
   * whether it is allowed to close or not. If the last opened window wants to
   * close, the editor will first ask the user if unsaved changes should be
   * written to the harddisc. Only if the user accepts this question and the
   * project is saved successfully, the method will return true to allow the
   * last window to close. Then it will also close the whole project.
   *
   * @param window    A reference to the window which is about to close
   *
   * @return true if the window can be closed, false if closing the window is
   * denied
   */
  bool windowIsAboutToClose(QMainWindow& window) noexcept;

  // Operator Overloadings
  ProjectEditor& operator=(const Project& rhs) = delete;

public slots:

  /**
   * @brief Open the schematic and/or the board editor window
   *
   * Which editors this will open depends on whether the project has schematics
   * and/or boards. If there aren't any boards or schematics, the schematic
   * editor will be shown anyway (otherwise the whole project editor would be
   * invisible).
   */
  void showAllRequiredEditors() noexcept;

  /**
   * @brief Open the schematic editor window and bring it to the front
   */
  void showSchematicEditor() noexcept;

  /**
   * @brief Open the board editor window and bring it to the front
   */
  void showBoardEditor() noexcept;

  /**
   * @brief Execute the project settings dialog (blocking!)
   *
   * @param parent    parent widget of the settings dialog (optional)
   */
  void execProjectSettingsDialog(QWidget* parent = nullptr) noexcept;

  /**
   * @brief Execute the netclasses editor dialog (blocking!)
   *
   * @param parent    parent widget of the dialog (optional)
   */
  void execNetClassesEditorDialog(QWidget* parent = nullptr) noexcept;

  /**
   * @brief Execute the *.lppz export dialog (blocking!)
   *
   * @param parent    parent widget of the dialog (optional)
   */
  void execLppzExportDialog(QWidget* parent = nullptr) noexcept;

  /**
   * @brief Save the whole project to the harddisc
   *
   * @note The whole save procedere is described in @ref doc_project_save.
   *
   * @return true on success, false on failure
   */
  bool saveProject() noexcept;

  /**
   * @brief Make a automatic backup of the project (save to temporary files)
   *
   * @note The whole save procedere is described in @ref doc_project_save.
   *
   * @return true on success, false on failure
   */
  bool autosaveProject() noexcept;

  /**
   * @brief Close the project (this will destroy this object!)
   *
   * If there are unsaved changes to the project, this method will ask the user
   * whether the changes should be saved or not. If the user clicks on "cancel"
   * or the project could not be saved successfully, this method will return
   * false. If there was no such error, this method will call
   * QObject::deleteLater() which means that this object will be deleted in the
   * Qt's event loop.
   *
   * @warning This method can be called both from within this class and from
   *          outside this class (for example from the
   *          ::librepcb::application::ControlPanel). But if you call this
   *          method from outside this class, you may have to delete the object
   *          yourself afterwards! In special cases, the deleteLater() mechanism
   *          could lead in fatal errors otherwise!
   *
   * @param askForSave    If true and there are unsaved changes, this method
   * shows a message box to ask whether the project should be saved or not. If
   * false, the project will NOT be saved.
   * @param msgBoxParent  Here you can specify a parent window for the message
   * box
   *
   * @return true on success (project closed), false on failure (project stays
   * open)
   */
  bool closeAndDestroy(bool askForSave, QWidget* msgBoxParent = 0) noexcept;

signals:

  void showControlPanelClicked();
  void openProjectLibraryUpdaterClicked(const FilePath& fp);
  void projectEditorClosed();

private:  // Methods
  int getCountOfVisibleEditorWindows() const noexcept;

private:  // Data
  workspace::Workspace& mWorkspace;
  Project& mProject;
  QTimer mAutoSaveTimer;  ///< the timer for the periodically automatic saving
                          ///< functionality (see also @ref doc_project_save)
  UndoStack* mUndoStack;  ///< See @ref doc_project_undostack
  SchematicEditor* mSchematicEditor;  ///< The schematic editor (GUI)
  BoardEditor* mBoardEditor;  ///< The board editor (GUI)
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_PROJECTEDITOR_H
