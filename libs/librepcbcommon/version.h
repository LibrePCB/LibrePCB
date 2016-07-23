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

#ifndef LIBREPCB_VERSION_H
#define LIBREPCB_VERSION_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Version
 ****************************************************************************************/

/**
 * @brief The Version class represents a version number in the format "1.42.7"
 *
 * Each #Version instance can either be valid or invalid (see #isValid()).
 * Rules for a valid version:
 *  - Minimum count of numbers: 1 (example: "15")
 *  - Maximum count of numbers: 10 (example: "31.41.5.926.5358.97.9.3.238.462")
 *  - Minimum count of digits of a number: 1
 *  - Maximum count of digits of a number: 5
 *
 * So the lowest possible version is "0", and the highest possible version is
 * "99999.99999.99999.99999.99999.99999.99999.99999.99999.99999".
 *
 * Leading zeros in numbers are ignored: "002.0005" will be converted to "2.5"
 * Trailing zero numbers are ignored: "2.5.0.0" will be converted to "2.5"
 *
 * @author ubruhin
 * @date 2014-10-30
 *
 * @todo Test all methods of this class! No idea if this class works as expected!
 */
class Version final
{
    public:

        // Constructors / Destructor

        /**
         * @brief Default constructor (creates an invalid #Version object)
         */
        Version() noexcept;

        /**
         * @brief Constructor which creates a #Version object from a version string
         *
         * @param version   See #setVersion()
         */
        explicit Version(const QString& version) noexcept;

        /**
         * @brief Copy constructor
         *
         * @param other     Another #Version object
         */
        Version(const Version& other) noexcept;

        /**
         * Destructor
         */
        ~Version() noexcept;


        // Getters

        /**
         * @brief Check if the object represents a valid version number
         *
         * @return true = valid, false = invalid
         */
        bool isValid() const noexcept {return (mNumbers.count() > 0);}

        /**
         * @brief Get the numbers in the version string
         *
         * The first item in the list is the major version number.
         *
         * @return List of numbers (empty list = invalid version)
         */
        const QList<int>& getNumbers() const noexcept {return mNumbers;}

        /**
         * @brief Get the version as a string in the format "1.2.3"
         *
         * @return The version as a string (empty string = invalid version)
         */
        QString toStr() const noexcept;

        /**
         * @brief Get the version as a comparable string (59 characters)
         *
         * The version will be returned with all 10x5 decimal places:
         * "#####.#####.#####.#####.#####.#####.#####.#####.#####.#####"
         *
         * This method is useful to compare versions in a database (e.g. SQLite) as you
         * only need a simple string compare.
         *
         * @return The version as a comparable string (empty string = invalid version)
         */
        QString toComparableStr() const noexcept;


        // Setters

        /**
         * @brief Set the version of the object from a string
         *
         * If the version string is valid, the object will be valid too. If the string
         * does not contain a valid version, the object will be invalid.
         *
         * @param version   The version string in the format "1.2.3" (variable count of numbers)
         *
         * @return validity of the version (true = valid, false = invalid)
         */
        bool setVersion(const QString& version) noexcept;


        //@{
        /**
         * @brief Operator overloadings
         *
         * @param rhs   The other object to compare
         *
         * @return If at least one of both objects is invalid, false will be returned!
         */
        Version& operator=(const Version& rhs) noexcept;
        bool operator>(const Version& rhs) const noexcept;
        bool operator<(const Version& rhs) const  noexcept;
        bool operator>=(const Version& rhs) const noexcept;
        bool operator<=(const Version& rhs) const noexcept;
        bool operator==(const Version& rhs) const noexcept;
        bool operator!=(const Version& rhs) const noexcept;
        //@}


    private:

        // Private Methods

        /**
         * @brief Compare method for operator overloadings
         *
         * @param other     The other version for the comparison
         *
         * @return -1:  this < other
         *          0:  this == other (or at least one of the versions is invalid)
         *          1:  this > other
         */
        int compare(const Version& other) const noexcept;


        // Attributes

        /**
         * @brief List of all version numbers of the whole version
         *
         * number count == 0: version invalid
         * number count >= 1: version valid
         */
        QList<int> mNumbers;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_VERSION_H
