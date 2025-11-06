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

#ifndef LIBREPCB_EDITOR_CMDCORPORATEEDIT_H
#define LIBREPCB_EDITOR_CMDCORPORATEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibrarybaseelementedit.h"

#include <librepcb/core/job/outputjob.h>
#include <librepcb/core/library/corp/corporatepcbproduct.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Corporate;

namespace editor {

/*******************************************************************************
 *  Class CmdCorporateEdit
 ******************************************************************************/

/**
 * @brief The CmdCorporateEdit class
 */
class CmdCorporateEdit final : public CmdLibraryBaseElementEdit {
public:
  // Constructors / Destructor
  CmdCorporateEdit() = delete;
  CmdCorporateEdit(const CmdCorporateEdit& other) = delete;
  explicit CmdCorporateEdit(Corporate& corporate) noexcept;
  ~CmdCorporateEdit() noexcept;

  // Setters
  void setLogoPng(const QByteArray& png) noexcept;
  void setUrl(const QUrl& url) noexcept;
  void setPriority(int priority) noexcept;
  void setPcbProducts(const QVector<CorporatePcbProduct>& list) noexcept;
  void setPcbOutputJobs(const OutputJobList& jobs) noexcept;
  void setAssemblyOutputJobs(const OutputJobList& jobs) noexcept;

  // Operator Overloadings
  CmdCorporateEdit& operator=(const CmdCorporateEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  Corporate& mCorporate;

  QByteArray mOldLogo;
  QByteArray mNewLogo;
  QUrl mOldUrl;
  QUrl mNewUrl;
  int mOldPriority;
  int mNewPriority;
  QVector<CorporatePcbProduct> mOldPcbProducts;
  QVector<CorporatePcbProduct> mNewPcbProducts;
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
