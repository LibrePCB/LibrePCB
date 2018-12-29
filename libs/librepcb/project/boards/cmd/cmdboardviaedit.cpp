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
#include "cmdboardviaedit.h"

#include "../items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardViaEdit::CmdBoardViaEdit(BI_Via& via) noexcept
  : UndoCommand(tr("Edit via")),
    mVia(via),
    mOldPos(via.getPosition()),
    mNewPos(mOldPos),
    mOldShape(via.getShape()),
    mNewShape(mOldShape),
    mOldSize(via.getSize()),
    mNewSize(mOldSize),
    mOldDrillDiameter(via.getDrillDiameter()),
    mNewDrillDiameter(mOldDrillDiameter),
    mOldStartLayerName(via.getStartLayerName()),
    mNewStartLayerName(mOldStartLayerName),
    mOldStopLayerName(via.getStopLayerName()),
    mNewStopLayerName(mOldStopLayerName) {
}

CmdBoardViaEdit::~CmdBoardViaEdit() noexcept {
  if (!wasEverExecuted()) {
    mVia.setPosition(mOldPos);
    mVia.setShape(mOldShape);
    mVia.setSize(mOldSize);
    mVia.setDrillDiameter(mOldDrillDiameter);
    mVia.setLayers(mOldStartLayerName, mOldStopLayerName);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardViaEdit::setPosition(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = pos;
  if (immediate) mVia.setPosition(mNewPos);
}

void CmdBoardViaEdit::setDeltaToStartPos(const Point& deltaPos,
                                         bool         immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = mOldPos + deltaPos;
  if (immediate) mVia.setPosition(mNewPos);
}

void CmdBoardViaEdit::setShape(BI_Via::Shape shape, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewShape = shape;
  if (immediate) mVia.setShape(mNewShape);
}

void CmdBoardViaEdit::setSize(const PositiveLength& size,
                              bool                  immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSize = size;
  if (immediate) mVia.setSize(mNewSize);
}

void CmdBoardViaEdit::setDrillDiameter(const PositiveLength& diameter,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDrillDiameter = diameter;
  if (immediate) mVia.setDrillDiameter(mNewDrillDiameter);
}

void CmdBoardViaEdit::setStartLayerName(const QString& startLayerName,
                                    bool immediate) noexcept {
    Q_ASSERT(!wasEverExecuted());
    mNewStartLayerName = startLayerName;
    if (immediate) mVia.setLayers(mNewStartLayerName, mOldStopLayerName);
}

void CmdBoardViaEdit::setStopLayerName(const QString& stopLayerName,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewStopLayerName = stopLayerName;
  if (immediate) mVia.setLayers(mOldStartLayerName, mNewStopLayerName);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardViaEdit::performExecute() {
  performRedo();  // can throw

  return true;  // TODO: determine if the via was really modified
}

void CmdBoardViaEdit::performUndo() {
  qDebug() << "UNDO!!!" << mOldStartLayerName << mOldStopLayerName;
  mVia.setPosition(mOldPos);
  mVia.setShape(mOldShape);
  mVia.setSize(mOldSize);
  mVia.setDrillDiameter(mOldDrillDiameter);
  mVia.setLayers(mOldStartLayerName, mOldStopLayerName);
}

void CmdBoardViaEdit::performRedo() {
  qDebug() << "REDO!!!" << mNewStartLayerName << mNewStopLayerName;
  mVia.setPosition(mNewPos);
  mVia.setShape(mNewShape);
  mVia.setSize(mNewSize);
  mVia.setDrillDiameter(mNewDrillDiameter);
  mVia.setLayers(mNewStartLayerName, mNewStopLayerName);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
