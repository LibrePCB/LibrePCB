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
#include "bi_polygon.h"

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

BI_Polygon::BI_Polygon(Board& board, const BoardPolygonData& data)
  : BI_Base(board), onEdited(*this), mData(data) {
}

BI_Polygon::~BI_Polygon() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BI_Polygon::setLayer(const Layer& layer) noexcept {
  const Layer& oldLayer = mData.getLayer();
  if (mData.setLayer(layer)) {
    onEdited.notify(Event::LayerChanged);
    invalidatePlanes(oldLayer);
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setLineWidth(const UnsignedLength& width) noexcept {
  if (mData.setLineWidth(width)) {
    onEdited.notify(Event::LineWidthChanged);
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setPath(const Path& path) noexcept {
  if (mData.setPath(path)) {
    onEdited.notify(Event::PathChanged);
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setIsFilled(bool isFilled) noexcept {
  if (mData.setIsFilled(isFilled)) {
    onEdited.notify(Event::IsFilledChanged);
    invalidatePlanes(mData.getLayer());
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setIsGrabArea(bool isGrabArea) noexcept {
  if (mData.setIsGrabArea(isGrabArea)) {
    onEdited.notify(Event::IsGrabAreaChanged);
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setLocked(bool locked) noexcept {
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

void BI_Polygon::addToBoard() {
  if (isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::addToBoard();
  invalidatePlanes(mData.getLayer());
}

void BI_Polygon::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
  invalidatePlanes(mData.getLayer());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BI_Polygon::invalidatePlanes(const Layer& layer) noexcept {
  if (layer.isCopper()) {
    mBoard.invalidatePlanes(&layer);
  } else if (layer == Layer::boardOutlines()) {
    mBoard.invalidatePlanes();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
