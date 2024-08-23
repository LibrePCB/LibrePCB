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

#ifndef LIBREPCB_EDITOR_EDITORCOMMANDSET_H
#define LIBREPCB_EDITOR_EDITORCOMMANDSET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editorcommand.h"
#include "editorcommandcategory.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class EditorCommandSet
 ******************************************************************************/

/**
 * @brief Collection of all commands across all editors
 *
 * @see ::librepcb::editor::EditorCommand
 * @see https://en.wikipedia.org/wiki/Table_of_keyboard_shortcuts
 * @see https://librepcb.discourse.group/t/hotkeys-anyone/229
 */
class EditorCommandSet final {
  Q_DECLARE_TR_FUNCTIONS(EditorCommandSet)

private:
  EditorCommandCategory categoryRoot{"categoryRoot", "", false};

  EditorCommandSet() noexcept {}
  ~EditorCommandSet() noexcept {}

public:
  // General Methods
  static EditorCommandSet& instance() noexcept {
    static EditorCommandSet obj;
    return obj;
  }
  void updateTranslations() noexcept {
    // Required to be called when the application's locale has changed.
    foreach (EditorCommandCategory* cat, getCategories()) {
      cat->updateTranslations();
      foreach (EditorCommand* cmd, getCommands(cat)) {
        cmd->updateTranslations();
      }
    }
  }
  QList<EditorCommandCategory*> getCategories() noexcept {
    return categoryRoot.findChildren<EditorCommandCategory*>();
  }
  QList<EditorCommand*> getCommands(
      const EditorCommandCategory* category) noexcept {
    Q_ASSERT(category);
    return category->findChildren<EditorCommand*>();
  }

