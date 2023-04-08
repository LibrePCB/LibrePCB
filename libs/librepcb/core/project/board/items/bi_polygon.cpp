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
  if (mData.setLayer(layer)) {
    onEdited.notify(Event::LayerChanged);
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setLineWidth(const UnsignedLength& width) noexcept {
  if (mData.setLineWidth(width)) {
    onEdited.notify(Event::LineWidthChanged);
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setPath(const Path& path) noexcept {
  if (mData.setPath(path)) {
    onEdited.notify(Event::PathChanged);
    return true;
  } else {
    return false;
  }
}

bool BI_Polygon::setIsFilled(bool isFilled) noexcept {
  if (mData.setIsFilled(isFilled)) {
    onEdited.notify(Event::IsFilledChanged);
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
}

void BI_Polygon::removeFromBoard() {
  if (!isAddedToBoard()) {
    throw LogicError(__FILE__, __LINE__);
  }
  BI_Base::removeFromBoard();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
