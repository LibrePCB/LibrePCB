/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBREPCB_FIRSTRUNWIZARDPAGE_WORKSPACEPATH_H
#define LIBREPCB_FIRSTRUNWIZARDPAGE_WORKSPACEPATH_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace Ui {
class FirstRunWizardPage_WorkspacePath;
}

/*****************************************************************************************
 *  Class FirstRunWizardPage_WorkspacePath
 ****************************************************************************************/

/**
 * @brief The FirstRunWizardPage_WorkspacePath class
 *
 * @author ubruhin
 * @date 2015-09-22
 */
class FirstRunWizardPage_WorkspacePath final : public QWizardPage
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit FirstRunWizardPage_WorkspacePath(QWidget *parent = 0) noexcept;
        ~FirstRunWizardPage_WorkspacePath() noexcept;

        // Inherited Methods
        bool validatePage() noexcept override;


    private slots:

        // Event Handlers
        void on_rbtnCreateWs_toggled(bool checked);
        void on_rbtnOpenWs_toggled(bool checked);
        void on_btnCreateWsBrowse_clicked();
        void on_btnOpenWsBrowse_clicked();


    private:

        // Private Methods
        Q_DISABLE_COPY(FirstRunWizardPage_WorkspacePath)

        // Private Membervariables
        QScopedPointer<Ui::FirstRunWizardPage_WorkspacePath> mUi;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_FIRSTRUNWIZARDPAGE_WORKSPACEPATH_H
