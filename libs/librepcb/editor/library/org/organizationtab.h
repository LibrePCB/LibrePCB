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

#ifndef LIBREPCB_EDITOR_ORGANIZATIONTAB_H
#define LIBREPCB_EDITOR_ORGANIZATIONTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryeditortab.h"

#include <librepcb/core/job/outputjob.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Organization;
class Project;

namespace editor {

class CmdOrganizationEdit;
class OrganizationPcbDesignRulesModel;

/*******************************************************************************
 *  Class OrganizationTab
 ******************************************************************************/

/**
 * @brief The OrganizationTab class
 */
class OrganizationTab final : public LibraryEditorTab {
  Q_OBJECT

public:
  // Signals
  Signal<OrganizationTab> onDerivedUiDataChanged;

  // Types
  enum class Mode { Open, New, Duplicate };

  // Constructors / Destructor
  OrganizationTab() = delete;
  OrganizationTab(const OrganizationTab& other) = delete;
  explicit OrganizationTab(LibraryEditor& editor,
                           std::unique_ptr<Organization> cat, Mode mode,
                           QObject* parent = nullptr) noexcept;
  ~OrganizationTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  ui::OrganizationTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::OrganizationTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;
  bool requestClose() noexcept override;

  // Operator Overloadings
  OrganizationTab& operator=(const OrganizationTab& rhs) = delete;

protected:
  std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
      runChecksImpl() override;
  bool autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                   bool checkOnly) override;
  template <typename MessageType>
  bool autoFixHelper(const std::shared_ptr<const RuleCheckMessage>& msg,
                     bool checkOnly);
  template <typename MessageType>
  bool autoFix(const MessageType& msg);
  void messageApprovalChanged(const SExpression& approval,
                              bool approved) noexcept override;
  void notifyDerivedUiDataChanged() noexcept override;

private:
  bool isWritable() const noexcept;
  void refreshUiData() noexcept;
  void commitUiData() noexcept;
  bool save() noexcept;
  void execOutputJobsDialog(
      const OutputJobList& jobs,
      void (CmdOrganizationEdit::*setter)(const OutputJobList& jobs)) noexcept;
  Project& getTmpProject();

private:
  // References
  std::unique_ptr<Organization> mOrganization;

  // Library metadata to be applied
  QByteArray mLogo;
  slint::SharedString mName;
  slint::SharedString mNameError;
  ElementName mNameParsed;
  slint::SharedString mDescription;
  slint::SharedString mKeywords;
  slint::SharedString mAuthor;
  slint::SharedString mVersion;
  slint::SharedString mVersionError;
  Version mVersionParsed;
  bool mDeprecated;
  slint::SharedString mUrl;
  slint::SharedString mUrlError;
  int mPriority;
  std::shared_ptr<OrganizationPcbDesignRulesModel> mPcbDesignRules;

  // Cached
  std::unique_ptr<Project> mTmpProject;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
