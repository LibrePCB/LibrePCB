/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef GRIDSETTINGSDIALOG_H
#define GRIDSETTINGSDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../units.h"
#include "../cadview.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace Ui {
class GridSettingsDialog;
}

/*****************************************************************************************
 *  Class GridSettingsDialog
 ****************************************************************************************/

/**
 * @brief The GridSettingsDialog class
 *
 * @author ubruhin
 * @date 2014-10-13
 */
class GridSettingsDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GridSettingsDialog(CADView::GridType type, const Length& interval,
                                    const LengthUnit& unit, QWidget* parent = 0);
        ~GridSettingsDialog();

        // Getters
        CADView::GridType getGridType() const noexcept {return mType;}
        const Length& getGridInterval() const noexcept {return mInterval;}
        const LengthUnit& getUnit() const noexcept {return mUnit;}


    private slots:

        // Private Slots
        void rbtnGroupClicked(int id);
        void spbxIntervalChanged(double arg1);
        void cbxUnitsChanged(int index);
        void buttonBoxClicked(QAbstractButton *button);

    private:

        // make some methods inaccessible...
        GridSettingsDialog();
        GridSettingsDialog(const GridSettingsDialog& other);
        GridSettingsDialog& operator=(const GridSettingsDialog& rhs);

        // Private Methods
        void updateInternalRepresentation() noexcept;

        // General Attributes
        Ui::GridSettingsDialog* mUi;
        CADView::GridType mType;
        Length mInterval;
        LengthUnit mUnit;
};

#endif // GRIDSETTINGSDIALOG_H
