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
#include "cmddeviceinstanceedit.h"

#include <librepcb/core/project/board/items/bi_device.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdDeviceInstanceEdit::CmdDeviceInstanceEdit(BI_Device& dev) noexcept
  : UndoCommand(tr("Edit Device")),
    mDevice(dev),
    mOldPos(mDevice.getPosition()),
    mNewPos(mOldPos),
    mOldRotation(mDevice.getRotation()),
    mNewRotation(mOldRotation),
    mOldMirrored(mDevice.getMirrored()),
    mNewMirrored(mOldMirrored),
    mOldLocked(mDevice.isLocked()),
    mNewLocked(mOldLocked) {
}

CmdDeviceInstanceEdit::~CmdDeviceInstanceEdit() noexcept {
  if (!wasEverExecuted()) {
    try {
      mDevice.setPosition(mOldPos);
      mDevice.setRotation(mOldRotation);
      mDevice.setMirrored(mOldMirrored);  // can throw
    } catch (Exception& e) {
      qCritical() << "Failed to revert device instance changes:" << e.getMsg();
    }
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdDeviceInstanceEdit::setPosition(const Point& pos,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = pos;
  if (immediate) mDevice.setPosition(mNewPos);
}

void CmdDeviceInstanceEdit::translate(const Point& deltaPos,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mDevice.setPosition(mNewPos);
}

void CmdDeviceInstanceEdit::snapToGrid(const PositiveLength& gridInterval,
                                       bool immediate) noexcept {
  setPosition(mNewPos.mappedToGrid(gridInterval), immediate);
}

void CmdDeviceInstanceEdit::setRotation(const Angle& angle,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mDevice.setRotation(mNewRotation);
}

void CmdDeviceInstanceEdit::rotate(const Angle& angle, const Point& center,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.rotate(angle, center);
  mNewRotation += mNewMirrored
      ? -angle
      : angle;  // mirror --> rotation direction is inverted!
  if (immediate) {
    mDevice.setPosition(mNewPos);
    mDevice.setRotation(mNewRotation);
  }
}

void CmdDeviceInstanceEdit::setMirrored(bool mirrored, bool immediate) {
  Q_ASSERT(!wasEverExecuted());
  if (immediate) {
    mDevice.setMirrored(mirrored);  // can throw
  }
  mNewMirrored = mirrored;
}

void CmdDeviceInstanceEdit::mirror(const Point& center,
                                   Qt::Orientation orientation,
                                   bool immediate) {
  Q_ASSERT(!wasEverExecuted());
  bool mirror = !mNewMirrored;
  Point position = mNewPos;
  Angle rotation = mNewRotation;
  switch (orientation) {
    case Qt::Vertical: {
      position.setY(position.getY() +
                    Length(2) * (center.getY() - position.getY()));
      rotation += Angle::deg180();
      break;
    }
    case Qt::Horizontal: {
      position.setX(position.getX() +
                    Length(2) * (center.getX() - position.getX()));
      break;
    }
    default: {
      qCritical() << "Unhandled switch-case in CmdDeviceInstanceEdit::mirror():"
                  << orientation;
      break;
    }
  }
  if (immediate) {
    mDevice.setMirrored(mirror);  // can throw
    mDevice.setPosition(position);
    mDevice.setRotation(rotation);
  }
  mNewMirrored = mirror;
  mNewPos = position;
  mNewRotation = rotation;
}

void CmdDeviceInstanceEdit::setLocked(bool locked) {
  Q_ASSERT(!wasEverExecuted());
  mNewLocked = locked;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdDeviceInstanceEdit::performExecute() {
  performRedo();  // can throw

  if (mNewPos != mOldPos) return true;
  if (mNewRotation != mOldRotation) return true;
  if (mNewMirrored != mOldMirrored) return true;
  if (mNewLocked != mOldLocked) return true;
  return false;
}

void CmdDeviceInstanceEdit::performUndo() {
  mDevice.setMirrored(mOldMirrored);  // can throw
  mDevice.setPosition(mOldPos);
  mDevice.setRotation(mOldRotation);
  mDevice.setLocked(mOldLocked);
}

void CmdDeviceInstanceEdit::performRedo() {
  mDevice.setMirrored(mNewMirrored);  // can throw
  mDevice.setPosition(mNewPos);
  mDevice.setRotation(mNewRotation);
  mDevice.setLocked(mNewLocked);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
