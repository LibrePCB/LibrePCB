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

#ifndef FIRSTRUNWIZARD_H
#define FIRSTRUNWIZARD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/fileio/filepath.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace Ui {
class FirstRunWizard;
}

/*****************************************************************************************
 *  Class FirstRunWizard
 ****************************************************************************************/

/**
 * @brief The FirstRunWizard class
 *
 * @author ubruhin
 * @date 2015-09-22
 */
class FirstRunWizard final : public QWizard
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit FirstRunWizard(QWidget* parent = 0) noexcept;
        ~FirstRunWizard() noexcept;

        // Getters
        bool getCreateNewWorkspace() const noexcept;
        FilePath getWorkspaceFilePath() const noexcept;


    private:

        // make some methods inaccessible...
        FirstRunWizard(const FirstRunWizard& other) = delete;
        FirstRunWizard& operator=(const FirstRunWizard& rhs) = delete;

        // General
        Ui::FirstRunWizard* mUi;
};

#endif // FIRSTRUNWIZARD_H
