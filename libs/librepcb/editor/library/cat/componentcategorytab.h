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

#ifndef LIBREPCB_EDITOR_COMPONENTCATEGORYTAB_H
#define LIBREPCB_EDITOR_COMPONENTCATEGORYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryeditortab.h"

#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentCategory;

namespace editor {

class CategoryTreeModel;

/*******************************************************************************
 *  Class ComponentCategoryTab
 ******************************************************************************/

/**
 * @brief The ComponentCategoryTab class
 */
class ComponentCategoryTab final : public LibraryEditorTab {
  Q_OBJECT

public:
  // Signals
  Signal<ComponentCategoryTab> onDerivedUiDataChanged;

  // Types
  enum class Mode { Open, New, Duplicate };

  // Constructors / Destructor
  ComponentCategoryTab() = delete;
  ComponentCategoryTab(const ComponentCategoryTab& other) = delete;
  explicit ComponentCategoryTab(LibraryEditor& editor,
                                std::unique_ptr<ComponentCategory> cat,
                                Mode mode, QObject* parent = nullptr) noexcept;
  ~ComponentCategoryTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  ui::CategoryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::CategoryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;
  bool requestClose() noexcept override;

  // Operator Overloadings
  ComponentCategoryTab& operator=(const ComponentCategoryTab& rhs) = delete;

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

private:
  // References
  std::unique_ptr<ComponentCategory> mCategory;

  // State
  bool mChooseParent;

  // Library metadata to be applied
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
  std::optional<Uuid> mParent;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mParents;
  std::shared_ptr<CategoryTreeModel> mParentsModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