  EditorCommandCategory categoryEditor{"categoryEditor", QT_TR_NOOP("Editor"),
                                       true, &categoryRoot};
  EditorCommand itemNew{
      "item_new",  // clang-format break
      QT_TR_NOOP("New"),
      QT_TR_NOOP("Add a new item"),
      ":/img/actions/new.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_N)},
      &categoryEditor,
  };
  EditorCommand itemOpen{
      "item_open",  // clang-format break
      QT_TR_NOOP("Open"),
      QT_TR_NOOP("Open the selected item(s)"),
      ":/img/actions/open.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Return)},
      &categoryEditor,
  };
  EditorCommand save{
      "save",  // clang-format break
      QT_TR_NOOP("Save"),
      QT_TR_NOOP("Save changes to filesystem"),
      ":/img/actions/save.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_S)},
      &categoryEditor,
  };
  EditorCommand saveAll{
      "save_all",  // clang-format break
      QT_TR_NOOP("Save All"),
      QT_TR_NOOP("Save all elements to filesystem"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S)},
      &categoryEditor,
  };
  EditorCommand selectAll{
      "select_all",  // clang-format break
      QT_TR_NOOP("Select All"),
      QT_TR_NOOP("Select all visible objects"),
      ":/img/actions/select_all.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_A)},
      &categoryEditor,
  };
  EditorCommand find{
      "find",  // clang-format break
      QT_TR_NOOP("Find"),
      QT_TR_NOOP("Find or filter objects"),
      ":/img/actions/search.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_F)},
      &categoryEditor,
  };
  EditorCommand findNext{
      "find_next",  // clang-format break
      QT_TR_NOOP("Find Next"),
      QT_TR_NOOP("Go to the next found object"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F3)},
      &categoryEditor,
  };
  EditorCommand findPrevious{
      "find_previous",  // clang-format break
      QT_TR_NOOP("Find Previous"),
      QT_TR_NOOP("Go to the previous found object"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::SHIFT | Qt::Key_F3)},
      &categoryEditor,
  };
  EditorCommand fileManager{
      "file_manager",  // clang-format break
      QT_TR_NOOP("Show in File Manager"),
      QT_TR_NOOP("Open the directory in the file manager"),
      ":/img/places/folder.png",
      EditorCommand::Flags(),
      {},
      &categoryEditor,
  };
  EditorCommand controlPanel{
      "control_panel",  // clang-format break
      QT_TR_NOOP("Control Panel"),
      QT_TR_NOOP("Bring the control panel window to front"),
      ":/img/actions/home.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Home)},
      &categoryEditor,
  };
  EditorCommand workspaceSwitch{
      "workspace_switch",  // clang-format break
      QT_TR_NOOP("Switch Workspace"),
      QT_TR_NOOP("Choose another workspace to open"),
      QString(),
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryEditor,
  };
  EditorCommand workspaceSettings{
      "workspace_settings",  // clang-format break
      QT_TR_NOOP("Workspace Settings"),
      QT_TR_NOOP("Open the workspace settings dialog"),
      ":/img/actions/settings.png",
      EditorCommand::Flag::OpensPopup | EditorCommand::Flag::PreferencesRole,
      {QKeySequence(Qt::CTRL | Qt::Key_Comma)},
      &categoryEditor,
  };
  EditorCommand workspaceLibrariesRescan{
      "workspace_libraries_rescan",  // clang-format break
      QT_TR_NOOP("Rescan Libraries"),
      QT_TR_NOOP("Scan all workspace libraries to update the cache"),
      ":/img/actions/refresh.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F5)},
      &categoryEditor,
  };
  EditorCommand libraryManager{
      "library_manager",  // clang-format break
      QT_TR_NOOP("Library Manager"),
      QT_TR_NOOP("Open the library manager window"),
      ":/img/library/package.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_M)},
      &categoryEditor,
  };
  EditorCommand libraryElementNew{
      "library_element_new",  // clang-format break
      QT_TR_NOOP("New Library Element"),
      QT_TR_NOOP("Create a new library element"),
      ":/img/actions/new.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_N)},
      &categoryEditor,
  };
  EditorCommand libraryElementDuplicate{
      "library_element_duplicate",  // clang-format break
      QT_TR_NOOP("Duplicate"),
      QT_TR_NOOP("Create a new element by duplicating this one"),
      ":/img/actions/clone.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_D)},
      &categoryEditor,
  };
  EditorCommand projectNew{
      "project_new",  // clang-format break
      QT_TR_NOOP("New Project"),
      QT_TR_NOOP("Create a new project"),
      ":/img/actions/new.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_N)},
      &categoryEditor,
  };
  EditorCommand projectOpen{
      "project_open",  // clang-format break
      QT_TR_NOOP("Open Project"),
      QT_TR_NOOP("Open an existing project"),
      ":/img/actions/open.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_O)},
      &categoryEditor,
  };
  EditorCommand projectSave{
      "project_save",  // clang-format break
      QT_TR_NOOP("Save Project"),
      QT_TR_NOOP("Save the currently opened project"),
      ":/img/actions/save.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_S)},
      &categoryEditor,
  };
  EditorCommand projectSetup{
      "project_setup",  // clang-format break
      QT_TR_NOOP("Project Setup"),
      QT_TR_NOOP("View/modify the project setup"),
      ":/img/actions/settings.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F6)},
      &categoryEditor,
  };
  EditorCommand gridProperties{
      "grid_properties",  // clang-format break
      QT_TR_NOOP("Grid Properties"),
      QT_TR_NOOP("View/modify the grid properties"),
      ":/img/actions/grid.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F4)},
      &categoryEditor,
  };
  EditorCommand boardSetup{
      "board_setup",  // clang-format break
      QT_TR_NOOP("Board Setup"),
      QT_TR_NOOP("View/modify the board setup"),
      ":/img/actions/settings.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F7)},
      &categoryEditor,
  };
  EditorCommand runQuickCheck{
      "run_quick_check",  // clang-format break
      QT_TR_NOOP("Run Quick Check"),
      QT_TR_NOOP("Run only the most important copper checks from the DRC"),
      ":/img/actions/quick_check.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::SHIFT | Qt::Key_F8)},
      &categoryEditor,
  };
  EditorCommand runDesignRuleCheck{
      "run_design_rule_check",  // clang-format break
      QT_TR_NOOP("Run Design Rule Check"),
      QT_TR_NOOP("Run the design rule check (DRC)"),
      ":/img/actions/drc.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F8)},
      &categoryEditor,
  };
  EditorCommand projectLibraryUpdate{
      "project_library_update",  // clang-format break
      QT_TR_NOOP("Update Project Library"),
      QT_TR_NOOP(
          "Update the project's library elements from workspace libraries"),
      ":/img/actions/refresh.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_F5)},
      &categoryEditor,
  };
  EditorCommand schematicEditor{
      "schematic_editor",  // clang-format break
      QT_TR_NOOP("Schematic Editor"),
      QT_TR_NOOP("Bring the schematic editor window to front"),
      ":/img/actions/schematic.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S)},
      &categoryEditor,
  };
  EditorCommand sheetNew{
      "sheet_new",  // clang-format break
      QT_TR_NOOP("New Sheet"),
      QT_TR_NOOP("Add a new schematic sheet to the project"),
      ":/img/actions/new.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_N)},
      &categoryEditor,
  };
  EditorCommand sheetRename{
      "sheet_rename",  // clang-format break
      QT_TR_NOOP("Rename Sheet"),
      QT_TR_NOOP("Rename the current schematic sheet"),
      QString(),
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_F2)},
      &categoryEditor,
  };
  EditorCommand sheetRemove{
      "sheet_remove",  // clang-format break
      QT_TR_NOOP("Remove Sheet"),
      QT_TR_NOOP("Remove the current schematic sheet from the project"),
      ":/img/actions/delete.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Delete)},
      &categoryEditor,
  };
  EditorCommand boardEditor{
      "board_editor",  // clang-format break
      QT_TR_NOOP("Board Editor"),
      QT_TR_NOOP("Bring the board editor window to front"),
      ":/img/actions/board_editor.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_B)},
      &categoryEditor,
  };
  EditorCommand boardNew{
      "board_new",  // clang-format break
      QT_TR_NOOP("New Board"),
      QT_TR_NOOP("Add a new board to the project"),
      ":/img/actions/new.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_N)},
      &categoryEditor,
  };
  EditorCommand boardCopy{
      "board_copy",  // clang-format break
      QT_TR_NOOP("Copy Board"),
      QT_TR_NOOP("Add a copy of the current board to the project"),
      ":/img/actions/copy.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_D)},
      &categoryEditor,
  };
  EditorCommand boardRemove{
      "board_remove",  // clang-format break
      QT_TR_NOOP("Remove Board"),
      QT_TR_NOOP("Remove the current board from the project"),
      ":/img/actions/delete.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Delete)},
      &categoryEditor,
  };
  EditorCommand planeShowAll{
      "plane_show_all",  // clang-format break
      QT_TR_NOOP("Show All Planes"),
      QT_TR_NOOP("Make the filled areas of all planes visible"),
      ":/img/actions/show_planes.png",
      EditorCommand::Flags(),
      {},
      &categoryEditor,
  };
  EditorCommand planeHideAll{
      "plane_hide_all",  // clang-format break
      QT_TR_NOOP("Hide All Planes"),
      QT_TR_NOOP("Make the filled areas of all planes invisible"),
      ":/img/actions/hide_planes.png",
      EditorCommand::Flags(),
      {},
      &categoryEditor,
  };
  EditorCommand planeRebuildAll{
      "plane_rebuild_all",  // clang-format break
      QT_TR_NOOP("Rebuild All Planes"),
      QT_TR_NOOP("Re-calculate the filled areas of all planes"),
      ":/img/actions/rebuild_plane.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R)},
      &categoryEditor,
  };

  EditorCommandCategory categoryTextInput{
      "categoryTextInput", QT_TR_NOOP("Text Input"), true, &categoryRoot};
  EditorCommand inputBrowse{
      "input_browse",  // clang-format break
      QT_TR_NOOP("Browse"),
      QT_TR_NOOP("Open file or directory browser"),
      ":/img/actions/open.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_B)},
      &categoryTextInput,
  };
  EditorCommand inputUnitChange{
      "input_unit_change",  // clang-format break
      QT_TR_NOOP("Change Unit"),
      QT_TR_NOOP("Change the measurement unit of the text input"),
      ":/img/actions/ruler.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_M)},
      &categoryTextInput,
  };
  EditorCommand inputRemove{
      "input_remove",  // clang-format break
      QT_TR_NOOP("Remove"),
      QT_TR_NOOP("Remove this item"),
      ":/img/actions/delete.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Delete)},
      &categoryTextInput,
  };
  EditorCommand inputAcceptAdd{
      "input_accept_add",  // clang-format break
      QT_TR_NOOP("Add"),
      QT_TR_NOOP("Add this item"),
      ":/img/actions/plus_2.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Return)},
      &categoryTextInput,
  };

  EditorCommandCategory categoryImportExport{
      "categoryImportExport", QT_TR_NOOP("Import/Export"), true, &categoryRoot};
  EditorCommand addExampleProjects{
      "add_example_projects",  // clang-format break
      QT_TR_NOOP("Add Example Projects"),
      QT_TR_NOOP("Add some example projects to the workspace"),
      ":/img/logo/32x32.png",
      EditorCommand::Flag::OpensPopup,
      {},
      nullptr,  // Exclude from shortcuts overview & configuration
  };
  EditorCommand importDxf{
      "import_dxf",  // clang-format break
      QT_TR_NOOP("Import DXF"),
      QT_TR_NOOP("Import a 2D mechanical drawing"),
      ":/img/actions/export_svg.png",
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryImportExport,
  };
  EditorCommand importEagleLibrary{
      "import_eagle_library",  // clang-format break
      QT_TR_NOOP("Import EAGLE Library"),
      QT_TR_NOOP("Import library elements from an EAGLE *.lbr file"),
      QString(),
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryImportExport,
  };
  EditorCommand importEagleProject{
      "import_eagle_project",  // clang-format break
      QT_TR_NOOP("Import EAGLE Project"),
      QT_TR_NOOP("Import schematic/board from EAGLE *.sch/*.brd files"),
      QString(),
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryImportExport,
  };
  EditorCommand exportLppz{
      "export_lppz",  // clang-format break
      QT_TR_NOOP("Export *.lppz Archive"),
      QT_TR_NOOP("Export the project as a self-contained *.lppz archive"),
      ":/img/actions/export_zip.png",
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryImportExport,
  };
  EditorCommand exportImage{
      "export_image",  // clang-format break
      QT_TR_NOOP("Export Image"),
      QT_TR_NOOP("Export graphics as a pixmap"),
      ":/img/actions/export_pixmap.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I)},
      &categoryImportExport,
  };
  EditorCommand exportPdf{
      "export_pdf",  // clang-format break
      QT_TR_NOOP("Export PDF"),
      QT_TR_NOOP("Export graphics as a PDF"),
      ":/img/actions/pdf.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P)},
      &categoryImportExport,
  };
  EditorCommand exportStep{
      "export_step",  // clang-format break
      QT_TR_NOOP("Export STEP Model"),
      QT_TR_NOOP("Export PCB as a STEP file for loading it into MCAD software"),
      ":/img/actions/export_step.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T)},
      &categoryImportExport,
  };
  EditorCommand print{
      "print",  // clang-format break
      QT_TR_NOOP("Print"),
      QT_TR_NOOP("Send graphics to a printer"),
      ":/img/actions/print.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::CTRL | Qt::Key_P)},
      &categoryImportExport,
  };
  EditorCommand generateBom{
      "generate_bom",  // clang-format break
      QT_TR_NOOP("Generate Bill Of Materials"),
      QT_TR_NOOP("Generate bill of materials (BOM) file"),
      ":/img/actions/generate_bom.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F9)},
      &categoryImportExport,
  };
  EditorCommand generateFabricationData{
      "generate_fabrication_data",  // clang-format break
      QT_TR_NOOP("Generate Fabrication Data"),
      QT_TR_NOOP("Generate Gerber/Excellon files for PCB fabrication"),
      ":/img/actions/export_gerber.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F10)},
      &categoryImportExport,
  };
  EditorCommand generatePickPlace{
      "generate_pick_place",  // clang-format break
      QT_TR_NOOP("Generate Pick&&Place Files"),
      QT_TR_NOOP("Generate pick&place files for automated PCB assembly"),
      ":/img/actions/export_pick_place_file.png",
      EditorCommand::Flag::OpensPopup,
      {},  // Was F11 until v0.1.7
      &categoryImportExport,
  };
  EditorCommand generateD356Netlist{
      "generate_d356_netlist",  // clang-format break
      QT_TR_NOOP("Generate IPC-D-356A Netlist"),
      QT_TR_NOOP("Generate netlist file for automated PCB testing"),
      ":/img/actions/generate_bom.png",  // No netlist icon yet.
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryImportExport,
  };
  EditorCommand outputJobs{
      "output_jobs",  // clang-format break
      QT_TR_NOOP("Output Jobs"),
      QT_TR_NOOP("Modify or run output jobs"),
      ":/img/actions/output_jobs.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F11)},
      &categoryImportExport,
  };
  EditorCommand orderPcb{
      "order_pcb",  // clang-format break
      QT_TR_NOOP("Order PCB"),
      QT_TR_NOOP("Start ordering the PCB online"),
      ":/img/actions/order_pcb.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_F12)},
      &categoryImportExport,
  };

  EditorCommandCategory categoryModify{"categoryModify", QT_TR_NOOP("Modify"),
                                       true, &categoryRoot};
  EditorCommand undo{
      "undo",  // clang-format break
      QT_TR_NOOP("Undo"),
      QT_TR_NOOP("Revert the last modification"),
      ":/img/actions/undo.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Z)},
      &categoryModify,
  };
  EditorCommand redo{
      "redo",  // clang-format break
      QT_TR_NOOP("Redo"),
      QT_TR_NOOP("Re-apply the last reverted modification"),
      ":/img/actions/redo.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Y),
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)},
      &categoryModify,
  };
  EditorCommand clipboardCut{
      "clipboard_cut",  // clang-format break
      QT_TR_NOOP("Cut"),
      QT_TR_NOOP("Cut the selected object(s) to clipboard"),
      ":/img/actions/cut.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_X)},
      &categoryModify,
  };
  EditorCommand clipboardCopy{
      "clipboard_copy",  // clang-format break
      QT_TR_NOOP("Copy"),
      QT_TR_NOOP("Copy the selected object(s) to clipboard"),
      ":/img/actions/copy.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_C)},
      &categoryModify,
  };
  EditorCommand clipboardPaste{
      "clipboard_paste",  // clang-format break
      QT_TR_NOOP("Paste"),
      QT_TR_NOOP("Paste object(s) from the clipboard"),
      ":/img/actions/paste.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_V)},
      &categoryModify,
  };
  EditorCommand moveLeft{
      "move_left",  // clang-format break
      QT_TR_NOOP("Move Left"),
      QT_TR_NOOP("Move the selected object(s) to the left"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Left)},
      &categoryModify,
  };
  EditorCommand moveRight{
      "move_right",  // clang-format break
      QT_TR_NOOP("Move Right"),
      QT_TR_NOOP("Move the selected object(s) to the right"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Right)},
      &categoryModify,
  };
  EditorCommand moveUp{
      "move_up",  // clang-format break
      QT_TR_NOOP("Move Up"),
      QT_TR_NOOP("Move the selected object(s) up"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Up)},
      &categoryModify,
  };
  EditorCommand moveDown{
      "move_down",  // clang-format break
      QT_TR_NOOP("Move Down"),
      QT_TR_NOOP("Move the selected object(s) down"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Down)},
      &categoryModify,
  };
  EditorCommand rotateCcw{
      "rotate_ccw",  // clang-format break
      QT_TR_NOOP("Rotate Counterclockwise"),
      QT_TR_NOOP("Rotate the selected object(s) counterclockwise"),
      ":/img/actions/rotate_left.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_R)},
      &categoryModify,
  };
  EditorCommand rotateCw{
      "rotate_cw",  // clang-format break
      QT_TR_NOOP("Rotate Clockwise"),
      QT_TR_NOOP("Rotate the selected object(s) clockwise"),
      ":/img/actions/rotate_right.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::SHIFT | Qt::Key_R)},
      &categoryModify,
  };
  EditorCommand mirrorHorizontal{
      "mirror_horizontal",  // clang-format break
      QT_TR_NOOP("Mirror Horizontally"),
      QT_TR_NOOP("Mirror the selected object(s) horizontally"),
      ":/img/actions/mirror_horizontal.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_M)},
      &categoryModify,
  };
  EditorCommand mirrorVertical{
      "mirror_vertical",  // clang-format break
      QT_TR_NOOP("Mirror Vertically"),
      QT_TR_NOOP("Mirror the selected object(s) vertically"),
      ":/img/actions/mirror_vertical.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::SHIFT | Qt::Key_M)},
      &categoryModify,
  };
  EditorCommand flipHorizontal{
      "flip_horizontal",  // clang-format break
      QT_TR_NOOP("Flip Horizontally"),
      QT_TR_NOOP(
          "Flip the selected object(s) horizontally to the other board side"),
      ":/img/actions/flip_horizontal.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F)},
      &categoryModify,
  };
  EditorCommand flipVertical{
      "flip_vertical",  // clang-format break
      QT_TR_NOOP("Flip Vertically"),
      QT_TR_NOOP(
          "Flip the selected object(s) vertically to the other board side"),
      ":/img/actions/flip_vertical.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::SHIFT | Qt::Key_F)},
      &categoryModify,
  };
  EditorCommand moveAlign{
      "move_align",  // clang-format break
      QT_TR_NOOP("Move/Align Objects"),
      QT_TR_NOOP("Move and/or align the selected object(s) vertically or "
                 "horizontally"),
      ":/img/actions/move.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_A)},
      &categoryModify,
  };
  EditorCommand snapToGrid{
      "snap_to_grid",  // clang-format break
      QT_TR_NOOP("Snap to Grid"),
      QT_TR_NOOP("Move the selected object(s) to snap the grid"),
      ":/img/actions/grid.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_S)},
      &categoryModify,
  };
  EditorCommand lock{
      "lock",  // clang-format break
      QT_TR_NOOP("Lock Placement"),
      QT_TR_NOOP("Lock the placement of the selected object(s)"),
      ":/img/status/locked.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_L)},
      &categoryModify,
  };
  EditorCommand unlock{
      "unlock",  // clang-format break
      QT_TR_NOOP("Unlock Placement"),
      QT_TR_NOOP("Unlock the placement of the selected object(s)"),
      ":/img/status/unlocked.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_U)},
      &categoryModify,
  };
  EditorCommand setLineWidth{
      "line_width_set",  // clang-format break
      QT_TR_NOOP("Set Line Width"),
      QT_TR_NOOP(
          "Change the line/trace/stroke width of the selected object(s)"),
      QString(),
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryModify,
  };
  EditorCommand deviceResetTextAll{
      "device_reset_text_all",  // clang-format break
      QT_TR_NOOP("Reset All Texts"),
      QT_TR_NOOP("Reset all texts of the footprint to their initial state"),
      ":/img/actions/undo.png",
      EditorCommand::Flags(),
      {},
      &categoryModify,
  };
  EditorCommand properties{
      "properties",  // clang-format break
      QT_TR_NOOP("Properties"),
      QT_TR_NOOP("View/modify the object properties"),
      ":/img/actions/settings.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_E)},
      &categoryModify,
  };
  EditorCommand rename{
      "rename",  // clang-format break
      QT_TR_NOOP("Rename"),
      QT_TR_NOOP("Rename the selected object"),
      ":/img/actions/edit.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F2)},
      &categoryModify,
  };
  EditorCommand remove{
      "remove",  // clang-format break
      QT_TR_NOOP("Remove"),
      QT_TR_NOOP("Delete the selected object(s)"),
      ":/img/actions/delete.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Delete)},
      &categoryModify,
  };

  EditorCommandCategory categoryView{"categoryView", QT_TR_NOOP("View"), true,
                                     &categoryRoot};
  EditorCommand zoomFitContent{
      "zoom_fit_content",  // clang-format break
      QT_TR_NOOP("Zoom to Fit Contents"),
      QT_TR_NOOP("Set the zoom level to fit the whole content"),
      ":/img/actions/zoom_all.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Home)},
      &categoryView,
  };
  EditorCommand zoomIn{
      "zoom_in",  // clang-format break
      QT_TR_NOOP("Zoom In"),
      QT_TR_NOOP("Increase the zoom level"),
      ":/img/actions/zoom_in.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Plus)},
      &categoryView,
  };
  EditorCommand zoomOut{
      "zoom_out",  // clang-format break
      QT_TR_NOOP("Zoom Out"),
      QT_TR_NOOP("Decrease the zoom level"),
      ":/img/actions/zoom_out.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Minus)},
      &categoryView,
  };
  EditorCommand gridIncrease{
      "grid_increase",  // clang-format break
      QT_TR_NOOP("Increase Grid Interval"),
      QT_TR_NOOP("Increase the grid interval"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Plus)},
      &categoryView,
  };
  EditorCommand gridDecrease{
      "grid_decrease",  // clang-format break
      QT_TR_NOOP("Decrease Grid Interval"),
      QT_TR_NOOP("Decrease the grid interval"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Minus)},
      &categoryView,
  };
  EditorCommand showPinNumbers{
      "show_pin_numbers",  // clang-format break
      QT_TR_NOOP("Show Pin Numbers"),
      QT_TR_NOOP("Show or hide symbol pin numbers"),
      ":/img/actions/show_pin_numbers.png",
      EditorCommand::Flags(),
      {QKeySequence()},
      &categoryView,
  };
  EditorCommand ignoreLocks{
      "ignore_locks",  // clang-format break
      QT_TR_NOOP("Ignore Placement Locks"),
      QT_TR_NOOP("Allow dragging locked items"),
      ":/img/status/unlocked.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L)},
      &categoryView,
  };
  EditorCommand toggle3d{
      "toggle_3d",  // clang-format break
      QT_TR_NOOP("Toggle 2D/3D Mode"),
      QT_TR_NOOP("Switch between 2D and 3D viewer mode"),
      ":/img/actions/view_3d.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_3)},
      &categoryView,
  };

  EditorCommandCategory categoryTools{"categoryTools", QT_TR_NOOP("Tools"),
                                      true, &categoryRoot};
  EditorCommand toolSelect{
      "tool_select",  // clang-format break
      QT_TR_NOOP("Select"),
      QT_TR_NOOP("Select & modify existing objects"),
      ":/img/actions/select.png",
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolLine{
      "tool_line",  // clang-format break
      QT_TR_NOOP("Draw Line"),
      QT_TR_NOOP("Draw graphical lines"),
      ":/img/actions/draw_line.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_L)},
      &categoryTools,
  };
  EditorCommand toolRect{
      "tool_rect",  // clang-format break
      QT_TR_NOOP("Draw Rectangle"),
      QT_TR_NOOP("Draw graphical rectangles"),
      ":/img/actions/draw_rectangle.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_G)},
      &categoryTools,
  };
  EditorCommand toolPolygon{
      "tool_polygon",  // clang-format break
      QT_TR_NOOP("Draw Polygon"),
      QT_TR_NOOP("Draw graphical polygons"),
      ":/img/actions/draw_polygon.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_P)},
      &categoryTools,
  };
  EditorCommand toolCircle{
      "tool_circle",  // clang-format break
      QT_TR_NOOP("Draw Circle"),
      QT_TR_NOOP("Draw graphical circles"),
      ":/img/actions/draw_circle.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_C)},
      &categoryTools,
  };
  EditorCommand toolArc{
      "tool_arc",  // clang-format break
      QT_TR_NOOP("Draw Arc"),
      QT_TR_NOOP("Draw graphical arcs"),
      ":/img/actions/draw_arc.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::SHIFT | Qt::Key_C)},
      &categoryTools,
  };
  EditorCommand toolText{
      "tool_text",  // clang-format break
      QT_TR_NOOP("Add Text"),
      QT_TR_NOOP("Add graphical text objects"),
      ":/img/actions/add_text.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_T)},
      &categoryTools,
  };
  EditorCommand toolName{
      "tool_name",  // clang-format break
      QT_TR_NOOP("Add Name"),
      QT_TR_NOOP("Add graphical text objects for '{{NAME}}'"),
      ":/img/actions/add_name.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_N)},
      &categoryTools,
  };
  EditorCommand toolValue{
      "tool_value",  // clang-format break
      QT_TR_NOOP("Add Value"),
      QT_TR_NOOP("Add graphical text objects for '{{VALUE}}'"),
      ":/img/actions/add_value.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_V)},
      &categoryTools,
  };
  EditorCommand toolPin{
      "tool_pin",  // clang-format break
      QT_TR_NOOP("Add Pin"),
      QT_TR_NOOP("Add symbol pins (electrical connections for schematics)"),
      ":/img/actions/add_symbol_pin.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_I)},
      &categoryTools,
  };
  EditorCommand toolPadTht{
      "tool_pad_tht",  // clang-format break
      QT_TR_NOOP("Add THT Pad"),
      QT_TR_NOOP("Add plated through-hole copper pads"),
      ":/img/actions/add_tht_pad.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_H)},
      &categoryTools,
  };
  EditorCommand toolPadSmt{
      "tool_pad_smt",  // clang-format break
      QT_TR_NOOP("Add SMT Pad"),
      QT_TR_NOOP("Add surface mounted (single layer) copper pads"),
      ":/img/actions/add_smt_pad.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_D)},
      &categoryTools,
  };
  EditorCommand toolPadThermal{
      "tool_pad_thermal",  // clang-format break
      QT_TR_NOOP("Add Thermal Pad"),
      QT_TR_NOOP("Add special SMT pads used as heat sink"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolPadBga{
      "tool_pad_bga",  // clang-format break
      QT_TR_NOOP("Add BGA Pad"),
      QT_TR_NOOP("Add special SMT pads used for ball grid arrays"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolPadEdgeConnector{
      "tool_pad_edge_connector",  // clang-format break
      QT_TR_NOOP("Add Edge Connector Pad"),
      QT_TR_NOOP("Add special SMT pads used as edge connector"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolPadTest{
      "tool_pad_test_point",  // clang-format break
      QT_TR_NOOP("Add Test Pad"),
      QT_TR_NOOP("Add special SMT pads used as test points"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolPadLocalFiducial{
      "tool_pad_local_fiducial",  // clang-format break
      QT_TR_NOOP("Add Local Fiducial Pad"),
      QT_TR_NOOP("Add special SMT pads used as local fiducials"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolPadGlobalFiducial{
      "tool_pad_global_fiducial",  // clang-format break
      QT_TR_NOOP("Add Global Fiducial Pad"),
      QT_TR_NOOP("Add special SMT pads used as global fiducials"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolZone{
      "tool_zone",  // clang-format break
      QT_TR_NOOP("Draw Keepout Zone"),
      QT_TR_NOOP("Draw keep-out zones"),
      ":/img/actions/draw_zone.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Z)},
      &categoryTools,
  };
  EditorCommand toolHole{
      "tool_hole",  // clang-format break
      QT_TR_NOOP("Add Hole"),
      QT_TR_NOOP("Add non-plated holes (NPTH drills)"),
      ":/img/actions/add_hole.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_O)},
      &categoryTools,
  };
  EditorCommand toolWire{
      "tool_wire",  // clang-format break
      QT_TR_NOOP("Draw Wire"),
      QT_TR_NOOP(
          "Draw wires to create electrical connections between symbol pins"),
      ":/img/actions/draw_wire.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_W)},
      &categoryTools,
  };
  EditorCommand toolNetLabel{
      "tool_netlabel",  // clang-format break
      QT_TR_NOOP("Add Net Label"),
      QT_TR_NOOP("Add net labels to explicitly specify the net of wires"),
      ":/img/actions/draw_netlabel.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_N)},
      &categoryTools,
  };
  EditorCommand toolComponent{
      "tool_component",  // clang-format break
      QT_TR_NOOP("Add Component"),
      QT_TR_NOOP("Insert components from the workspace libraries"),
      ":/img/actions/add_component.png",
      EditorCommand::Flag::OpensPopup,
      {QKeySequence(Qt::Key_A)},
      &categoryTools,
  };
  EditorCommand toolTrace{
      "tool_trace",  // clang-format break
      QT_TR_NOOP("Draw Trace"),
      QT_TR_NOOP("Draw copper traces to interconnect devices"),
      ":/img/actions/draw_wire.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_W)},
      &categoryTools,
  };
  EditorCommand toolVia{
      "tool_via",  // clang-format break
      QT_TR_NOOP("Add Via"),
      QT_TR_NOOP("Add plated through-hole vias"),
      ":/img/actions/add_via.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_V)},
      &categoryTools,
  };
  EditorCommand toolPlane{
      "tool_plane",  // clang-format break
      QT_TR_NOOP("Draw Plane"),
      QT_TR_NOOP("Draw auto-filled copper areas to interconnect pads and vias"),
      ":/img/actions/add_plane.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_N)},
      &categoryTools,
  };
  EditorCommand toolGenerateOutline{
      "tool_generate_outline",  // clang-format break
      QT_TR_NOOP("Generate Outline"),
      QT_TR_NOOP("Automatically generate the outline polygon"),
      ":/img/actions/wizard.png",
      EditorCommand::Flags(),
      {},
      &categoryTools,
  };
  EditorCommand toolGenerateCourtyard{
      "tool_generate_courtyard",  // clang-format break
      QT_TR_NOOP("Generate Courtyard"),
      QT_TR_NOOP("Automatically generate the courtyard polygon"),
      ":/img/actions/wizard.png",
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryTools,
  };
  EditorCommand toolMeasure{
      "tool_measure",  // clang-format break
      QT_TR_NOOP("Measure Distance"),
      QT_TR_NOOP("Measure the distance between two points"),
      ":/img/actions/ruler.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_M)},
      &categoryTools,
  };

  EditorCommandCategory categoryCommands{
      "categoryCommands", QT_TR_NOOP("Commands"), true, &categoryRoot};
  EditorCommand commandToolBarFocus{
      "command_toolbar_focus",  // clang-format break
      QT_TR_NOOP("Go To Command Toolbar"),
      QT_TR_NOOP("Move the focus into the command toolbar"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Tab)},
      &categoryCommands,
  };
  EditorCommand abort{
      "abort",  // clang-format break
      QT_TR_NOOP("Abort Command"),
      QT_TR_NOOP("Abort the currently active command"),
      ":/img/actions/stop.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Escape)},
      &categoryCommands,
  };
  EditorCommand layerUp{
      "layer_up",  // clang-format break
      QT_TR_NOOP("Layer Up"),
      QT_TR_NOOP("Switch to the next higher layer (bottom->top)"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_PageUp)},
      &categoryCommands,
  };
  EditorCommand layerDown{
      "layer_down",  // clang-format break
      QT_TR_NOOP("Layer Down"),
      QT_TR_NOOP("Switch to the next lower layer (top->bottom)"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_PageDown)},
      &categoryCommands,
  };
  EditorCommand lineWidthIncrease{
      "line_width_increase",  // clang-format break
      QT_TR_NOOP("Increase Line Width"),
      QT_TR_NOOP("Increase the line/trace/stroke/pad width"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Plus)},
      &categoryCommands,
  };
  EditorCommand lineWidthDecrease{
      "line_width_decrease",  // clang-format break
      QT_TR_NOOP("Decrease Line Width"),
      QT_TR_NOOP("Decrease the line/trace/stroke/pad width"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Minus)},
      &categoryCommands,
  };
  EditorCommand sizeIncrease{
      "size_increase",  // clang-format break
      QT_TR_NOOP("Increase Size"),
      QT_TR_NOOP("Increase the via/pad/pin/text size"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Asterisk)},
      &categoryCommands,
  };
  EditorCommand sizeDecrease{
      "size_decrease",  // clang-format break
      QT_TR_NOOP("Decrease Size"),
      QT_TR_NOOP("Decrease the via/pad/pin/text size"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Slash)},
      &categoryCommands,
  };
  EditorCommand drillIncrease{
      "drill_increase",  // clang-format break
      QT_TR_NOOP("Increase Drill"),
      QT_TR_NOOP("Increase the via/pad/hole drill diameter"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Home)},
      &categoryCommands,
  };
  EditorCommand drillDecrease{
      "drill_decrease",  // clang-format break
      QT_TR_NOOP("Decrease Drill"),
      QT_TR_NOOP("Decrease the via/pad/hole drill diameter"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_End)},
      &categoryCommands,
  };
  EditorCommand widthAutoToggle{
      "width_auto_toggle",  // clang-format break
      QT_TR_NOOP("Toggle Auto-Width"),
      QT_TR_NOOP("Toggle the auto-width property state"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Period)},
      &categoryCommands,
  };
  EditorCommand fillToggle{
      "fill_toggle",  // clang-format break
      QT_TR_NOOP("Toggle Fill"),
      QT_TR_NOOP("Toggle the fill property state"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F)},
      &categoryCommands,
  };
  EditorCommand grabAreaToggle{
      "grab_area_toggle",  // clang-format break
      QT_TR_NOOP("Toggle Grab Area"),
      QT_TR_NOOP("Toggle the grab area property state"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_Comma)},
      &categoryCommands,
  };
  EditorCommand alignHorizontalLeft{
      "align_horizontal_left",  // clang-format break
      QT_TR_NOOP("Align Left"),
      QT_TR_NOOP("Horizontal alignment: Left"),
      ":/img/command_toolbars/align_horizontal_left.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_1)},
      &categoryCommands,
  };
  EditorCommand alignHorizontalCenter{
      "align_horizontal_center",  // clang-format break
      QT_TR_NOOP("Align Center"),
      QT_TR_NOOP("Horizontal alignment: Center"),
      ":/img/command_toolbars/align_horizontal_center.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_2)},
      &categoryCommands,
  };
  EditorCommand alignHorizontalRight{
      "align_horizontal_right",  // clang-format break
      QT_TR_NOOP("Align Right"),
      QT_TR_NOOP("Horizontal alignment: Right"),
      ":/img/command_toolbars/align_horizontal_right.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_3)},
      &categoryCommands,
  };
  EditorCommand alignVerticalBottom{
      "align_vertical_bottom",  // clang-format break
      QT_TR_NOOP("Align Bottom"),
      QT_TR_NOOP("Vertical alignment: Bottom"),
      ":/img/command_toolbars/align_vertical_bottom.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_4)},
      &categoryCommands,
  };
  EditorCommand alignVerticalCenter{
      "align_vertical_center",  // clang-format break
      QT_TR_NOOP("Align Center"),
      QT_TR_NOOP("Vertical alignment: Center"),
      ":/img/command_toolbars/align_vertical_center.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_5)},
      &categoryCommands,
  };
  EditorCommand alignVerticalTop{
      "align_vertical_top",  // clang-format break
      QT_TR_NOOP("Align Top"),
      QT_TR_NOOP("Vertical alignment: Top"),
      ":/img/command_toolbars/align_vertical_top.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_6)},
      &categoryCommands,
  };
  EditorCommand wireModeHV{
      "wire_mode_h_v",  // clang-format break
      QT_TR_NOOP("Horizontal - Vertical"),
      QT_TR_NOOP(
          "Wire mode: First segment horizontal, second segment vertical"),
      ":/img/command_toolbars/wire_h_v.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_1)},
      &categoryCommands,
  };
  EditorCommand wireModeVH{
      "wire_mode_v_h",  // clang-format break
      QT_TR_NOOP("Vertical - Horizontal"),
      QT_TR_NOOP(
          "Wire mode: First segment vertical, second segment horizontal"),
      ":/img/command_toolbars/wire_v_h.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_2)},
      &categoryCommands,
  };
  EditorCommand wireMode9045{
      "wire_mode_90_45",  // clang-format break
      QT_TR_NOOP("90° - 45°"),
      QT_TR_NOOP("Wire mode: First segment 90°, second segment 45°"),
      ":/img/command_toolbars/wire_90_45.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_3)},
      &categoryCommands,
  };
  EditorCommand wireMode4590{
      "wire_mode_45_90",  // clang-format break
      QT_TR_NOOP("45° - 90°"),
      QT_TR_NOOP("Wire mode: First segment 45°, second segment 90°"),
      ":/img/command_toolbars/wire_45_90.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_4)},
      &categoryCommands,
  };
  EditorCommand wireModeStraight{
      "wire_mode_straight",  // clang-format break
      QT_TR_NOOP("Straight"),
      QT_TR_NOOP("Wire mode: Straight line"),
      ":/img/command_toolbars/wire_straight.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_5)},
      &categoryCommands,
  };
  EditorCommand shapeRound{
      "shape_round",  // clang-format break
      QT_TR_NOOP("Round"),
      QT_TR_NOOP("Shape: Round"),
      ":/img/command_toolbars/shape_round.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_1)},
      &categoryCommands,
  };
  EditorCommand shapeRoundedRect{
      "shape_rounded_rect",  // clang-format break
      QT_TR_NOOP("Rounded Rectangle"),
      QT_TR_NOOP("Shape: Rounded Rectangle"),
      ":/img/command_toolbars/shape_rounded_rect.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_2)},
      &categoryCommands,
  };
  EditorCommand shapeRect{
      "shape_rect",  // clang-format break
      QT_TR_NOOP("Rectangle"),
      QT_TR_NOOP("Shape: Rectangle"),
      ":/img/command_toolbars/shape_rect.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_3)},
      &categoryCommands,
  };
  EditorCommand shapeOctagon{
      "shape_octagon",  // clang-format break
      QT_TR_NOOP("Octagon"),
      QT_TR_NOOP("Shape: Octagon"),
      ":/img/command_toolbars/shape_octagon.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_4)},
      &categoryCommands,
  };

  EditorCommandCategory categoryComponents{
      "categoryComponents", QT_TR_NOOP("Components"), true, &categoryRoot};
  EditorCommand componentResistor{
      "component_resistor",  // clang-format break
      QT_TR_NOOP("Resistor"),
      QT_TR_NOOP("Add standard component: Resistor"),
      ":/img/library/resistor_eu.png",
      EditorCommand::Flags(),
      {},
      &categoryComponents,
  };
  EditorCommand componentInductor{
      "component_inductor",  // clang-format break
      QT_TR_NOOP("Inductor"),
      QT_TR_NOOP("Add standard component: Inductor"),
      ":/img/library/inductor_eu.png",
      EditorCommand::Flags(),
      {},
      &categoryComponents,
  };
  EditorCommand componentCapacitorBipolar{
      "component_capacitor_bipolar",  // clang-format break
      QT_TR_NOOP("Bipolar Capacitor"),
      QT_TR_NOOP("Add standard component: Bipolar capacitor"),
      ":/img/library/bipolar_capacitor_eu.png",
      EditorCommand::Flags(),
      {},
      &categoryComponents,
  };
  EditorCommand componentCapacitorUnipolar{
      "component_capacitor_unipolar",  // clang-format break
      QT_TR_NOOP("Unipolar Capacitor"),
      QT_TR_NOOP("Add standard component: Unipolar capacitor"),
      ":/img/library/unipolar_capacitor_eu.png",
      EditorCommand::Flags(),
      {},
      &categoryComponents,
  };
  EditorCommand componentGnd{
      "component_gnd",  // clang-format break
      QT_TR_NOOP("GND Supply"),
      QT_TR_NOOP("Add standard component: GND supply"),
      ":/img/library/gnd.png",
      EditorCommand::Flags(),
      {},
      &categoryComponents,
  };
  EditorCommand componentVcc{
      "component_vcc",  // clang-format break
      QT_TR_NOOP("VCC Supply"),
      QT_TR_NOOP("Add standard component: VCC supply"),
      ":/img/library/vcc.png",
      EditorCommand::Flags(),
      {},
      &categoryComponents,
  };

  EditorCommandCategory categoryDocks{"categoryDocks", QT_TR_NOOP("Docks"),
                                      true, &categoryRoot};
  EditorCommand dockPages{
      "dock_pages",  // clang-format break
      QT_TR_NOOP("Pages"),
      QT_TR_NOOP("Go to the pages dock"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G)},
      &categoryDocks,
  };
  EditorCommand dockErc{
      "dock_erc",  // clang-format break
      QT_TR_NOOP("Electrical Rule Check (ERC)"),
      QT_TR_NOOP("Go to the ERC messages dock"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_E)},
      &categoryDocks,
  };
  EditorCommand dockDrc{
      "dock_drc",  // clang-format break
      QT_TR_NOOP("Design Rule Check (DRC)"),
      QT_TR_NOOP("Go to the DRC messages dock"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_D)},
      &categoryDocks,
  };
  EditorCommand dockLayers{
      "dock_layers",  // clang-format break
      QT_TR_NOOP("Layers"),
      QT_TR_NOOP("Go to the layers dock"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L)},
      &categoryDocks,
  };
  EditorCommand dockPlaceDevices{
      "dock_place_devices",  // clang-format break
      QT_TR_NOOP("Place Devices"),
      QT_TR_NOOP("Go to the dock for placing devices"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P)},
      &categoryDocks,
  };

  EditorCommandCategory categoryWindowManagement{
      "categoryWindowManagement", QT_TR_NOOP("Window Management"), true,
      &categoryRoot};
  EditorCommand pageNext{
      "page_next",  // clang-format break
      QT_TR_NOOP("Next Tab/Page"),
      QT_TR_NOOP("Navigate to the next tab or page"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_Tab)},
      &categoryWindowManagement,
  };
  EditorCommand pagePrevious{
      "page_previous",  // clang-format break
      QT_TR_NOOP("Previous Tab/Page"),
      QT_TR_NOOP("Navigate to the previous tab or page"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab)},
      &categoryWindowManagement,
  };
  EditorCommand tabClose{
      "tab_close",  // clang-format break
      QT_TR_NOOP("Close Tab"),
      QT_TR_NOOP("Close the currently opened tab"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_W)},
      &categoryWindowManagement,
  };
  EditorCommand tabCloseAll{
      "tab_close_all",  // clang-format break
      QT_TR_NOOP("Close All Tabs"),
      QT_TR_NOOP("Close all currently opened tabs"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W)},
      &categoryWindowManagement,
  };
  EditorCommand windowClose{
      "window_close",  // clang-format break
      QT_TR_NOOP("Close Window"),
      QT_TR_NOOP("Close this window"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::ALT | Qt::Key_F4)},
      &categoryWindowManagement,
  };
  EditorCommand projectClose{
      "project_close",  // clang-format break
      QT_TR_NOOP("Close Project"),
      QT_TR_NOOP("Close the currently opened project"),
      ":/img/actions/close.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_F4)},
      &categoryWindowManagement,
  };
  EditorCommand projectCloseAll{
      "project_close_all",  // clang-format break
      QT_TR_NOOP("Close All Projects"),
      QT_TR_NOOP("Close all currently opened projects"),
      ":/img/actions/close.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F4)},
      &categoryWindowManagement,
  };
  EditorCommand applicationQuit{
      "application_quit",  // clang-format break
      QT_TR_NOOP("Quit"),
      QT_TR_NOOP("Close the whole application"),
      ":/img/actions/quit.png",
      EditorCommand::Flag::QuitRole,
      {QKeySequence(Qt::CTRL | Qt::Key_Q)},
      &categoryWindowManagement,
  };

  EditorCommandCategory categoryHelp{"categoryHelp", QT_TR_NOOP("Help"), true,
                                     &categoryRoot};
  EditorCommand aboutLibrePcb{
      "about_librepcb",  // clang-format break
      QT_TR_NOOP("About LibrePCB"),
      QT_TR_NOOP("Show information about the application"),
      ":/img/logo/48x48.png",
      EditorCommand::Flag::OpensPopup | EditorCommand::Flag::AboutRole,
      {},
      &categoryHelp,
  };
  EditorCommand aboutQt{
      "about_qt",  // clang-format break
      QT_TR_NOOP("About Qt"),
      QT_TR_NOOP("Show information about Qt"),
      QString(),
      EditorCommand::Flag::OpensPopup | EditorCommand::Flag::AboutQtRole,
      {},
      &categoryHelp,
  };
  EditorCommand website{
      "website",  // clang-format break
      QT_TR_NOOP("LibrePCB Website"),
      QT_TR_NOOP("Open the LibrePCB website in the web browser"),
      ":/img/actions/open_browser.png",
      EditorCommand::Flags(),
      {},
      &categoryHelp,
  };
  EditorCommand documentationOnline{
      "documentation_online",  // clang-format break
      QT_TR_NOOP("Online Documentation"),
      QT_TR_NOOP("Open the documentation in the web browser"),
      ":/img/actions/help.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::Key_F1)},
      &categoryHelp,
  };
  EditorCommand keyboardShortcutsReference{
      "keyboard_shortcuts_reference",  // clang-format break
      QT_TR_NOOP("Keyboard Shortcuts Reference"),
      QT_TR_NOOP("Open a quick reference about the keyboard shortcuts"),
      QString(),
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_F1)},
      &categoryHelp,
  };

  EditorCommandCategory categoryContextMenu{
      "categoryContextMenu", QT_TR_NOOP("Context Menu"), false, &categoryRoot};
  EditorCommand folderNew{
      "folder_new",  // clang-format break
      QT_TR_NOOP("New Folder"),
      QT_TR_NOOP("Create a new folder"),
      ":/img/actions/new_folder.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand favoriteAdd{
      "favorite_add",  // clang-format break
      QT_TR_NOOP("Add To Favorites"),
      QT_TR_NOOP("Add project to favorites"),
      ":/img/actions/bookmark_gray.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand favoriteRemove{
      "favorite_remove",  // clang-format break
      QT_TR_NOOP("Remove From Favorites"),
      QT_TR_NOOP("Remove project from favorites"),
      ":/img/actions/bookmark.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand vertexAdd{
      "vertex_add",  // clang-format break
      QT_TR_NOOP("Add Vertex"),
      QT_TR_NOOP("Insert a new vertex into the selected polygon edge"),
      ":/img/actions/add.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand vertexRemove{
      "vertex_remove",  // clang-format break
      QT_TR_NOOP("Remove Vertex"),
      QT_TR_NOOP("Remove the selected vertex from the polygon"),
      ":/img/actions/delete.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand traceSelectWhole{
      "trace_select_whole",  // clang-format break
      QT_TR_NOOP("Select Whole Trace"),
      QT_TR_NOOP("Select the whole trace"),
      ":/img/actions/bookmark.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand traceMeasureLength{
      "trace_measure_length",  // clang-format break
      QT_TR_NOOP("Measure Selected Segments Length"),
      QT_TR_NOOP("Measure the total length of all selected trace segments"),
      ":/img/actions/ruler.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand traceRemoveWhole{
      "trace_remove_whole",  // clang-format break
      QT_TR_NOOP("Remove Whole Trace"),
      QT_TR_NOOP("Remove the whole trace"),
      ":/img/actions/minus.png",
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand locked{
      "locked",  // clang-format break
      QT_TR_NOOP("Lock Placement"),
      QT_TR_NOOP("Toggle placement lock"),
      ":/img/status/locked.png",  // For consistent context menu look.
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand visible{
      "visible",  // clang-format break
      QT_TR_NOOP("Visible"),
      QT_TR_NOOP("Toggle visibility"),
      QString(),
      EditorCommand::Flags(),
      {},
      &categoryContextMenu,
  };
  EditorCommand copyMpnToClipboard{
      "copy_mpn_to_clipboard",  // clang-format break
      QT_TR_NOOP("Copy MPN to Clipboard"),
      QT_TR_NOOP("Copy this MPN into the clipboard"),
      ":/img/actions/copy.png",
      EditorCommand::Flags(),
      {QKeySequence(Qt::CTRL | Qt::Key_C)},
      &categoryContextMenu,
  };
  EditorCommand openProductWebsite{
      "open_product_website",  // clang-format break
      QT_TR_NOOP("Open Product Website"),
      QT_TR_NOOP("Open product details about this part in the web browser"),
      ":/img/actions/open_browser.png",
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryContextMenu,
  };
  EditorCommand openPricingWebsite{
      "open_pricing_website",  // clang-format break
      QT_TR_NOOP("Open Pricing Website"),
      QT_TR_NOOP("Open pricing details about this part in the web browser"),
      ":/img/library/part.png",
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryContextMenu,
  };
  EditorCommand generateContent{
      // Actually not really for the context menu :-/
      "generate_content",  // clang-format break
      QT_TR_NOOP("Generate Content"),
      QT_TR_NOOP("Automatically generate some content"),
      ":/img/actions/wizard.png",
      EditorCommand::Flag::OpensPopup,
      {},
      &categoryContextMenu,
  };
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
