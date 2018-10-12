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

#ifndef LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COMPONENTSYMBOLS_H
#define LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COMPONENTSYMBOLS_H

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
namespace library {
namespace editor {

namespace Ui {
class NewElementWizardPage_ComponentSymbols;
}

/*******************************************************************************
 *  Class NewElementWizardPage_ComponentSymbols
 ******************************************************************************/

/**
 * @brief The NewElementWizardPage_ComponentSymbols class
 *
 * @author ubruhin
 * @date 2017-03-26
 */
class NewElementWizardPage_ComponentSymbols final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  NewElementWizardPage_ComponentSymbols() = delete;
  NewElementWizardPage_ComponentSymbols(
      const NewElementWizardPage_ComponentSymbols& other) = delete;
  explicit NewElementWizardPage_ComponentSymbols(
      NewElementWizardContext& context, QWidget* parent = 0) noexcept;
  ~NewElementWizardPage_ComponentSymbols() noexcept;

  // Getters
  bool validatePage() noexcept override;
  bool isComplete() const noexcept override;
  int  nextId() const noexcept override;

  // Operator Overloadings
  NewElementWizardPage_ComponentSymbols& operator       =(
      const NewElementWizardPage_ComponentSymbols& rhs) = delete;

private:  // Methods
  void initializePage() noexcept override;
  void cleanupPage() noexcept override;

private:  // Data
  NewElementWizardContext&                                  mContext;
  QScopedPointer<Ui::NewElementWizardPage_ComponentSymbols> mUi;
  ComponentSymbolVariantList                                mSymbolVariantList;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COMPONENTSYMBOLS_H
