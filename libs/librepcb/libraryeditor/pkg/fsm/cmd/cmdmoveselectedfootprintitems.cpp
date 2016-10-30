/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "cmdmoveselectedfootprintitems.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/geometry/cmd/cmdellipseedit.h>
#include <librepcb/common/geometry/cmd/cmdtextedit.h>
#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonmove.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include <librepcb/library/pkg/footprintpadgraphicsitem.h>
#include <librepcb/common/graphics/ellipsegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/common/graphics/holegraphicsitem.h>
#include <librepcb/library/pkg/cmd/cmdfootprintpadedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdMoveSelectedFootprintItems::CmdMoveSelectedFootprintItems(
        const PackageEditorState::Context& context, const Point& startPos) noexcept :
    UndoCommandGroup(tr("Move Footprint Elements")),
    mContext(context), mStartPos(startPos), mDeltaPos(0, 0)
{
    Q_ASSERT(context.currentFootprint && context.currentGraphicsItem);
    QList<QSharedPointer<FootprintPadGraphicsItem>> pads = context.currentGraphicsItem->getSelectedPads();
    foreach (const QSharedPointer<FootprintPadGraphicsItem>& pad, pads) {Q_ASSERT(pad);
        mPadEditCmds.append(new CmdFootprintPadEdit(pad->getPad()));
    }

    QList<QSharedPointer<EllipseGraphicsItem>> ellipses = context.currentGraphicsItem->getSelectedEllipses();
    foreach (const QSharedPointer<EllipseGraphicsItem>& ellipse, ellipses) {Q_ASSERT(ellipse);
        mEllipseEditCmds.append(new CmdEllipseEdit(ellipse->getEllipse()));
    }

    QList<QSharedPointer<PolygonGraphicsItem>> polygons = context.currentGraphicsItem->getSelectedPolygons();
    foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {Q_ASSERT(polygon);
        mPolygonEditCmds.append(new CmdPolygonMove(polygon->getPolygon()));
    }

    QList<QSharedPointer<TextGraphicsItem>> texts = context.currentGraphicsItem->getSelectedTexts();
    foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {Q_ASSERT(text);
        mTextEditCmds.append(new CmdTextEdit(text->getText()));
    }

    QList<QSharedPointer<HoleGraphicsItem>> holes = context.currentGraphicsItem->getSelectedHoles();
    foreach (const QSharedPointer<HoleGraphicsItem>& hole, holes) {Q_ASSERT(hole);
        mHoleEditCmds.append(new CmdHoleEdit(hole->getHole()));
    }
}

CmdMoveSelectedFootprintItems::~CmdMoveSelectedFootprintItems() noexcept
{
    deleteAllCommands();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdMoveSelectedFootprintItems::setCurrentPosition(const Point& pos) noexcept
{
    Point delta = pos - mStartPos;
    delta.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());

    if (delta != mDeltaPos) {
        // move selected elements
        foreach (CmdFootprintPadEdit* cmd, mPadEditCmds) {
            cmd->setDeltaToStartPos(delta, true);
        }
        foreach (CmdEllipseEdit* cmd, mEllipseEditCmds) {
            cmd->setDeltaToStartCenter(delta, true);
        }
        foreach (CmdPolygonMove* cmd, mPolygonEditCmds) {
            cmd->setDeltaToStartPos(delta, true);
        }
        foreach (CmdTextEdit* cmd, mTextEditCmds) {
            cmd->setDeltaToStartPos(delta, true);
        }
        foreach (CmdHoleEdit* cmd, mHoleEditCmds) {
            cmd->setDeltaToStartPos(delta, true);
        }
        mDeltaPos = delta;
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdMoveSelectedFootprintItems::performExecute()
{
    if (mDeltaPos.isOrigin()) {
        // no movement required --> discard all move commands
        deleteAllCommands();
        return false;
    }

    // move all child commands to parent class
    while (mPadEditCmds.count() > 0) {
        appendChild(mPadEditCmds.takeLast());
    }
    while (mEllipseEditCmds.count() > 0) {
        appendChild(mEllipseEditCmds.takeLast());
    }
    while (mPolygonEditCmds.count() > 0) {
        appendChild(mPolygonEditCmds.takeLast());
    }
    while (mTextEditCmds.count() > 0) {
        appendChild(mTextEditCmds.takeLast());
    }
    while (mHoleEditCmds.count() > 0) {
        appendChild(mHoleEditCmds.takeLast());
    }

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void CmdMoveSelectedFootprintItems::deleteAllCommands() noexcept
{
    qDeleteAll(mPadEditCmds);           mPadEditCmds.clear();
    qDeleteAll(mEllipseEditCmds);       mEllipseEditCmds.clear();
    qDeleteAll(mPolygonEditCmds);       mPolygonEditCmds.clear();
    qDeleteAll(mTextEditCmds);          mTextEditCmds.clear();
    qDeleteAll(mHoleEditCmds);          mHoleEditCmds.clear();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
