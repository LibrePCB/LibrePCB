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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboltab.h"

#include "../libraryeditor2.h"
#include "utils/uihelpers.h"
#include "utils/slinthelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolTab::SymbolTab(GuiApplication& app, LibraryEditor2& editor,
                     const FilePath& fp, QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mEditor(editor),
    mDirPath(fp) {
  // Connect library editor.
  connect(&mEditor, &LibraryEditor2::aboutToBeDestroyed, this,
          &SymbolTab::closeEnforced);
}

SymbolTab::~SymbolTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData SymbolTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::Symbol,  // Type
      q2s(QString("TODO")),  // Title
      ui::TabFeatures{},  // Features
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::SymbolTabData SymbolTab::getDerivedUiData() const noexcept {
  return ui::SymbolTabData{
      mEditor.getUiIndex(),  // Library index
          q2s(QString("TODO")), // Name
          q2s(QString("TODO")), // Description
          q2s(QString("TODO")), // Keywords
           q2s(QString("TODO")), // Author
           q2s(QString("TODO")), // Version
          false, // Deprecated
          nullptr, // Categories
          q2s(Qt::white), // Background color
          q2s(Qt::black), // Foreground color
          q2s(Qt::white), // Overlay color
          q2s(Qt::black), // Overlay text color
          ui::GridStyle::None, // Grid style
          l2s(Length(2540000)), // Grid interval
          l2s(LengthUnit::millimeters()), // Unit
          ui::EditorTool::Select, // Tool
          q2s(Qt::ArrowCursor), // Tool cursor
          slint::SharedString(), // Tool overlay text
          ui::ComboBoxData{}, // Tool layer
      ui::LengthEditData{}, // Tool line width
      ui::LengthEditData{}, // Tool size
      false, // Tool filled
      ui::LineEditData{}, // Tool value
      slint::LogicalPosition{}, // Scene image position
      0, // Frame index
  };
}

void SymbolTab::setDerivedUiData(const ui::SymbolTabData& data) noexcept {
}

void SymbolTab::trigger(ui::TabAction a) noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
