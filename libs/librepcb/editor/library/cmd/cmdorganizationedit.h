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

#ifndef LIBREPCB_EDITOR_CMDORGANIZATIONEDIT_H
#define LIBREPCB_EDITOR_CMDORGANIZATIONEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibrarybaseelementedit.h"

#include <librepcb/core/job/outputjob.h>
#include <librepcb/core/library/org/organizationpcbdesignrules.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Organization;

namespace editor {

/*******************************************************************************
 *  Class CmdOrganizationEdit
 ******************************************************************************/

/**
 * @brief The CmdOrganizationEdit class
 */
class CmdOrganizationEdit final : public CmdLibraryBaseElementEdit {
public:
  // Constructors / Destructor
  CmdOrganizationEdit() = delete;
  CmdOrganizationEdit(const CmdOrganizationEdit& other) = delete;
  explicit CmdOrganizationEdit(Organization& organization) noexcept;
  ~CmdOrganizationEdit() noexcept;

  // Setters
  void setLogoPng(const QByteArray& png) noexcept;
  void setUrl(const QUrl& url) noexcept;
  void setPriority(int priority) noexcept;
  void setPcbDesignRules(
      const QVector<OrganizationPcbDesignRules>& list) noexcept;
  void setPcbOutputJobs(const OutputJobList& jobs) noexcept;
  void setAssemblyOutputJobs(const OutputJobList& jobs) noexcept;

  // Operator Overloadings
  CmdOrganizationEdit& operator=(const CmdOrganizationEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  Organization& mOrganization;

  QByteArray mOldLogo;
  QByteArray mNewLogo;
  QUrl mOldUrl;
  QUrl mNewUrl;
  int mOldPriority;
  int mNewPriority;
  QVector<OrganizationPcbDesignRules> mOldPcbDesignRules;
  QVector<OrganizationPcbDesignRules> mNewPcbDesignRules;
  OutputJobList mOldPcbOutputJobs;
  OutputJobList mNewPcbOutputJobs;
  OutputJobList mOldAssemblyOutputJobs;
  OutputJobList mNewAssemblyOutputJobs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
