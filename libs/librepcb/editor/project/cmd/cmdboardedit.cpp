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
#include "cmdboardedit.h"

#include <librepcb/core/project/board/board.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardEdit::CmdBoardEdit(Board& board) noexcept
  : UndoCommand(tr("Modify Board Setup")),
    mBoard(board),
    mOldName(mBoard.getName()),
    mNewName(mOldName),
    mOldInnerLayerCount(mBoard.getInnerLayerCount()),
    mNewInnerLayerCount(mOldInnerLayerCount),
    mOldPcbThickness(mBoard.getPcbThickness()),
    mNewPcbThickness(mOldPcbThickness),
    mOldSolderResist(mBoard.getSolderResist()),
    mNewSolderResist(mOldSolderResist),
    mOldSilkscreenColor(&mBoard.getSilkscreenColor()),
    mNewSilkscreenColor(mOldSilkscreenColor),
    mOldSilkscreenLayersTop(mBoard.getSilkscreenLayersTop()),
    mNewSilkscreenLayersTop(mOldSilkscreenLayersTop),
    mOldSilkscreenLayersBot(mBoard.getSilkscreenLayersBot()),
    mNewSilkscreenLayersBot(mOldSilkscreenLayersBot),
    mOldDesignRules(mBoard.getDesignRules()),
    mNewDesignRules(mOldDesignRules),
    mOldDrcSettings(mBoard.getDrcSettings()),
    mNewDrcSettings(mOldDrcSettings) {
}

CmdBoardEdit::~CmdBoardEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardEdit::setName(const ElementName& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

void CmdBoardEdit::setInnerLayerCount(int count) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewInnerLayerCount = count;
}

void CmdBoardEdit::setPcbThickness(const PositiveLength& thickness) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPcbThickness = thickness;
}

void CmdBoardEdit::setSolderResist(const PcbColor* c) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSolderResist = c;
}
void CmdBoardEdit::setSilkscreenColor(const PcbColor& c) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSilkscreenColor = &c;
}

void CmdBoardEdit::setSilkscreenLayersTop(
    const QVector<const Layer*>& l) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSilkscreenLayersTop = l;
}

void CmdBoardEdit::setSilkscreenLayersBot(
    const QVector<const Layer*>& l) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSilkscreenLayersBot = l;
}

void CmdBoardEdit::setDesignRules(const BoardDesignRules& rules) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDesignRules = rules;
}

void CmdBoardEdit::setDrcSettings(
    const BoardDesignRuleCheckSettings& settings) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDrcSettings = settings;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewInnerLayerCount != mOldInnerLayerCount) return true;
  if (mNewPcbThickness != mOldPcbThickness) return true;
  if (mNewSolderResist != mOldSolderResist) return true;
  if (mNewSilkscreenColor != mOldSilkscreenColor) return true;
  if (mNewSilkscreenLayersTop != mOldSilkscreenLayersTop) return true;
  if (mNewSilkscreenLayersBot != mOldSilkscreenLayersBot) return true;
  if (mNewDesignRules != mOldDesignRules) return true;
  if (mNewDrcSettings != mOldDrcSettings) return true;
  return false;
}

void CmdBoardEdit::performUndo() {
  mBoard.setName(mOldName);
  mBoard.setInnerLayerCount(mOldInnerLayerCount);
  mBoard.setPcbThickness(mOldPcbThickness);
  mBoard.setSolderResist(mOldSolderResist);
  mBoard.setSilkscreenColor(*mOldSilkscreenColor);
  mBoard.setSilkscreenLayersTop(mOldSilkscreenLayersTop);
  mBoard.setSilkscreenLayersBot(mOldSilkscreenLayersBot);
  mBoard.setDesignRules(mOldDesignRules);
  mBoard.setDrcSettings(mOldDrcSettings);
}

void CmdBoardEdit::performRedo() {
  mBoard.setName(mNewName);
  mBoard.setInnerLayerCount(mNewInnerLayerCount);
  mBoard.setPcbThickness(mNewPcbThickness);
  mBoard.setSolderResist(mNewSolderResist);
  mBoard.setSilkscreenColor(*mNewSilkscreenColor);
  mBoard.setSilkscreenLayersTop(mNewSilkscreenLayersTop);
  mBoard.setSilkscreenLayersBot(mNewSilkscreenLayersBot);
  mBoard.setDesignRules(mNewDesignRules);
  mBoard.setDrcSettings(mNewDrcSettings);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
