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

#ifndef LIBREPCB_APPLICATION_FIRSTRUNWIZARDPAGE_WORKSPACESETTINGS_H
#define LIBREPCB_APPLICATION_FIRSTRUNWIZARDPAGE_WORKSPACESETTINGS_H

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
class FirstRunWizardPage_WorkspaceSettings;
}

/*******************************************************************************
 *  Class FirstRunWizardPage_WorkspaceSettings
 ******************************************************************************/

/**
 * @brief The FirstRunWizardPage_WorkspaceSettings class
 */
class FirstRunWizardPage_WorkspaceSettings final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit FirstRunWizardPage_WorkspaceSettings(QWidget *parent = 0) noexcept;
  ~FirstRunWizardPage_WorkspaceSettings() noexcept;

  // Inherited Methods
  bool validatePage() noexcept override;

private:  // Methods
  Q_DISABLE_COPY(FirstRunWizardPage_WorkspaceSettings)

private:  // Data
  QScopedPointer<Ui::FirstRunWizardPage_WorkspaceSettings> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif  // LIBREPCB_APPLICATION_FIRSTRUNWIZARDPAGE_WORKSPACESETTINGS_H
