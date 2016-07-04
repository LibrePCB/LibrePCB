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

#ifndef LIBREPCB_WSI_APPDEFAULTMEASUREMENTUNITS_H
#define LIBREPCB_WSI_APPDEFAULTMEASUREMENTUNITS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "wsi_base.h"
#include <librepcbcommon/units/lengthunit.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Class WSI_AppDefaultMeasurementUnits
 ****************************************************************************************/

/**
 * @brief The WSI_AppDefaultMeasurementUnits class represents the application's default
 *        measurement units (for example the application's default length unit)
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_AppDefaultMeasurementUnits final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_AppDefaultMeasurementUnits(WorkspaceSettings& settings);
        ~WSI_AppDefaultMeasurementUnits();

        // Getters
        const LengthUnit& getLengthUnit() const {return mLengthUnit;}

        // Getters: Widgets
        QString getLengthUnitLabelText() const {return tr("Default Length Unit:");}
        QComboBox* getLengthUnitComboBox() const {return mLengthUnitComboBox;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();


    public slots:

        // Public Slots
        void lengthUnitComboBoxIndexChanged(int index);


    private:

        // make some methods inaccessible...
        WSI_AppDefaultMeasurementUnits();
        WSI_AppDefaultMeasurementUnits(const WSI_AppDefaultMeasurementUnits& other);
        WSI_AppDefaultMeasurementUnits& operator=(const WSI_AppDefaultMeasurementUnits& rhs);


        // Private Methods
        void updateLengthUnitComboBoxIndex();


        // General Attributes

        /**
         * @brief The application's default length unit
         *
         * Default: millimeters
         */
        LengthUnit mLengthUnit;
        LengthUnit mLengthUnitTmp;

        // Widgets
        QComboBox* mLengthUnitComboBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_APPDEFAULTMEASUREMENTUNITS_H
