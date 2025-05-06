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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITOR_H
#define LIBREPCB_EDITOR_SCHEMATICEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Schematic;

namespace editor {

class ProjectEditor;

/*******************************************************************************
 *  Class SchematicEditor
 ******************************************************************************/

/**
 * @brief The SchematicEditor class
 */
class SchematicEditor final : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<SchematicEditor> onUiDataChanged;

  // Constructors / Destructor
  SchematicEditor() = delete;
  SchematicEditor(const SchematicEditor& other) = delete;
  explicit SchematicEditor(ProjectEditor& prjEditor, Schematic& schematic,
                           int uiIndex, QObject* parent = nullptr) noexcept;
  ~SchematicEditor() noexcept;

  // General Methods
  ProjectEditor& getProjectEditor() noexcept { return mProjectEditor; }
  Schematic& getSchematic() noexcept { return mSchematic; }
  int getUiIndex() const noexcept { return mUiIndex; }
  void setUiIndex(int index) noexcept;
  ui::SchematicData getUiData() const noexcept;
  void setUiData(const ui::SchematicData& data) noexcept;

  // Operator Overloadings
  SchematicEditor& operator=(const SchematicEditor& rhs) = delete;

signals:
  void uiIndexChanged();
  void aboutToBeDestroyed();

private:
  ProjectEditor& mProjectEditor;
  Schematic& mSchematic;
  int mUiIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
