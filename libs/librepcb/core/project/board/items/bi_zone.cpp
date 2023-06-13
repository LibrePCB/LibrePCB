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
#include "bi_zone.h"

#include "../../../types/layer.h"
#include "../board.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BI_Zone::BI_Zone(Board& board, const BoardZoneData& data)
  : BI_Base(board), onEdited(*this), mData(data) {
  connect(&mBoard, &Board::innerLayerCountChanged, this,
          [this]() { onEdited.notify(Event::BoardLayersChanged); });
}

BI_Zone::~BI_Zone() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BI_Zone::setLayers(const QSet<const Layer*>& layers) {
  const QSet<const Layer*> oldLayers = mData.getLayers();
  if (mData.setLayers(layers)) {
    onEdited.notify(Event::LayersChanged);
    mBoard.invalidatePlanes(oldLayers | mData.getLayers());
    return true;
  } else {
    return false;
  }
}

bool BI_Zone::setRules(Zone::Rules rules) noexcept {
  if (mData.setRules(rules)) {
    onEdited.notify(Event::RulesChanged);
    mBoard.invalidatePlanes(mData.getLayers());
    return true;
  } else {
    return false;
  }
}

bool BI_Zone::setOutline(const Path& outline) noexcept {
  if (mData.setOutline(outline)) {
    onEdited.notify(Event::OutlineChanged);
    mBoard.invalidatePlanes(mData.getLayers());
    return true;
  } else {
    return false;
  }
}

bool BI_Zone::setLocked(bool locked) noexcept {
  if (mData.setLocked(locked)) {
    onEdited.notify(Event::IsLockedChanged);
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BI_Zone::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
  mBoard.invalidatePlanes(mData.getLayers());
}

void BI_Zone::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
  mBoard.invalidatePlanes(mData.getLayers());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
