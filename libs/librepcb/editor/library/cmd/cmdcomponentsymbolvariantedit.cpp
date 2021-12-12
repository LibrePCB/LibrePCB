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
#include "cmdcomponentsymbolvariantedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdComponentSymbolVariantEdit::CmdComponentSymbolVariantEdit(
    ComponentSymbolVariant& variant) noexcept
  : UndoCommand(tr("Edit component symbol variant")),
    mVariant(variant),
    mOldNorm(variant.getNorm()),
    mNewNorm(mOldNorm),
    mOldNames(variant.getNames()),
    mNewNames(mOldNames),
    mOldDescriptions(variant.getDescriptions()),
    mNewDescriptions(mOldDescriptions),
    mOldSymbolItems(variant.getSymbolItems()),
    mNewSymbolItems(mOldSymbolItems) {
}

CmdComponentSymbolVariantEdit::~CmdComponentSymbolVariantEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdComponentSymbolVariantEdit::setNorm(const QString& norm) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewNorm = norm;
}

void CmdComponentSymbolVariantEdit::setNames(
    const LocalizedNameMap& names) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewNames = names;
}

void CmdComponentSymbolVariantEdit::setDescriptions(
    const LocalizedDescriptionMap& descriptions) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDescriptions = descriptions;
}

void CmdComponentSymbolVariantEdit::setSymbolItems(
    const ComponentSymbolVariantItemList& items) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewSymbolItems = items;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdComponentSymbolVariantEdit::performExecute() {
  performRedo();  // can throw

  if (mNewNorm != mOldNorm) return true;
  if (mNewNames != mOldNames) return true;
  if (mNewDescriptions != mOldDescriptions) return true;
  if (mNewSymbolItems != mOldSymbolItems) return true;
  return false;
}

void CmdComponentSymbolVariantEdit::performUndo() {
  mVariant.setNorm(mOldNorm);
  mVariant.setNames(mOldNames);
  mVariant.setDescriptions(mOldDescriptions);
  mVariant.getSymbolItems() = mOldSymbolItems;
}

void CmdComponentSymbolVariantEdit::performRedo() {
  mVariant.setNorm(mNewNorm);
  mVariant.setNames(mNewNames);
  mVariant.setDescriptions(mNewDescriptions);
  mVariant.getSymbolItems() = mNewSymbolItems;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
