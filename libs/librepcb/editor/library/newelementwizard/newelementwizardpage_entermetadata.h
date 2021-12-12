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

#ifndef LIBREPCB_EDITOR_NEWELEMENTWIZARDPAGE_ENTERMETADATA_H
#define LIBREPCB_EDITOR_NEWELEMENTWIZARDPAGE_ENTERMETADATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "newelementwizardcontext.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class NewElementWizardPage_EnterMetadata;
}

/*******************************************************************************
 *  Class NewElementWizardPage_ChooseType
 ******************************************************************************/

/**
 * @brief The NewElementWizardPage_EnterMetadata class
 */
class NewElementWizardPage_EnterMetadata final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  NewElementWizardPage_EnterMetadata() = delete;
  NewElementWizardPage_EnterMetadata(
      const NewElementWizardPage_EnterMetadata& other) = delete;
  explicit NewElementWizardPage_EnterMetadata(NewElementWizardContext& context,
                                              QWidget* parent = 0) noexcept;
  ~NewElementWizardPage_EnterMetadata() noexcept;

  // Getters
  bool isComplete() const noexcept override;
  int nextId() const noexcept override;

  // Operator Overloadings
  NewElementWizardPage_EnterMetadata& operator=(
      const NewElementWizardPage_EnterMetadata& rhs) = delete;

private:  // Methods
  void edtNameTextChanged(const QString& text) noexcept;
  void edtDescriptionTextChanged() noexcept;
  void edtKeywordsTextChanged(const QString& text) noexcept;
  void edtAuthorTextChanged(const QString& text) noexcept;
  void edtVersionTextChanged(const QString& text) noexcept;
  void btnChooseCategoryClicked() noexcept;
  void btnResetCategoryClicked() noexcept;
  void updateCategoryTreeLabel() noexcept;
  void initializePage() noexcept override;
  void cleanupPage() noexcept override;

private:  // Data
  NewElementWizardContext& mContext;
  QScopedPointer<Ui::NewElementWizardPage_EnterMetadata> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
