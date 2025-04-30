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
#include "cmdsymbolinstanceedit.h"

#include <librepcb/core/project/schematic/items/si_symbol.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSymbolInstanceEdit::CmdSymbolInstanceEdit(SI_Symbol& symbol) noexcept
  : UndoCommand(tr("Edit Symbol")),
    mSymbol(symbol),
    mOldPos(symbol.getPosition()),
    mNewPos(mOldPos),
    mOldRotation(symbol.getRotation()),
    mNewRotation(mOldRotation),
    mOldMirrored(symbol.getMirrored()),
    mNewMirrored(mOldMirrored) {
}

CmdSymbolInstanceEdit::~CmdSymbolInstanceEdit() noexcept {
  if (!wasEverExecuted()) {
    mSymbol.setPosition(mOldPos);
    mSymbol.setRotation(mOldRotation);
    mSymbol.setMirrored(mOldMirrored);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdSymbolInstanceEdit::setPosition(const Point& pos,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = pos;
  if (immediate) mSymbol.setPosition(mNewPos);
}

void CmdSymbolInstanceEdit::translate(const Point& deltaPos,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mSymbol.setPosition(mNewPos);
}

void CmdSymbolInstanceEdit::snapToGrid(const PositiveLength& gridInterval,
                                       bool immediate) noexcept {
  setPosition(mNewPos.mappedToGrid(gridInterval), immediate);
}

void CmdSymbolInstanceEdit::setRotation(const Angle& angle,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mSymbol.setRotation(mNewRotation);
}

void CmdSymbolInstanceEdit::rotate(const Angle& angle, const Point& center,
                                   bool immediate) noexcept {
  setPosition(mNewPos.rotated(angle, center), immediate);
  setRotation(mNewRotation + angle, immediate);
}

void CmdSymbolInstanceEdit::setMirrored(bool mirrored,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMirrored = mirrored;
  if (immediate) mSymbol.setMirrored(mNewMirrored);
}

void CmdSymbolInstanceEdit::mirror(const Point& center,
                                   Qt::Orientation orientation,
                                   bool immediate) noexcept {
  setPosition(mNewPos.mirrored(orientation, center), immediate);
  setRotation((orientation == Qt::Horizontal)
                  ? -mNewRotation
                  : (Angle::deg180() - mNewRotation),
              immediate);
  setMirrored(!mNewMirrored, immediate);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSymbolInstanceEdit::performExecute() {
  performRedo();  // can throw

  if (mNewPos != mOldPos) return true;
  if (mNewRotation != mOldRotation) return true;
  if (mNewMirrored != mOldMirrored) return true;
  return false;
}

void CmdSymbolInstanceEdit::performUndo() {
  mSymbol.setPosition(mOldPos);
  mSymbol.setRotation(mOldRotation);
  mSymbol.setMirrored(mOldMirrored);
}

void CmdSymbolInstanceEdit::performRedo() {
  mSymbol.setPosition(mNewPos);
  mSymbol.setRotation(mNewRotation);
  mSymbol.setMirrored(mNewMirrored);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
