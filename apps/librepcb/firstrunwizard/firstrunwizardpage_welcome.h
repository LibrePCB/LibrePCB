/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_FIRSTRUNWIZARDPAGE_WELCOME_H
#define LIBREPCB_FIRSTRUNWIZARDPAGE_WELCOME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace application {

namespace Ui {
class FirstRunWizardPage_Welcome;
}

/*******************************************************************************
 *  Class FirstRunWizardPage_Welcome
 ******************************************************************************/

/**
 * @brief The FirstRunWizardPage_Welcome class
 *
 * @author ubruhin
 * @date 2015-09-22
 */
class FirstRunWizardPage_Welcome final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit FirstRunWizardPage_Welcome(QWidget *parent = 0) noexcept;
  ~FirstRunWizardPage_Welcome() noexcept;

private:
  // Private Methods
  Q_DISABLE_COPY(FirstRunWizardPage_Welcome)

  // Private Membervariables
  QScopedPointer<Ui::FirstRunWizardPage_Welcome> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif  // LIBREPCB_FIRSTRUNWIZARDPAGE_WELCOME_H
