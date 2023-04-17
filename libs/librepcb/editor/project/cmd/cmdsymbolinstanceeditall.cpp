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
#include "cmdsymbolinstanceeditall.h"

#include "../../cmd/cmdtextedit.h"
#include "cmdsymbolinstanceedit.h"

#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_text.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSymbolInstanceEditAll::CmdSymbolInstanceEditAll(SI_Symbol& symbol) noexcept
  : UndoCommandGroup(tr("Drag Symbol")), mSymEditCmd(nullptr) {
  mSymEditCmd = new CmdSymbolInstanceEdit(symbol);
  appendChild(mSymEditCmd);

  foreach (SI_Text* text, symbol.getTexts()) {
    CmdTextEdit* cmd = new CmdTextEdit(text->getTextObj());
    mTextEditCmds.append(cmd);
    appendChild(cmd);
  }
}

CmdSymbolInstanceEditAll::~CmdSymbolInstanceEditAll() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdSymbolInstanceEditAll::setPosition(const Point& pos,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  translate(pos - mSymEditCmd->mNewPos, immediate);
}

void CmdSymbolInstanceEditAll::translate(const Point& deltaPos,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mSymEditCmd->translate(deltaPos, immediate);
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->translate(deltaPos, immediate);
  }
}

void CmdSymbolInstanceEditAll::setRotation(const Angle& angle,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  const Angle delta = angle - mSymEditCmd->mNewRotation;
  mSymEditCmd->setRotation(angle, immediate);
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(delta, mSymEditCmd->mNewPos, immediate);
  }
}

void CmdSymbolInstanceEditAll::rotate(const Angle& angle, const Point& center,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mSymEditCmd->rotate(angle, center, immediate);
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->rotate(angle, center, immediate);
  }
}

void CmdSymbolInstanceEditAll::setMirrored(bool mirrored,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  if (mirrored != mSymEditCmd->mNewMirrored) {
    mSymEditCmd->setMirrored(mirrored, immediate);
    foreach (CmdTextEdit* cmd, mTextEditCmds) {
      cmd->mirror(mSymEditCmd->mNewRotation, mSymEditCmd->mNewPos, immediate);
    }
  }
}

void CmdSymbolInstanceEditAll::mirror(const Point& center,
                                      Qt::Orientation orientation,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mSymEditCmd->mirror(center, orientation, immediate);
  foreach (CmdTextEdit* cmd, mTextEditCmds) {
    cmd->mirror(orientation, center, immediate);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
