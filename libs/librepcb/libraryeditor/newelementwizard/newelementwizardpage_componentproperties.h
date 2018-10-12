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

#ifndef LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COMPONENTPROPERTIES_H
#define LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COMPONENTPROPERTIES_H

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
class NewElementWizardPage_ComponentProperties;
}

/*******************************************************************************
 *  Class NewElementWizardPage_ComponentProperties
 ******************************************************************************/

/**
 * @brief The NewElementWizardPage_ComponentProperties class
 *
 * @author ubruhin
 * @date 2017-03-26
 */
class NewElementWizardPage_ComponentProperties final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  NewElementWizardPage_ComponentProperties() = delete;
  NewElementWizardPage_ComponentProperties(
      const NewElementWizardPage_ComponentProperties& other) = delete;
  explicit NewElementWizardPage_ComponentProperties(
      NewElementWizardContext& context, QWidget* parent = 0) noexcept;
  ~NewElementWizardPage_ComponentProperties() noexcept;

  // Getters
  bool validatePage() noexcept override;
  bool isComplete() const noexcept override;
  int  nextId() const noexcept override;

  // Operator Overloadings
  NewElementWizardPage_ComponentProperties& operator       =(
      const NewElementWizardPage_ComponentProperties& rhs) = delete;

private:  // Methods
  void initializePage() noexcept override;
  void cleanupPage() noexcept override;

private:  // Data
  NewElementWizardContext&                                     mContext;
  QScopedPointer<Ui::NewElementWizardPage_ComponentProperties> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COMPONENTPROPERTIES_H
