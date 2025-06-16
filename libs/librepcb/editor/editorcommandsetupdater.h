// GENERATED FILE! DO NOT MODIFY!
// Run dev/generate_editorcommandset.py to re-generate.

#ifndef LIBREPCB_EDITOR_EDITORCOMMANDSETUPDATER_H
#define LIBREPCB_EDITOR_EDITORCOMMANDSETUPDATER_H

#include "appwindow.h"
#include "editorcommandset.h"
#include "utils/uihelpers.h"

namespace librepcb {
namespace editor {

class EditorCommandSetUpdater {
public:
  static void update(const ui::EditorCommandSet& out) noexcept {
    EditorCommandSet& cmd = EditorCommandSet::instance();
    // clang-format off
    out.set_item_new(l2s(cmd.itemNew, out.get_item_new()));
    out.set_item_open(l2s(cmd.itemOpen, out.get_item_open()));
    out.set_save(l2s(cmd.save, out.get_save()));
    out.set_save_all(l2s(cmd.saveAll, out.get_save_all()));
    out.set_select_all(l2s(cmd.selectAll, out.get_select_all()));
    out.set_find(l2s(cmd.find, out.get_find()));
    out.set_find_next(l2s(cmd.findNext, out.get_find_next()));
    out.set_find_previous(l2s(cmd.findPrevious, out.get_find_previous()));
    out.set_file_manager(l2s(cmd.fileManager, out.get_file_manager()));
    out.set_workspace_switch(l2s(cmd.workspaceSwitch, out.get_workspace_switch()));
    out.set_workspace_settings(l2s(cmd.workspaceSettings, out.get_workspace_settings()));
    out.set_workspace_libraries_rescan(l2s(cmd.workspaceLibrariesRescan, out.get_workspace_libraries_rescan()));
    out.set_library_manager(l2s(cmd.libraryManager, out.get_library_manager()));
    out.set_library_element_new(l2s(cmd.libraryElementNew, out.get_library_element_new()));
    out.set_library_element_duplicate(l2s(cmd.libraryElementDuplicate, out.get_library_element_duplicate()));
    out.set_project_new(l2s(cmd.projectNew, out.get_project_new()));
    out.set_project_open(l2s(cmd.projectOpen, out.get_project_open()));
    out.set_project_setup(l2s(cmd.projectSetup, out.get_project_setup()));
    out.set_grid_properties(l2s(cmd.gridProperties, out.get_grid_properties()));
    out.set_board_setup(l2s(cmd.boardSetup, out.get_board_setup()));
    out.set_run_quick_check(l2s(cmd.runQuickCheck, out.get_run_quick_check()));
    out.set_run_design_rule_check(l2s(cmd.runDesignRuleCheck, out.get_run_design_rule_check()));
    out.set_project_library_update(l2s(cmd.projectLibraryUpdate, out.get_project_library_update()));
    out.set_sheet_new(l2s(cmd.sheetNew, out.get_sheet_new()));
    out.set_sheet_rename(l2s(cmd.sheetRename, out.get_sheet_rename()));
    out.set_sheet_remove(l2s(cmd.sheetRemove, out.get_sheet_remove()));
    out.set_board_new(l2s(cmd.boardNew, out.get_board_new()));
    out.set_board_copy(l2s(cmd.boardCopy, out.get_board_copy()));
    out.set_board_remove(l2s(cmd.boardRemove, out.get_board_remove()));
    out.set_plane_show_all(l2s(cmd.planeShowAll, out.get_plane_show_all()));
    out.set_plane_hide_all(l2s(cmd.planeHideAll, out.get_plane_hide_all()));
    out.set_plane_rebuild_all(l2s(cmd.planeRebuildAll, out.get_plane_rebuild_all()));
    out.set_input_browse(l2s(cmd.inputBrowse, out.get_input_browse()));
    out.set_input_unit_change(l2s(cmd.inputUnitChange, out.get_input_unit_change()));
    out.set_input_remove(l2s(cmd.inputRemove, out.get_input_remove()));
    out.set_input_accept_add(l2s(cmd.inputAcceptAdd, out.get_input_accept_add()));
    out.set_add_example_projects(l2s(cmd.addExampleProjects, out.get_add_example_projects()));
    out.set_import_dxf(l2s(cmd.importDxf, out.get_import_dxf()));
    out.set_import_eagle_library(l2s(cmd.importEagleLibrary, out.get_import_eagle_library()));
    out.set_import_eagle_project(l2s(cmd.importEagleProject, out.get_import_eagle_project()));
    out.set_import_kicad_library(l2s(cmd.importKiCadLibrary, out.get_import_kicad_library()));
    out.set_import_specctra_ses(l2s(cmd.importSpecctraSes, out.get_import_specctra_ses()));
    out.set_export_lppz(l2s(cmd.exportLppz, out.get_export_lppz()));
    out.set_export_image(l2s(cmd.exportImage, out.get_export_image()));
    out.set_export_pdf(l2s(cmd.exportPdf, out.get_export_pdf()));
    out.set_export_specctra_dsn(l2s(cmd.exportSpecctraDsn, out.get_export_specctra_dsn()));
    out.set_export_step(l2s(cmd.exportStep, out.get_export_step()));
    out.set_print(l2s(cmd.print, out.get_print()));
    out.set_generate_bom(l2s(cmd.generateBom, out.get_generate_bom()));
    out.set_generate_fabrication_data(l2s(cmd.generateFabricationData, out.get_generate_fabrication_data()));
    out.set_generate_pick_place(l2s(cmd.generatePickPlace, out.get_generate_pick_place()));
    out.set_generate_d356_netlist(l2s(cmd.generateD356Netlist, out.get_generate_d356_netlist()));
    out.set_output_jobs(l2s(cmd.outputJobs, out.get_output_jobs()));
    out.set_order_pcb(l2s(cmd.orderPcb, out.get_order_pcb()));
    out.set_undo(l2s(cmd.undo, out.get_undo()));
    out.set_redo(l2s(cmd.redo, out.get_redo()));
    out.set_clipboard_cut(l2s(cmd.clipboardCut, out.get_clipboard_cut()));
    out.set_clipboard_copy(l2s(cmd.clipboardCopy, out.get_clipboard_copy()));
    out.set_clipboard_paste(l2s(cmd.clipboardPaste, out.get_clipboard_paste()));
    out.set_move_left(l2s(cmd.moveLeft, out.get_move_left()));
    out.set_move_right(l2s(cmd.moveRight, out.get_move_right()));
    out.set_move_up(l2s(cmd.moveUp, out.get_move_up()));
    out.set_move_down(l2s(cmd.moveDown, out.get_move_down()));
    out.set_rotate_ccw(l2s(cmd.rotateCcw, out.get_rotate_ccw()));
    out.set_rotate_cw(l2s(cmd.rotateCw, out.get_rotate_cw()));
    out.set_mirror_horizontal(l2s(cmd.mirrorHorizontal, out.get_mirror_horizontal()));
    out.set_mirror_vertical(l2s(cmd.mirrorVertical, out.get_mirror_vertical()));
    out.set_flip_horizontal(l2s(cmd.flipHorizontal, out.get_flip_horizontal()));
    out.set_flip_vertical(l2s(cmd.flipVertical, out.get_flip_vertical()));
    out.set_move_align(l2s(cmd.moveAlign, out.get_move_align()));
    out.set_snap_to_grid(l2s(cmd.snapToGrid, out.get_snap_to_grid()));
    out.set_lock(l2s(cmd.lock, out.get_lock()));
    out.set_unlock(l2s(cmd.unlock, out.get_unlock()));
    out.set_line_width_set(l2s(cmd.setLineWidth, out.get_line_width_set()));
    out.set_device_reset_text_all(l2s(cmd.deviceResetTextAll, out.get_device_reset_text_all()));
    out.set_properties(l2s(cmd.properties, out.get_properties()));
    out.set_rename(l2s(cmd.rename, out.get_rename()));
    out.set_remove(l2s(cmd.remove, out.get_remove()));
    out.set_zoom_fit_content(l2s(cmd.zoomFitContent, out.get_zoom_fit_content()));
    out.set_zoom_in(l2s(cmd.zoomIn, out.get_zoom_in()));
    out.set_zoom_out(l2s(cmd.zoomOut, out.get_zoom_out()));
    out.set_grid_increase(l2s(cmd.gridIncrease, out.get_grid_increase()));
    out.set_grid_decrease(l2s(cmd.gridDecrease, out.get_grid_decrease()));
    out.set_show_pin_numbers(l2s(cmd.showPinNumbers, out.get_show_pin_numbers()));
    out.set_ignore_locks(l2s(cmd.ignoreLocks, out.get_ignore_locks()));
    out.set_toggle_background_image(l2s(cmd.toggleBackgroundImage, out.get_toggle_background_image()));
    out.set_toggle_3d(l2s(cmd.toggle3d, out.get_toggle_3d()));
    out.set_tool_select(l2s(cmd.toolSelect, out.get_tool_select()));
    out.set_tool_line(l2s(cmd.toolLine, out.get_tool_line()));
    out.set_tool_rect(l2s(cmd.toolRect, out.get_tool_rect()));
    out.set_tool_polygon(l2s(cmd.toolPolygon, out.get_tool_polygon()));
    out.set_tool_circle(l2s(cmd.toolCircle, out.get_tool_circle()));
    out.set_tool_arc(l2s(cmd.toolArc, out.get_tool_arc()));
    out.set_tool_text(l2s(cmd.toolText, out.get_tool_text()));
    out.set_tool_name(l2s(cmd.toolName, out.get_tool_name()));
    out.set_tool_value(l2s(cmd.toolValue, out.get_tool_value()));
    out.set_tool_pin(l2s(cmd.toolPin, out.get_tool_pin()));
    out.set_tool_pad_tht(l2s(cmd.toolPadTht, out.get_tool_pad_tht()));
    out.set_tool_pad_smt(l2s(cmd.toolPadSmt, out.get_tool_pad_smt()));
    out.set_tool_pad_thermal(l2s(cmd.toolPadThermal, out.get_tool_pad_thermal()));
    out.set_tool_pad_bga(l2s(cmd.toolPadBga, out.get_tool_pad_bga()));
    out.set_tool_pad_edge_connector(l2s(cmd.toolPadEdgeConnector, out.get_tool_pad_edge_connector()));
    out.set_tool_pad_test_point(l2s(cmd.toolPadTest, out.get_tool_pad_test_point()));
    out.set_tool_pad_local_fiducial(l2s(cmd.toolPadLocalFiducial, out.get_tool_pad_local_fiducial()));
    out.set_tool_pad_global_fiducial(l2s(cmd.toolPadGlobalFiducial, out.get_tool_pad_global_fiducial()));
    out.set_tool_zone(l2s(cmd.toolZone, out.get_tool_zone()));
    out.set_tool_hole(l2s(cmd.toolHole, out.get_tool_hole()));
    out.set_tool_wire(l2s(cmd.toolWire, out.get_tool_wire()));
    out.set_tool_netlabel(l2s(cmd.toolNetLabel, out.get_tool_netlabel()));
    out.set_tool_component(l2s(cmd.toolComponent, out.get_tool_component()));
    out.set_tool_trace(l2s(cmd.toolTrace, out.get_tool_trace()));
    out.set_tool_via(l2s(cmd.toolVia, out.get_tool_via()));
    out.set_tool_plane(l2s(cmd.toolPlane, out.get_tool_plane()));
    out.set_tool_generate_outline(l2s(cmd.toolGenerateOutline, out.get_tool_generate_outline()));
    out.set_tool_generate_courtyard(l2s(cmd.toolGenerateCourtyard, out.get_tool_generate_courtyard()));
    out.set_tool_renumber_pads(l2s(cmd.toolReNumberPads, out.get_tool_renumber_pads()));
    out.set_tool_measure(l2s(cmd.toolMeasure, out.get_tool_measure()));
    out.set_command_toolbar_focus(l2s(cmd.commandToolBarFocus, out.get_command_toolbar_focus()));
    out.set_abort(l2s(cmd.abort, out.get_abort()));
    out.set_layer_up(l2s(cmd.layerUp, out.get_layer_up()));
    out.set_layer_down(l2s(cmd.layerDown, out.get_layer_down()));
    out.set_line_width_increase(l2s(cmd.lineWidthIncrease, out.get_line_width_increase()));
    out.set_line_width_decrease(l2s(cmd.lineWidthDecrease, out.get_line_width_decrease()));
    out.set_size_increase(l2s(cmd.sizeIncrease, out.get_size_increase()));
    out.set_size_decrease(l2s(cmd.sizeDecrease, out.get_size_decrease()));
    out.set_drill_increase(l2s(cmd.drillIncrease, out.get_drill_increase()));
    out.set_drill_decrease(l2s(cmd.drillDecrease, out.get_drill_decrease()));
    out.set_width_auto_toggle(l2s(cmd.widthAutoToggle, out.get_width_auto_toggle()));
    out.set_fill_toggle(l2s(cmd.fillToggle, out.get_fill_toggle()));
    out.set_grab_area_toggle(l2s(cmd.grabAreaToggle, out.get_grab_area_toggle()));
    out.set_align_horizontal_left(l2s(cmd.alignHorizontalLeft, out.get_align_horizontal_left()));
    out.set_align_horizontal_center(l2s(cmd.alignHorizontalCenter, out.get_align_horizontal_center()));
    out.set_align_horizontal_right(l2s(cmd.alignHorizontalRight, out.get_align_horizontal_right()));
    out.set_align_vertical_bottom(l2s(cmd.alignVerticalBottom, out.get_align_vertical_bottom()));
    out.set_align_vertical_center(l2s(cmd.alignVerticalCenter, out.get_align_vertical_center()));
    out.set_align_vertical_top(l2s(cmd.alignVerticalTop, out.get_align_vertical_top()));
    out.set_wire_mode_h_v(l2s(cmd.wireModeHV, out.get_wire_mode_h_v()));
    out.set_wire_mode_v_h(l2s(cmd.wireModeVH, out.get_wire_mode_v_h()));
    out.set_wire_mode_90_45(l2s(cmd.wireMode9045, out.get_wire_mode_90_45()));
    out.set_wire_mode_45_90(l2s(cmd.wireMode4590, out.get_wire_mode_45_90()));
    out.set_wire_mode_straight(l2s(cmd.wireModeStraight, out.get_wire_mode_straight()));
    out.set_shape_round(l2s(cmd.shapeRound, out.get_shape_round()));
    out.set_shape_rounded_rect(l2s(cmd.shapeRoundedRect, out.get_shape_rounded_rect()));
    out.set_shape_rect(l2s(cmd.shapeRect, out.get_shape_rect()));
    out.set_shape_octagon(l2s(cmd.shapeOctagon, out.get_shape_octagon()));
    out.set_component_resistor(l2s(cmd.componentResistor, out.get_component_resistor()));
    out.set_component_inductor(l2s(cmd.componentInductor, out.get_component_inductor()));
    out.set_component_capacitor_bipolar(l2s(cmd.componentCapacitorBipolar, out.get_component_capacitor_bipolar()));
    out.set_component_capacitor_unipolar(l2s(cmd.componentCapacitorUnipolar, out.get_component_capacitor_unipolar()));
    out.set_component_gnd(l2s(cmd.componentGnd, out.get_component_gnd()));
    out.set_component_vcc(l2s(cmd.componentVcc, out.get_component_vcc()));
    out.set_dock_erc(l2s(cmd.dockErc, out.get_dock_erc()));
    out.set_dock_drc(l2s(cmd.dockDrc, out.get_dock_drc()));
    out.set_dock_layers(l2s(cmd.dockLayers, out.get_dock_layers()));
    out.set_dock_place_devices(l2s(cmd.dockPlaceDevices, out.get_dock_place_devices()));
    out.set_page_next(l2s(cmd.pageNext, out.get_page_next()));
    out.set_page_previous(l2s(cmd.pagePrevious, out.get_page_previous()));
    out.set_tab_close(l2s(cmd.tabClose, out.get_tab_close()));
    out.set_tab_close_all(l2s(cmd.tabCloseAll, out.get_tab_close_all()));
    out.set_window_new(l2s(cmd.windowNew, out.get_window_new()));
    out.set_window_close(l2s(cmd.windowClose, out.get_window_close()));
    out.set_project_close(l2s(cmd.projectClose, out.get_project_close()));
    out.set_application_quit(l2s(cmd.applicationQuit, out.get_application_quit()));
    out.set_about_librepcb(l2s(cmd.aboutLibrePcb, out.get_about_librepcb()));
    out.set_about_qt(l2s(cmd.aboutQt, out.get_about_qt()));
    out.set_website(l2s(cmd.website, out.get_website()));
    out.set_documentation_online(l2s(cmd.documentationOnline, out.get_documentation_online()));
    out.set_support(l2s(cmd.support, out.get_support()));
    out.set_donate(l2s(cmd.donate, out.get_donate()));
    out.set_keyboard_shortcuts_reference(l2s(cmd.keyboardShortcutsReference, out.get_keyboard_shortcuts_reference()));
    out.set_vertex_add(l2s(cmd.vertexAdd, out.get_vertex_add()));
    out.set_vertex_remove(l2s(cmd.vertexRemove, out.get_vertex_remove()));
    out.set_trace_select_whole(l2s(cmd.traceSelectWhole, out.get_trace_select_whole()));
    out.set_trace_measure_length(l2s(cmd.traceMeasureLength, out.get_trace_measure_length()));
    out.set_trace_remove_whole(l2s(cmd.traceRemoveWhole, out.get_trace_remove_whole()));
    out.set_locked(l2s(cmd.locked, out.get_locked()));
    out.set_visible(l2s(cmd.visible, out.get_visible()));
    out.set_copy_mpn_to_clipboard(l2s(cmd.copyMpnToClipboard, out.get_copy_mpn_to_clipboard()));
    out.set_open_product_website(l2s(cmd.openProductWebsite, out.get_open_product_website()));
    out.set_open_pricing_website(l2s(cmd.openPricingWebsite, out.get_open_pricing_website()));
    out.set_helper_tools(l2s(cmd.helperTools, out.get_helper_tools()));
    // clang-format on
  }
};

}  // namespace editor
}  // namespace librepcb

#endif
