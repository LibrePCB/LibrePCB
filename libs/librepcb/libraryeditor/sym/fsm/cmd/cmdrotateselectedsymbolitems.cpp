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
#include "cmdrotateselectedsymbolitems.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/geometry/cmd/cmdellipseedit.h>
#include <librepcb/common/geometry/cmd/cmdtextedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonmove.h>
#include <librepcb/library/sym/symbolpin.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/library/sym/symbolpingraphicsitem.h>
#include <librepcb/common/graphics/ellipsegraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include <librepcb/common/graphics/textgraphicsitem.h>
#include <librepcb/library/sym/cmd/cmdsymbolpinedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRotateSelectedSymbolItems::CmdRotateSelectedSymbolItems(const SymbolEditorState::Context& context,
                                                       const Angle& angle) noexcept :
    UndoCommandGroup(tr("Rotate Symbol Elements")), mContext(context), mAngle(angle)
{
}

CmdRotateSelectedSymbolItems::~CmdRotateSelectedSymbolItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRotateSelectedSymbolItems::performExecute()
{
    // get all selected items
    QList<QSharedPointer<SymbolPinGraphicsItem>> pins = mContext.symbolGraphicsItem.getSelectedPins();
    QList<QSharedPointer<EllipseGraphicsItem>> ellipses = mContext.symbolGraphicsItem.getSelectedEllipses();
    QList<QSharedPointer<PolygonGraphicsItem>> polygons = mContext.symbolGraphicsItem.getSelectedPolygons();
    QList<QSharedPointer<TextGraphicsItem>> texts = mContext.symbolGraphicsItem.getSelectedTexts();
    int count = pins.count() + ellipses.count() + polygons.count() + texts.count();

    // no items selected --> nothing to do here
    if (count <= 0) {
        return false;
    }

    // find the center of all elements
    Point center = Point(0, 0);
    foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin, pins) {Q_ASSERT(pin);
        center += pin->getPin().getPosition();
    }
    foreach (const QSharedPointer<EllipseGraphicsItem>& ellipse, ellipses) {Q_ASSERT(ellipse);
        center += ellipse->getEllipse().getCenter();
    }
    foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {Q_ASSERT(polygon);
        center += polygon->getPolygon().calcCenter();
    }
    foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {Q_ASSERT(text);
        center += text->getText().getPosition();
    }
    center /= count;
    center.mapToGrid(mContext.graphicsView.getGridProperties().getInterval());

    // rotate all selected elements
    foreach (const QSharedPointer<SymbolPinGraphicsItem>& pin, pins) {Q_ASSERT(pin);
        CmdSymbolPinEdit* cmd = new CmdSymbolPinEdit(pin->getPin());
        cmd->rotate(mAngle, center, false);
        appendChild(cmd);
    }
    foreach (const QSharedPointer<EllipseGraphicsItem>& ellipse, ellipses) {Q_ASSERT(ellipse);
        CmdEllipseEdit* cmd = new CmdEllipseEdit(ellipse->getEllipse());
        cmd->rotate(mAngle, center, false);
        appendChild(cmd);
    }
    foreach (const QSharedPointer<PolygonGraphicsItem>& polygon, polygons) {Q_ASSERT(polygon);
        CmdPolygonMove* cmd = new CmdPolygonMove(polygon->getPolygon());
        cmd->rotate(mAngle, center, false);
        appendChild(cmd);
    }
    foreach (const QSharedPointer<TextGraphicsItem>& text, texts) {Q_ASSERT(text);
        CmdTextEdit* cmd = new CmdTextEdit(text->getText());
        cmd->rotate(mAngle, center, false);
        appendChild(cmd);
    }

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
