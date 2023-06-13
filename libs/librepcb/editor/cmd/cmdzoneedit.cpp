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
#include "cmdzoneedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdZoneEdit::CmdZoneEdit(Zone& zone) noexcept
  : UndoCommand(tr("Edit zone")),
    mZone(zone),
    mOldLayers(zone.getLayers()),
    mNewLayers(mOldLayers),
    mOldRules(zone.getRules()),
    mNewRules(mOldRules),
    mOldOutline(zone.getOutline()),
    mNewOutline(mOldOutline) {
}

CmdZoneEdit::~CmdZoneEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdZoneEdit::setLayers(Zone::Layers layers, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayers = layers;
  if (immediate) mZone.setLayers(mNewLayers);
}

void CmdZoneEdit::setRules(Zone::Rules rules, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRules = rules;
  if (immediate) mZone.setRules(mNewRules);
}

void CmdZoneEdit::setOutline(const Path& path, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewOutline = path;
  if (immediate) mZone.setOutline(mNewOutline);
}

void CmdZoneEdit::translate(const Point& deltaPos, bool immediate) noexcept {
  setOutline(mNewOutline.translated(deltaPos), immediate);
}

void CmdZoneEdit::snapToGrid(const PositiveLength& gridInterval,
                             bool immediate) noexcept {
  setOutline(mNewOutline.mappedToGrid(gridInterval), immediate);
}

void CmdZoneEdit::rotate(const Angle& angle, const Point& center,
                         bool immediate) noexcept {
  setOutline(mNewOutline.rotated(angle, center), immediate);
}

void CmdZoneEdit::mirrorGeometry(Qt::Orientation orientation,
                                 const Point& center, bool immediate) noexcept {
  setOutline(mNewOutline.mirrored(orientation, center), immediate);
}

void CmdZoneEdit::mirrorLayers(bool immediate) noexcept {
  Zone::Layers tmp = mNewLayers;
  tmp.setFlag(Zone::Layer::Top, mNewLayers.testFlag(Zone::Layer::Bottom));
  tmp.setFlag(Zone::Layer::Bottom, mNewLayers.testFlag(Zone::Layer::Top));
  setLayers(tmp, immediate);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdZoneEdit::performExecute() {
  performRedo();  // can throw

  if (mNewLayers != mOldLayers) return true;
  if (mNewRules != mOldRules) return true;
  if (mNewOutline != mOldOutline) return true;
  return false;
}

void CmdZoneEdit::performUndo() {
  mZone.setLayers(mOldLayers);
  mZone.setRules(mOldRules);
  mZone.setOutline(mOldOutline);
}

void CmdZoneEdit::performRedo() {
  mZone.setLayers(mNewLayers);
  mZone.setRules(mNewRules);
  mZone.setOutline(mNewOutline);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
