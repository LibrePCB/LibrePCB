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
#include "schematictab.h"

#include "apptoolbox.h"
#include "guiapplication.h"
#include "project/projecteditor.h"

#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/editor/project/schematiceditor/schematicgraphicsscene.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

static QString getTitle(std::shared_ptr<ProjectEditor> prj,
                        int schematicIndex) {
  if (auto s = prj->getProject().getSchematicByIndex(schematicIndex)) {
    return *s->getName();
  }
  return QString();
}

SchematicTab::SchematicTab(GuiApplication& app,
                           std::shared_ptr<ProjectEditor> prj,
                           int schematicIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, ui::TabType::Schematic, prj, schematicIndex,
                     getTitle(prj, schematicIndex), Qt::white, parent) {
}

SchematicTab::~SchematicTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicTab::activate() noexcept {
  if (auto sch = mProject->getProject().getSchematicByIndex(mObjIndex)) {
    mScene.reset(new SchematicGraphicsScene(
        *sch, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    mUiData.overlay_color = q2s(Qt::black);
    emit requestRepaint();
  }
}

void SchematicTab::deactivate() noexcept {
  mScene.reset();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
