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

#ifndef LIBREPCB_EDITOR_PACKAGECATEGORYTAB_H
#define LIBREPCB_EDITOR_PACKAGECATEGORYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../windowtab.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PackageCategory;

namespace editor {

class CategoryTreeModel2;
class LibraryEditor2;
class UndoStack;

/*******************************************************************************
 *  Class PackageCategoryTab
 ******************************************************************************/

/**
 * @brief The PackageCategoryTab class
 */
class PackageCategoryTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<PackageCategoryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  PackageCategoryTab() = delete;
  PackageCategoryTab(const PackageCategoryTab& other) = delete;
  explicit PackageCategoryTab(LibraryEditor2& editor,
                              std::unique_ptr<PackageCategory> cat,
                              QObject* parent = nullptr) noexcept;
  ~PackageCategoryTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept;
  ui::TabData getUiData() const noexcept override;
  ui::CategoryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::CategoryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  PackageCategoryTab& operator=(const PackageCategoryTab& rhs) = delete;

private:  // Methods
  void refreshMetadata() noexcept;
  void commitMetadata() noexcept;

private:
  // References
  LibraryEditor2& mEditor;
  std::unique_ptr<PackageCategory> mCategory;
  std::unique_ptr<UndoStack> mUndoStack;

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

  // UI Data
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mParents;
  std::shared_ptr<CategoryTreeModel2> mParentsModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
