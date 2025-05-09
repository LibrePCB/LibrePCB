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
#include "schematiceditor.h"

#include "../../utils/slinthelpers.h"

#include <librepcb/core/project/schematic/schematic.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SchematicEditor::SchematicEditor(ProjectEditor& prjEditor, Schematic& schematic,
                                 int uiIndex, QObject* parent) noexcept
  : QObject(parent),
    onUiDataChanged(*this),
    mProjectEditor(prjEditor),
    mSchematic(schematic),
    mUiIndex(uiIndex) {
  connect(&mSchematic, &Schematic::nameChanged, this,
          [this]() { onUiDataChanged.notify(); });
}

SchematicEditor::~SchematicEditor() noexcept {
  emit aboutToBeDestroyed();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicEditor::setUiIndex(int index) noexcept {
  if (index != mUiIndex) {
    mUiIndex = index;
    emit uiIndexChanged();
  }
}

ui::SchematicData SchematicEditor::getUiData() const noexcept {
  return ui::SchematicData{
      q2s(*mSchematic.getName()),  // Name
  };
}

void SchematicEditor::setUiData(const ui::SchematicData& data) noexcept {
  Q_UNUSED(data);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
