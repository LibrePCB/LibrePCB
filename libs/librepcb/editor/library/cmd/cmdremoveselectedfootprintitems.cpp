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
#include "cmdremoveselectedfootprintitems.h"

#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdholeedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdstroketextedit.h"
#include "cmdfootprintpadedit.h"

#include <librepcb/core/graphics/circlegraphicsitem.h>
#include <librepcb/core/graphics/holegraphicsitem.h>
#include <librepcb/core/graphics/polygongraphicsitem.h>
#include <librepcb/core/graphics/stroketextgraphicsitem.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/footprintgraphicsitem.h>
#include <librepcb/core/library/pkg/footprintpadgraphicsitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdRemoveSelectedFootprintItems::CmdRemoveSelectedFootprintItems(
    const PackageEditorState::Context& context) noexcept
  : UndoCommandGroup(tr("Remove Footprint Elements")), mContext(context) {
  Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);
}

CmdRemoveSelectedFootprintItems::~CmdRemoveSelectedFootprintItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdRemoveSelectedFootprintItems::performExecute() {
  // remove pins
  foreach (const auto& pad, mContext.currentGraphicsItem->getSelectedPads()) {
    appendChild(new CmdFootprintPadRemove(mContext.currentFootprint->getPads(),
                                          pad->getPad().get()));
  }

  // remove circles
  foreach (const auto& circle,
           mContext.currentGraphicsItem->getSelectedCircles()) {
    appendChild(new CmdCircleRemove(mContext.currentFootprint->getCircles(),
                                    &circle->getCircle()));
  }

  // remove polygons
  foreach (const auto& polygon,
           mContext.currentGraphicsItem->getSelectedPolygons()) {
    appendChild(new CmdPolygonRemove(mContext.currentFootprint->getPolygons(),
                                     &polygon->getPolygon()));
  }

  // remove texts
  foreach (const auto& text,
           mContext.currentGraphicsItem->getSelectedStrokeTexts()) {
    appendChild(new CmdStrokeTextRemove(
        mContext.currentFootprint->getStrokeTexts(), &text->getText()));
  }

  // remove holes
  foreach (const auto& hole, mContext.currentGraphicsItem->getSelectedHoles()) {
    appendChild(new CmdHoleRemove(mContext.currentFootprint->getHoles(),
                                  &hole->getHole()));
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
