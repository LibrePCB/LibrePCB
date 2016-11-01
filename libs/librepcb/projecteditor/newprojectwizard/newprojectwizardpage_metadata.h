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

#ifndef LIBREPCB_PROJECT_NEWPROJECTWIZARDPAGE_METADATA_H
#define LIBREPCB_PROJECT_NEWPROJECTWIZARDPAGE_METADATA_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/

namespace librepcb {
namespace project {
namespace editor {

namespace Ui {
class NewProjectWizardPage_Metadata;
}

/*****************************************************************************************
 *  Class NewProjectWizardPage_Metadata
 ****************************************************************************************/

/**
 * @brief The NewProjectWizardPage_Metadata class
 *
 * @author ubruhin
 * @date 2016-08-13
 */
class NewProjectWizardPage_Metadata final : public QWizardPage
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit NewProjectWizardPage_Metadata(QWidget* parent = nullptr) noexcept;
        NewProjectWizardPage_Metadata(const NewProjectWizardPage_Metadata& other) = delete;
        ~NewProjectWizardPage_Metadata() noexcept;

        // Setters
        void setDefaultLocation(const FilePath& dir) noexcept;

        // Getters
        QString getProjectName() const noexcept;
        QString getProjectAuthor() const noexcept;
        bool isLicenseSet() const noexcept;
        FilePath getProjectLicenseFilePath() const noexcept;
        FilePath getFullFilePath() const noexcept;

        // Operator Overloadings
        NewProjectWizardPage_Metadata& operator=(const NewProjectWizardPage_Metadata& rhs) = delete;


    private: // GUI Action Handlers

        void nameChanged(const QString& name) noexcept;
        void locationChanged(const QString& dir) noexcept;
        void chooseLocationClicked() noexcept;


    private: // Methods

        void updateProjectFilePath() noexcept;
        bool isComplete() const noexcept override;
        bool validatePage() noexcept override;


    private: // Data

        QScopedPointer<Ui::NewProjectWizardPage_Metadata> mUi;
        FilePath mFullFilePath;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_NEWPROJECTWIZARDPAGE_METADATA_H
