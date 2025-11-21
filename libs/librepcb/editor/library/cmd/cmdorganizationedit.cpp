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
#include "cmdorganizationedit.h"

#include <librepcb/core/library/org/organization.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdOrganizationEdit::CmdOrganizationEdit(Organization& organization) noexcept
  : CmdLibraryBaseElementEdit(organization, tr("Edit Organization Properties")),
    mOrganization(organization),
    mOldLogo(organization.getLogoPng()),
    mNewLogo(mOldLogo),
    mOldUrl(organization.getUrl()),
    mNewUrl(mOldUrl),
    mOldPriority(organization.getPriority()),
    mNewPriority(mOldPriority),
    mOldPcbDesignRules(organization.getPcbDesignRules()),
    mNewPcbDesignRules(mOldPcbDesignRules),
    mOldPcbOutputJobs(organization.getPcbOutputJobs()),
    mNewPcbOutputJobs(mOldPcbOutputJobs),
    mOldAssemblyOutputJobs(organization.getAssemblyOutputJobs()),
    mNewAssemblyOutputJobs(mOldAssemblyOutputJobs) {
}

CmdOrganizationEdit::~CmdOrganizationEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdOrganizationEdit::setLogoPng(const QByteArray& png) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLogo = png;
}

void CmdOrganizationEdit::setUrl(const QUrl& url) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewUrl = url;
}

void CmdOrganizationEdit::setPriority(int priority) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPriority = priority;
}

void CmdOrganizationEdit::setPcbDesignRules(
    const QVector<OrganizationPcbDesignRules>& list) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPcbDesignRules = list;
}

void CmdOrganizationEdit::setPcbOutputJobs(const OutputJobList& jobs) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPcbOutputJobs = jobs;
}

void CmdOrganizationEdit::setAssemblyOutputJobs(
    const OutputJobList& jobs) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewAssemblyOutputJobs = jobs;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdOrganizationEdit::performExecute() {
  if (CmdLibraryBaseElementEdit::performExecute()) return true;  // can throw
  if (mNewLogo != mOldLogo) return true;
  if (mNewUrl != mOldUrl) return true;
  if (mNewPriority != mOldPriority) return true;
  if (mNewPcbDesignRules != mOldPcbDesignRules) return true;
  if (mNewPcbOutputJobs != mOldPcbOutputJobs) return true;
  if (mNewAssemblyOutputJobs != mOldAssemblyOutputJobs) return true;
  return false;
}

void CmdOrganizationEdit::performUndo() {
  CmdLibraryBaseElementEdit::performUndo();  // can throw
  mOrganization.setLogoPng(mOldLogo);
  mOrganization.setUrl(mOldUrl);
  mOrganization.setPriority(mOldPriority);
  mOrganization.setPcbDesignRules(mOldPcbDesignRules);
  mOrganization.setPcbOutputJobs(mOldPcbOutputJobs);
  mOrganization.setAssemblyOutputJobs(mOldAssemblyOutputJobs);
}

void CmdOrganizationEdit::performRedo() {
  CmdLibraryBaseElementEdit::performRedo();  // can throw
  mOrganization.setLogoPng(mNewLogo);
  mOrganization.setUrl(mNewUrl);
  mOrganization.setPriority(mNewPriority);
  mOrganization.setPcbDesignRules(mNewPcbDesignRules);
  mOrganization.setPcbOutputJobs(mNewPcbOutputJobs);
  mOrganization.setAssemblyOutputJobs(mNewAssemblyOutputJobs);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
