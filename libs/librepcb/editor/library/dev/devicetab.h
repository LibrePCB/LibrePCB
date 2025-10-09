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

#ifndef LIBREPCB_EDITOR_DEVICETAB_H
#define LIBREPCB_EDITOR_DEVICETAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../libraryeditortab.h"

#include <librepcb/core/library/dev/devicepadsignalmap.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Device;
class Package;
class Symbol;

namespace editor {

class AttributeListModel;
class CategoryTreeModel;
class ComponentSignalNameListModel;
class DevicePinoutBuilder;
class DevicePinoutListModel;
class FootprintGraphicsItem;
class GraphicsScene;
class LibraryEditor;
class LibraryElementCategoriesModel;
class PartListModel;
class SymbolGraphicsItem;

/*******************************************************************************
 *  Class DeviceTab
 ******************************************************************************/

/**
 * @brief The DeviceTab class
 */
class DeviceTab final : public LibraryEditorTab {
  Q_OBJECT

public:
  // Signals
  Signal<DeviceTab> onDerivedUiDataChanged;

  // Types
  enum class Mode { Open, New, Duplicate };

  // Constructors / Destructor
  DeviceTab() = delete;
  DeviceTab(const DeviceTab& other) = delete;
  explicit DeviceTab(LibraryEditor& editor, std::unique_ptr<Device> dev,
                     Mode mode, QObject* parent = nullptr) noexcept;
  ~DeviceTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept override;
  ui::TabData getUiData() const noexcept override;
  ui::DeviceTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::DeviceTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;
  slint::Image renderScene(float width, float height,
                           int scene) noexcept override;
  bool requestClose() noexcept override;

  // Operator Overloadings
  DeviceTab& operator=(const DeviceTab& rhs) = delete;

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
  void refreshDependentElements() noexcept;
  void commitUiData() noexcept;
  bool save() noexcept;
  void selectComponent() noexcept;
  void selectPackage() noexcept;

private:
  // References
  std::unique_ptr<Device> mDevice;
  const Mode mMode;
  const bool mIsNewElement;
  QCollator mCollator;
  std::unique_ptr<DevicePinoutBuilder> mPinoutBuilder;
  std::shared_ptr<ComponentSignalNameListModel> mSignalNames;
  std::unique_ptr<GraphicsScene> mComponentScene;
  std::unique_ptr<GraphicsScene> mPackageScene;

  // State
  bool mWizardMode;
  int mCurrentPageIndex;
  bool mComponentSelected;
  bool mPackageSelected;
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
  std::shared_ptr<AttributeListModel> mAttributes;
  std::shared_ptr<DevicePinoutListModel> mPinout;
  std::shared_ptr<slint::SortModel<ui::DevicePinoutData>> mPinoutSorted;
  std::shared_ptr<PartListModel> mParts;

  /// Broken interface detection
  bool mIsInterfaceBroken;
  Uuid mOriginalComponentUuid;
  Uuid mOriginalPackageUuid;
  DevicePadSignalMap mOriginalPadSignalMap;

  // Referenced library elements
  std::shared_ptr<const Component> mComponent;  // May be nullptr!
  QString mComponentDescription;  // Error message on failure
  QList<std::shared_ptr<const Symbol>> mSymbols;
  QList<std::shared_ptr<SymbolGraphicsItem>> mSymbolGraphicsItems;
  std::shared_ptr<const Package> mPackage;  // May be nullptr!
  QString mPackageDescription;  // Error message on failure
  std::unique_ptr<FootprintGraphicsItem> mFootprintGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
