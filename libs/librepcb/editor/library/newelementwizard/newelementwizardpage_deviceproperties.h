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

#ifndef LIBREPCB_EDITOR_NEWELEMENTWIZARDPAGE_DEVICEPROPERTIES_H
#define LIBREPCB_EDITOR_NEWELEMENTWIZARDPAGE_DEVICEPROPERTIES_H

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
class NewElementWizardPage_DeviceProperties;
}

/*******************************************************************************
 *  Class NewElementWizardPage_DeviceProperties
 ******************************************************************************/

/**
 * @brief The NewElementWizardPage_DeviceProperties class
 */
class NewElementWizardPage_DeviceProperties final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  NewElementWizardPage_DeviceProperties() = delete;
  NewElementWizardPage_DeviceProperties(
      const NewElementWizardPage_DeviceProperties& other) = delete;
  explicit NewElementWizardPage_DeviceProperties(
      NewElementWizardContext& context, QWidget* parent = 0) noexcept;
  ~NewElementWizardPage_DeviceProperties() noexcept;

  // Getters
  bool isComplete() const noexcept override;
  int nextId() const noexcept override;

  // Operator Overloadings
  NewElementWizardPage_DeviceProperties& operator=(
      const NewElementWizardPage_DeviceProperties& rhs) = delete;

private:  // Methods
  void btnChooseComponentClicked() noexcept;
  void btnChoosePackageClicked() noexcept;
  void setComponent(const std::optional<Uuid>& uuid) noexcept;
  void setPackage(const std::optional<Uuid>& uuid) noexcept;
  void initializePage() noexcept override;
  void cleanupPage() noexcept override;

private:  // Data
  NewElementWizardContext& mContext;
  QScopedPointer<Ui::NewElementWizardPage_DeviceProperties> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
