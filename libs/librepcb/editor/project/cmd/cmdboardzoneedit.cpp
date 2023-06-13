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
#include "cmdboardzoneedit.h"

#include <librepcb/core/project/board/items/bi_zone.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardZoneEdit::CmdBoardZoneEdit(BI_Zone& polygon) noexcept
  : UndoCommand(tr("Edit zone")),
    mZone(polygon),
    mOldData(polygon.getData()),
    mNewData(mOldData) {
}

CmdBoardZoneEdit::~CmdBoardZoneEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardZoneEdit::setLayers(const QSet<const Layer*>& layers,
                                 bool immediate) {
  Q_ASSERT(!wasEverExecuted());
  mNewData.setLayers(layers);  // can throw
  if (immediate) mZone.setLayers(layers);
}

void CmdBoardZoneEdit::setRules(Zone::Rules rules, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewData.setRules(rules);
  if (immediate) mZone.setRules(rules);
}

void CmdBoardZoneEdit::setOutline(const Path& path, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewData.setOutline(path);
  if (immediate) mZone.setOutline(path);
}

void CmdBoardZoneEdit::translate(const Point& deltaPos,
                                 bool immediate) noexcept {
  setOutline(mNewData.getOutline().translated(deltaPos), immediate);
}

void CmdBoardZoneEdit::snapToGrid(const PositiveLength& gridInterval,
                                  bool immediate) noexcept {
  setOutline(mNewData.getOutline().mappedToGrid(gridInterval), immediate);
}

void CmdBoardZoneEdit::rotate(const Angle& angle, const Point& center,
                              bool immediate) noexcept {
  setOutline(mNewData.getOutline().rotated(angle, center), immediate);
}

void CmdBoardZoneEdit::mirrorGeometry(Qt::Orientation orientation,
                                      const Point& center,
                                      bool immediate) noexcept {
  setOutline(mNewData.getOutline().mirrored(orientation, center), immediate);
}

void CmdBoardZoneEdit::mirrorLayers(int innerLayers, bool immediate) {
  QSet<const Layer*> layers;
  foreach (const Layer* layer, mNewData.getLayers()) {
    layers.insert(&layer->mirrored(innerLayers));
  }
  setLayers(layers, immediate);  // can throw
}

void CmdBoardZoneEdit::setLocked(bool locked) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewData.setLocked(locked);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardZoneEdit::performExecute() {
  performRedo();  // can throw
  return (mNewData != mOldData);
}

void CmdBoardZoneEdit::performUndo() {
  mZone.setLayers(mOldData.getLayers());
  mZone.setRules(mOldData.getRules());
  mZone.setOutline(mOldData.getOutline());
  mZone.setLocked(mOldData.isLocked());
}

void CmdBoardZoneEdit::performRedo() {
  mZone.setLayers(mNewData.getLayers());
  mZone.setRules(mNewData.getRules());
  mZone.setOutline(mNewData.getOutline());
  mZone.setLocked(mNewData.isLocked());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
