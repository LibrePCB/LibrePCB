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

#ifndef LIBREPCB_EDITOR_COMPONENTTAB_H
#define LIBREPCB_EDITOR_COMPONENTTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryeditortab.h"

#include <librepcb/core/library/cmp/componentprefix.h>
#include <librepcb/core/library/cmp/componentsymbolvariant.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;

namespace editor {

class AttributeListModel;
class CategoryTreeModel;
class ComponentSignalListModel;
class ComponentSignalNameListModel;
class ComponentVariantListModel;
class LibraryEditor;
class LibraryElementCategoriesModel;

/*******************************************************************************
 *  Class ComponentTab
 ******************************************************************************/

/**
 * @brief The ComponentTab class
 */
class ComponentTab final : public LibraryEditorTab {
  Q_OBJECT

public:
  // Signals
  Signal<ComponentTab> onDerivedUiDataChanged;

  // Types
  enum class Mode { Open, New, Duplicate };

  // Constructors / Destructor
  ComponentTab() = delete;
  ComponentTab(const ComponentTab& other) = delete;
  explicit ComponentTab(LibraryEditor& editor, std::unique_ptr<Component> cmp,
                        Mode mode, QObject* parent = nullptr) noexcept;
  ~ComponentTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  ui::ComponentTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::ComponentTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;
  slint::Image renderScene(float width, float height,
                           int scene) noexcept override;
  bool requestClose() noexcept override;

  // Operator Overloadings
  ComponentTab& operator=(const ComponentTab& rhs) = delete;

protected:
  std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
      runChecksImpl() override;
  bool autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                   bool checkOnly) override;
  template <typename MessageType>
  bool autoFixHelper(const std::shared_ptr<const RuleCheckMessage>& msg,
                     bool checkOnly);
  template <typename MessageType>
  void autoFix(const MessageType& msg);
  void messageApprovalChanged(const SExpression& approval,
                              bool approved) noexcept override;
  void notifyDerivedUiDataChanged() noexcept override;

private:
  bool isWritable() const noexcept;
  bool isInterfaceBroken() const noexcept;
  void refreshUiData() noexcept;
  void commitUiData() noexcept;
  bool save() noexcept;

private:
  // References
  std::unique_ptr<Component> mComponent;
  const bool mIsNewElement;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  bool mChooseCategory;

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
  std::shared_ptr<LibraryElementCategoriesModel> mCategories;
  std::shared_ptr<CategoryTreeModel> mCategoriesTree;
  slint::SharedString mDatasheetUrl;
  slint::SharedString mDatasheetUrlError;
  bool mSchematicOnly;
  slint::SharedString mPrefix;
  slint::SharedString mPrefixError;
  ComponentPrefix mPrefixParsed;
  slint::SharedString mDefaultValue;
  slint::SharedString mDefaultValueError;
  std::shared_ptr<AttributeListModel> mAttributes;
  std::shared_ptr<ComponentSignalListModel> mSignals;
  std::shared_ptr<slint::SortModel<ui::ComponentSignalData>> mSignalsSorted;
  std::shared_ptr<ComponentSignalNameListModel> mSignalNames;
  slint::SharedString mNewSignalName;
  slint::SharedString mNewSignalNameError;
  std::shared_ptr<ComponentVariantListModel> mVariants;

  /// Broken interface detection
  bool mIsInterfaceBroken;
  bool mOriginalIsSchematicOnly;
  QSet<Uuid> mOriginalSignalUuids;
  ComponentSymbolVariantList mOriginalSymbolVariants;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
