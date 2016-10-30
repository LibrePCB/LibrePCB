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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEPADCOMBOBOX_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEPADCOMBOBOX_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/library/pkg/package.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Class PackagePadComboBox
 ****************************************************************************************/

/**
 * @brief The PackagePadComboBox class
 *
 * @author ubruhin
 * @date 2017-08-16
 */
class PackagePadComboBox final : public QWidget
{
        Q_OBJECT

    public:
        // Constructors / Destructor
        explicit PackagePadComboBox(QWidget* parent = nullptr) noexcept;
        PackagePadComboBox(const PackagePadComboBox& other) = delete;
        ~PackagePadComboBox() noexcept;

        // Getters
        PackagePad* getCurrentPad() const noexcept;

        // Setters
        void setPackage(Package* package, Footprint* footprint = nullptr) noexcept;
        void setCurrentPad(PackagePad* pad) noexcept;

        // General Methods
        void updatePads() noexcept;

        // Operator Overloadings
        PackagePadComboBox& operator=(const PackagePadComboBox& rhs) = delete;


    signals:
        void currentPadChanged(PackagePad* pad);


    private: // Methods
        void currentIndexChanged(int index) noexcept;


    private: // Data
        Package* mPackage;
        Footprint* mFootprint;
        QComboBox* mComboBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_PACKAGEPADCOMBOBOX_H
