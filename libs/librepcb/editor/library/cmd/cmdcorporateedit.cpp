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
#include "cmdcorporateedit.h"

#include <librepcb/core/library/corp/corporate.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdCorporateEdit::CmdCorporateEdit(Corporate& corporate) noexcept
  : CmdLibraryBaseElementEdit(corporate, tr("Edit Corporate Properties")),
    mCorporate(corporate),
    mOldLogo(corporate.getLogoPng()),
    mNewLogo(mOldLogo),
    mOldUrl(corporate.getUrl()),
    mNewUrl(mOldUrl),
    mOldPriority(corporate.getPriority()),
    mNewPriority(mOldPriority),
    mOldPcbProducts(corporate.getPcbProducts()),
    mNewPcbProducts(mOldPcbProducts),
    mOldPcbOutputJobs(corporate.getPcbOutputJobs()),
    mNewPcbOutputJobs(mOldPcbOutputJobs),
    mOldAssemblyOutputJobs(corporate.getAssemblyOutputJobs()),
    mNewAssemblyOutputJobs(mOldAssemblyOutputJobs) {
}

CmdCorporateEdit::~CmdCorporateEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdCorporateEdit::setLogoPng(const QByteArray& png) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLogo = png;
}

void CmdCorporateEdit::setUrl(const QUrl& url) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewUrl = url;
}

void CmdCorporateEdit::setPriority(int priority) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPriority = priority;
}

void CmdCorporateEdit::setPcbProducts(
    const QVector<CorporatePcbProduct>& list) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPcbProducts = list;
}

void CmdCorporateEdit::setPcbOutputJobs(const OutputJobList& jobs) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPcbOutputJobs = jobs;
}

void CmdCorporateEdit::setAssemblyOutputJobs(
    const OutputJobList& jobs) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAssemblyOutputJobs = jobs;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCorporateEdit::performExecute() {
  if (CmdLibraryBaseElementEdit::performExecute()) return true;  // can throw
  if (mNewLogo != mOldLogo) return true;
  if (mNewUrl != mOldUrl) return true;
  if (mNewPriority != mOldPriority) return true;
  if (mNewPcbProducts != mOldPcbProducts) return true;
  if (mNewPcbOutputJobs != mOldPcbOutputJobs) return true;
  if (mNewAssemblyOutputJobs != mOldAssemblyOutputJobs) return true;
  return false;
}

void CmdCorporateEdit::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
  mCorporate.setLogoPng(mOldLogo);
  mCorporate.setUrl(mOldUrl);
  mCorporate.setPriority(mOldPriority);
  mCorporate.setPcbProducts(mOldPcbProducts);
  mCorporate.setPcbOutputJobs(mOldPcbOutputJobs);
  mCorporate.setAssemblyOutputJobs(mOldAssemblyOutputJobs);
}

void CmdCorporateEdit::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
  mCorporate.setLogoPng(mNewLogo);
  mCorporate.setUrl(mNewUrl);
  mCorporate.setPriority(mNewPriority);
  mCorporate.setPcbProducts(mNewPcbProducts);
  mCorporate.setPcbOutputJobs(mNewPcbOutputJobs);
  mCorporate.setAssemblyOutputJobs(mNewAssemblyOutputJobs);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
